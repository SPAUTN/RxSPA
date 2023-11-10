#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <Utils.hpp>
#include <esp_sntp.h>
#include <time.h>
#include <esp_random.h>
#include <Logger.hpp>
#include <RestCall.hpp>

#define utcOffsetInSeconds 10800
#define POOL_NTP_URL "pool.ntp.org"

#define API_URL "https://spa-backend-81f8-dev.fl0.io"

#define DB_USER "serviceesp"
#define DB_PASS "Spautn2023pf"

#define ETC "etc"
#define WET_WEIGHT "wetweight"

WiFiUDP ntpUDP;
WiFiManager wifiManager;
HTTPClient http;
String sendedHour = "xx";
int sendedMinutes = 0;

Logger logger;
RestCall restCall;
ATFunctions atFunctions;
HexFunctions hexFunctions;
Timestamp timestamp;

void setup() {
  // Internal clock
  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  sntp_setservername(0, POOL_NTP_URL);
  sntp_init();
  sntp_sync_time(0);

  Serial.begin(115200);
  Serial2.begin(115200);
  wifiManager.autoConnect();
  logger.config(String(API_URL) + String(LOG_CONTEXT), String(DB_USER), String(DB_PASS));
  restCall.config(String(API_URL), String(DB_USER), String(DB_PASS));

  String atCommandResetResponse = atFunctions.sendATCommand(Serial2, AT_RESET);
  String atConfigSetP2PResponse = atFunctions.sendATCommand(Serial2, AT_P2P_CONFIG_SET);
  String atConfigGetP2PResponse = atFunctions.sendATCommand(Serial2, AT_P2P_CONFIG_GET);
  String atConfigContRecvResponse = atFunctions.sendATCommand(Serial2, AT_P2P_CONFIG_TX_SET);
}

void loop() {
  String pollCommand;
  String restCallResponse;
  int timeToAttempt = 0;
  String currentTime = timestamp.getLocalTimeStamp();
  String hour = timestamp.getHours();
  int minutes = atoi(timestamp.getMinutes().c_str());
  int seconds = atoi(timestamp.getSeconds().c_str());
  String frame = "";

  if(minutes == 0 && seconds == 0) {
    if(sendedHour != hour) {
      Serial.println("Polling to SPA...");
      if(currentTime.substring(11,19) == "00:00:00"){
        pollCommand = restCall.getWeightAndRain(String(IRR_COMMAND));
        timeToAttempt = 60000;
      } else {
        pollCommand = POLL_COMMAND;
        timeToAttempt = 15000;
      }    
      logger.info(0, "Sending " + pollCommand + " to SPA.");
      String pollResponse = atFunctions.sendP2PPacket(Serial2, pollCommand);
      String listeningResponse = atFunctions.sendATCommand(Serial2, AT_SEMICONTINUOUS_PRECV_CONFIG_SET);
      boolean frameReceived = false;
      long actualMilis = millis();
      Serial.print("Waiting response:");
      while (!frameReceived){
        Serial.print(".");
        delay(200);
        if (Serial2.available() > 0) {
          String rxData = atFunctions.readSerial(Serial2);
          rxData = hexFunctions.hexToASCII(rxData.substring(rxData.lastIndexOf(':')+1));
          Serial.print("\nReceived data: ");
          Serial.println(rxData);
          frameReceived = true;
          frame = rxData;
          String frameLog = frame;
          frameLog.replace("\"", "'");
          logger.info(0, "Received frame data from SPA: '" + frameLog + "'");
        }
        if (actualMilis + timeToAttempt <= millis()) {
          Serial.printf("\nResending %s command...", &pollCommand);
          logger.error(0, "Not frame received, resending command " + pollCommand + " to SPA.");
          atFunctions.sendATCommand(Serial2, AT_P2P_CONFIG_TX_SET);
          atFunctions.sendP2PPacket(Serial2, pollCommand);
          atFunctions.sendATCommand(Serial2, AT_SEMICONTINUOUS_PRECV_CONFIG_SET);
          actualMilis = millis();
        }
      }
      if(!frame.startsWith("ERROR")) {
        if(!pollCommand.startsWith(IRR_COMMAND)) {
          restCallResponse = restCall.sendFrameData(frame, STATION_TABLE, 3);
          logger.log(restCall.getResponseCode(), restCallResponse, restCall.getDebugLevel(), RXSPA);
        } else {
          restCallResponse = restCall.sendFrameData(String(IRR_COMMAND) + frame.substring(0, frame.indexOf(ETC)-1), STATION_TABLE, 3);
          logger.log(restCall.getResponseCode(), restCallResponse, restCall.getDebugLevel(), RXSPA);
          
          restCallResponse = restCall.sendFrameData(String(IRR_COMMAND) + frame.substring(frame.indexOf(ETC)-1, frame.indexOf(WET_WEIGHT)-1) , ETC_TABLE, 3);
          logger.log(restCall.getResponseCode(), restCallResponse, restCall.getDebugLevel(), RXSPA);
          
          restCallResponse = restCall.sendFrameData(String(IRR_COMMAND) + frame.substring(frame.indexOf(WET_WEIGHT)-1, frame.length()), WET_WEIGHT_TABLE, 3);
          logger.log(restCall.getResponseCode(), restCallResponse, restCall.getDebugLevel(), RXSPA);
        }
        sendedHour = hour;
        sendedMinutes = minutes;
      } else {
        logger.error(0, frame, "ADCSPA");
      }
      
    }
  }
}
