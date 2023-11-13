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
String sendedDay = "xx";
int sendedMinutes = 0;

Logger logger;
RestCall restCall;
ATFunctions atFunctions;
HexFunctions hexFunctions;
Timestamp timestamp;

void sendPollCommand(String pollCommand, Logger *logger, int timeToAttempt) {
  String frame = "";
  String restCallResponse;
  Serial.println("Polling to SPA...");
  logger -> info(0, "Sending " + pollCommand + " to SPA.");
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
      logger -> info(0, "Received frame data from SPA: '" + frameLog + "'");
    }
    if (actualMilis + timeToAttempt <= millis()) {
      Serial.printf("\nResending %s command...", &pollCommand);
      logger -> error(0, "Not frame received, resending command " + pollCommand + " to SPA.");
      atFunctions.sendATCommand(Serial2, AT_P2P_CONFIG_TX_SET);
      atFunctions.sendP2PPacket(Serial2, pollCommand);
      atFunctions.sendATCommand(Serial2, AT_SEMICONTINUOUS_PRECV_CONFIG_SET);
      actualMilis = millis();
    }
  }
  if(!frame.startsWith("ERROR")) {
    if(!pollCommand.startsWith(IRR_COMMAND)) {
      restCallResponse = restCall.sendFrameData(frame, STATION_TABLE, 3);
      logger -> log(restCall.getResponseCode(), restCallResponse, restCall.getDebugLevel(), RXSPA);
    } else {
      restCallResponse = restCall.sendFrameData(frame.substring(0, frame.indexOf(ETC)-2) + "}", STATION_TABLE, 3);
      logger -> log(restCall.getResponseCode(), restCallResponse, restCall.getDebugLevel(), RXSPA);
      
      restCallResponse = restCall.sendFrameData("{" + frame.substring(frame.indexOf(ETC)-1, frame.indexOf(WET_WEIGHT)-2) + "}", ETC_TABLE, 3);
      logger -> log(restCall.getResponseCode(), restCallResponse, restCall.getDebugLevel(), RXSPA);
      
      restCallResponse = restCall.sendFrameData("{" + frame.substring(frame.indexOf(WET_WEIGHT)-1, frame.length()), WET_WEIGHT_TABLE, 3);
      logger -> log(restCall.getResponseCode(), restCallResponse, restCall.getDebugLevel(), RXSPA);
    }
  } else {
    logger -> error(0, frame, "ADCSPA");
  }
}

void pollAlarm() {
  sendPollCommand(POLL_COMMAND, &logger, 10000);
}

void irrAlarm() {
  sendPollCommand(IRR_COMMAND, &logger, 10000);
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
  logger.config(String(API_URL) + String(LOG_CONTEXT), String(DB_USER), String(DB_PASS));
  restCall.config(String(API_URL), String(DB_USER), String(DB_PASS));

  String atCommandResetResponse = atFunctions.sendATCommand(Serial2, AT_RESET);
  String atConfigSetP2PResponse = atFunctions.sendATCommand(Serial2, AT_P2P_CONFIG_SET);
  String atConfigGetP2PResponse = atFunctions.sendATCommand(Serial2, AT_P2P_CONFIG_GET);
  String atConfigContRecvResponse = atFunctions.sendATCommand(Serial2, AT_P2P_CONFIG_TX_SET);
}

void loop() {
  String day = timestamp.getDay();
  String hour = timestamp.getHours();
  String minutes = timestamp.getMinutes();
  String seconds = timestamp.getSeconds();

  // Execute pollAlarm every one hour
  if (hour != sendedHour) {
    sendedHour = hour;
    pollAlarm();
  }

  // Execute irrAlarm once a day at 00 hs
  if (hour == "00" && sendedDay != day) {
    sendedDay = day;
    irrAlarm();
  }
}
