#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <Utils.h>
#include <esp_sntp.h>
#include <time.h>
#include <esp_random.h>
#include <Logger.hpp>

#define AT_CONTINUOUS_PRECV_CONFIG_SET "AT+PRECV=65534" // 65534 for continuous receive, 65535 for continuous receive until one reception.
#define AT_SEMICONTINUOUS_PRECV_CONFIG_SET "AT+PRECV=65535" 

#define utcOffsetInSeconds 10800
#define POOL_NTP_URL "pool.ntp.org"

#define DB_HOST "https://spa-backend-81f8-dev.fl0.io/insert"
#define LOG_HOST "https://spa-backend-81f8-dev.fl0.io/log"
#define ETCRAIN_HOST "https://spa-backend-81f8-dev.fl0.io/etcrain"

#define DB_USER "serviceesp"
#define DB_PASS "Spautn2023pf"

#define STATION_TABLE "spa.weatherstation"
#define WET_WEIGHT_TABLE "spa.wetweights"
#define ETC_TABLE "spa.etc"

WiFiUDP ntpUDP;
WiFiManager wifiManager;
HTTPClient http;
String sendedHour = "xx";
int sendedMinutes = 0;

Logger logger;

String getLocalTimeStamp() {
  time_t now;
  struct tm timeInfo;
  char strftime_buf[64];

  time(&now);
  localtime_r(&now, &timeInfo);
    
  char timeString[30];
  snprintf(timeString, sizeof(timeString), "%04d-%02d-%02dT%02d:%02d:%02d%", 
    timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday,
    timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec);

  logger.config(String(LOG_HOST), String(DB_USER), String(DB_PASS));
  
  return timeString;
}

int sendFrameData(String frame, String table, int attempts){
  int n_attemp = 0;
  Serial.print("Frame to send: ");
  Serial.println(frame);
  http.begin(DB_HOST);
  http.addHeader("Content-Type", "application/json");
  http.setAuthorization(DB_USER, DB_PASS);
  String bodyRequest = "{\"table\": \"" + table + "\",\"frame\": " + frame + "}";
  Serial.print("Body request: ");
  int httpResponseCode;
  String log_message;
  do {
    n_attemp ++;
    Serial.println(bodyRequest);
    httpResponseCode = http.POST(bodyRequest);
    String httpMessage = http.getString();
    httpMessage = httpMessage.substring(12, httpMessage.length()-2);
    log_message = "Inserting on table: " + table + " - http message: <" + httpMessage + "> - on inserting frame";
    Serial.print("HTTP Response code: ");
    Serial.print(httpResponseCode);
    Serial.print(" on attemp number ");
    Serial.println(n_attemp);
    if(httpResponseCode != 201) {
      logger.log(httpResponseCode, log_message, ERROR_LEVEL);
      delay(500);
    } else {
      logger.log(httpResponseCode, log_message, DEBUG_LEVEL);
    }
  } while (n_attemp > attempts && httpResponseCode != 201);
  
  http.end();

  return httpResponseCode;
}

String queryWetweightAndRainValues(String command = "") {
  HTTPClient http;
  String wetweightAndRainValues = "";
  http.begin(ETCRAIN_HOST);
  http.addHeader("Content-Type", "application/json");
  http.setAuthorization(DB_USER, DB_PASS);
  int httpCode = http.GET();

  if (httpCode == 200) {
    String responseBody = http.getString();
    http.end();
    Serial.print("Query received: ");
    Serial.println(responseBody);
    const size_t capacity = JSON_OBJECT_SIZE(2) + 40;
    DynamicJsonDocument doc(capacity);
    deserializeJson(doc, responseBody);
    double wetweight = doc["wetweight"];
    double cumulative_rain = doc["cumulative_rain"];
    wetweightAndRainValues = String(wetweight, 2) + ";" + String(cumulative_rain, 2);
    Serial.println(wetweightAndRainValues);
  }
  return command + ";" + wetweightAndRainValues + ";";
}

void setup() {
  // Internal clock
  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  sntp_setservername(0, POOL_NTP_URL);
  sntp_init();
  sntp_sync_time(0);

  Serial.begin(115200);
  Serial2.begin(115200);
  wifiManager.autoConnect();
  String atCommandResetResponse = sendATCommand(Serial2, AT_RESET);
  String atConfigSetP2PResponse = sendATCommand(Serial2, AT_P2P_CONFIG_SET);
  String atConfigGetP2PResponse = sendATCommand(Serial2, AT_P2P_CONFIG_GET);
  String atConfigContRecvResponse = sendATCommand(Serial2, AT_P2P_CONFIG_TX_SET);
}

void loop() {
  String pollCommand;
  int httpResponse;
  int timeToAttempt = 0;
  String currentTime = getLocalTimeStamp();
  String hour = currentTime.substring(11,13);
  int minutes = atoi(currentTime.substring(14,16).c_str());
  int seconds = atoi(currentTime.substring(18,19).c_str());
  String frame = "";

  if(minutes % 2 == 0) {
    if(sendedMinutes != minutes){
      Serial.println("Polling to SPA...");
      if(currentTime.substring(11,19) == "00:00:00"){
        pollCommand = queryWetweightAndRainValues(String(IRR_COMMAND));
        timeToAttempt = 60000;
      } else {
        pollCommand = POLL_COMMAND;
        timeToAttempt = 10000;
      }    
      String pollResponse = sendP2PPacket(Serial2, pollCommand);
      logger.log(0, "Sended " + pollCommand + "to SPA.", INFO_LEVEL);
      String listeningResponse = sendATCommand(Serial2, AT_SEMICONTINUOUS_PRECV_CONFIG_SET);
      boolean frameReceived = false;
      long actualMilis = millis();
      Serial.print("Waiting response:");
      while (!frameReceived){
        Serial.print(".");
        delay(200);
        if (Serial2.available() > 0) {
          String rxData = readSerial(Serial2);
          rxData.trim();
          rxData = hexToASCII(rxData.substring(rxData.lastIndexOf(':')+1));
          Serial.print("\nReceived data: ");
          Serial.println(rxData);
          frameReceived = true;
          frame = rxData;
          String frameLog = frame;
          frameLog.replace("\"", "'");
          logger.log(0, "Received frame data from SPA: '" + frameLog + "'", INFO_LEVEL);
        }
        if (actualMilis + timeToAttempt <= millis()) {
          Serial.printf("\nResending %s command...", &pollCommand);
          logger.log(0, "Not frame received, resending command " + pollCommand + " to SPA.", ERROR_LEVEL);
          sendATCommand(Serial2, AT_P2P_CONFIG_TX_SET);
          sendP2PPacket(Serial2, pollCommand);
          sendATCommand(Serial2, AT_SEMICONTINUOUS_PRECV_CONFIG_SET);
          actualMilis = millis();
        }
      }
      
      if(!pollCommand.startsWith(IRR_COMMAND)) {
        httpResponse = sendFrameData(frame, STATION_TABLE, 3);
      } else {
        httpResponse = sendFrameData(frame.substring(0, frame.indexOf("etc")-2) + "}", STATION_TABLE, 3);
        httpResponse = sendFrameData("{" + frame.substring(frame.indexOf("etc")-1, frame.indexOf("wetweight")-2) + "}", ETC_TABLE, 3);
        httpResponse = sendFrameData("{" + frame.substring(frame.indexOf("wetweight")-1, frame.length()), WET_WEIGHT_TABLE, 3);
      }
      sendedHour = hour;
      sendedMinutes = minutes;
    }
  }
}
