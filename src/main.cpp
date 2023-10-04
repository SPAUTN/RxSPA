#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <Utils.h>
#include <esp_sntp.h>
#include <time.h>
#include <esp_random.h>

#define AT_CONTINUOUS_PRECV_CONFIG_SET "AT+PRECV=65534" // 65534 for continuous receive, 65535 for continuous receive until one reception.
#define AT_SEMICONTINUOUS_PRECV_CONFIG_SET "AT+PRECV=65535" 
#define utcOffsetInSeconds 10800
#define POOL_NTP_URL "pool.ntp.org"

#define DB_HOST "https://spa-backend-81f8-dev.fl0.io/insert"
#define DB_USER "serviceesp"
#define DB_PASS "Spautn2023pf"
#define STATION_TABLE "spa.weatherstation"
#define DRY_WEIGHT_TABLE "spa.dryweights"
#define WET_WEIGHT_TABLE "spa.wetweights"

WiFiUDP ntpUDP;
WiFiManager wifiManager;
HTTPClient http;
String sendedHour = "xx";
int sendedMinutes = 0;

struct frameStructure {
  int rainMilimeters;
  int windSpeed;
  int windDirection;
  int leafMoisture;
  long int humidity;
  long int radiation;
  int temperature;
  int pressure;
  int weight;
  int dryweight;
  int wetweight;
};

String getLocalTimeStamp() {
  time_t now;
  struct tm timeInfo;
  char strftime_buf[64];

  time(&now);
  localtime_r(&now, &timeInfo);
  
  //localtime_r(&currentTime, &timeInfo);
  
  char timeString[30];
  snprintf(timeString, sizeof(timeString), "%04d-%02d-%02dT%02d:%02d:%02d%", 
    timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday,
    timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec);
  
  return timeString;
}

// rainMilimeters, windSpeed, windDirection, leafMoisture, humidity, radiation, temperature, pressure, weight

int sendFrameData(String frame, String table){
  Serial.print("Frame to send: ");
  Serial.println(frame);
  http.begin(DB_HOST);
  http.addHeader("Content-Type", "application/json");
  //http.setAuthorization(DB_USER, DB_PASS);
  String bodyRequest = "{\"table\": \"" + table + "\",\"user\": \"" + DB_USER + "\",\"password\": \"" +DB_PASS + "\",\"frame\": " + frame + "}";
  Serial.print("Body request: ");
  Serial.println(bodyRequest);
  int httpResponseCode = http.PUT(bodyRequest);
  http.end();
  return httpResponseCode;
}

int logWrite(String timestamp, int httpCode){
  http.begin(DB_HOST);
  http.addHeader("Content-Type", "application/json");
  http.setAuthorization(DB_USER, DB_PASS);
  String sqlTemplate = "{\"stmt\": \"INSERT INTO spa.logs (timestamp, httpCode) VALUES ($1, $2) \",\"args\":";
  char buffer[100]; 

  sprintf(buffer, "[\"%s\", %d]}", timestamp.c_str(), httpCode);

  String finalData = sqlTemplate + buffer;
  
  int httpResponseCode = http.PUT(finalData);
  http.end();
  return httpResponseCode;
}

frameStructure parseFrameJson (String frame) {
  frameStructure frameStructureReceived;
  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, frame);
  if (error) {
    Serial.printf("Error at trying to parse JSON: %s \n", frame);
    Serial.printf("Message error: %s \n", error.c_str());
  }

  frameStructureReceived.rainMilimeters = doc["rain_milimeters"];
  frameStructureReceived.windSpeed = doc["wind_speed"];
  frameStructureReceived.windDirection = doc["wind_direction"];
  frameStructureReceived.leafMoisture = doc["leaf_moisture"];
  frameStructureReceived.humidity = doc["relative_humidity"];
  frameStructureReceived.radiation = doc["solar_radiation"];
  frameStructureReceived.temperature = doc["temperature"];
  frameStructureReceived.pressure = doc["pressure"];
  frameStructureReceived.weight = doc["weight"];

  int dryweight;
  int wetweight;

  try {
    dryweight = doc["dryweight"];
    wetweight = doc["wetweight"];
  } catch(const std::exception& e) {
    Serial.println("No measures for weights before and after irrigations.");
  }

  return frameStructureReceived;
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
  String currentTime = getLocalTimeStamp();
  String hour = currentTime.substring(11,13);
  int minutes = atoi(currentTime.substring(14,16).c_str());
  int seconds = atoi(currentTime.substring(18,19).c_str());
  String frame = "";

  if(minutes % 2 == 0) {
    if(sendedMinutes != minutes){
      Serial.println("Polling to SPA...");
      if(currentTime.substring(11,19) == "00:00:00"){
        pollCommand = IRR_COMMAND;
      } else {
        pollCommand = POLL_COMMAND;
      }
      String pollResponse = sendP2PPacket(Serial2, pollCommand);
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
        }
        if (actualMilis + 5000 <= millis()) {
          Serial.printf("\nResending %s command...", pollCommand);
          sendATCommand(Serial2, AT_P2P_CONFIG_TX_SET);
          sendP2PPacket(Serial2, pollCommand);
          sendATCommand(Serial2, AT_SEMICONTINUOUS_PRECV_CONFIG_SET);
          actualMilis = millis();
        }
      }

      // TODO: Modificar parseRxData para que solo reciba la structure, modificar tambien nombre, no es parse sino send
      if(pollCommand != IRR_COMMAND) {
        httpResponse = sendFrameData(frame, STATION_TABLE);
        logWrite(currentTime, httpResponse);
      } else {
        httpResponse = sendFrameData(frame.substring(0, frame.indexOf("dryweight")-2) + "}", STATION_TABLE);
        logWrite(currentTime, httpResponse);
        httpResponse = sendFrameData("{" + frame.substring(frame.indexOf("dryweight")-1, frame.indexOf("wetweight")-2) + "}", DRY_WEIGHT_TABLE);
        logWrite(currentTime, httpResponse);
        httpResponse = sendFrameData("{" + frame.substring(frame.indexOf("wetweight")-1, frame.length()), WET_WEIGHT_TABLE);
        logWrite(currentTime, httpResponse);
      }
      sendedHour = hour;
      sendedMinutes = minutes;
    } while (httpResponse != 200);
  }
}
