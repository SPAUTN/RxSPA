#include <Arduino.h>
#include <WiFiManager.h>
#include <Utils.hpp>
#include <esp_sntp.h>
#include <time.h>
#include <Logger.hpp>
#include <RestCall.hpp>
#include "secrets.h"

#define POOL_NTP_URL "pool.ntp.org"

#define API_URL "https://backend-spa.onrender.com"

#define IRR_HOUR "18"
#define POLL_MINUTES "00"

#define ETC "etc"
#define WET_WEIGHT "wwh"
#define IRR_PREFIX ">IRR+"
#define FRAME_CLOSE "<"
#define ERROR "ERROR"
#define ADCSPA "ADCSPA"

WiFiManager wifiManager;
String sendedHour = "xx";
String sendedDay = "xx";
int sendedMinutes = 0;

Logger logger;
RestCall restCall;
ATFunctions atFunctions;
HexFunctions hexFunctions;
Timestamp timestamp;

void sendPollCommand(String pollCommand, Logger *logger, int timeToAttempt, int attempts = 5) {
  int currentAttempt = 1;
  String frame = "";
  String restCallResponse;
  Serial.println("Polling to SPA...");
  logger -> info(0, "Sending " + pollCommand + " to SPA.");
  String pollResponse = atFunctions.sendP2PPacket(Serial2, pollCommand);
  String listeningResponse = atFunctions.sendATCommand(Serial2, AT_SEMICONTINUOUS_PRECV_CONFIG_SET);
  boolean frameReceived = false;
  long actualMilis = millis();
  Serial.print("Waiting response:");
  while (!frameReceived && currentAttempt < attempts){
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
      logger -> error(0, "Not frame received, resending command " + pollCommand + " to SPA." + " Attempt: " + String(currentAttempt));
      currentAttempt++;
      if (currentAttempt >= attempts) {
        Serial.println("\nMax attempts reached.");
        logger -> error(0, "Max attempts reached. Not frame received from SPA.");
      }
      atFunctions.sendATCommand(Serial2, AT_P2P_CONFIG_TX_SET);
      atFunctions.sendP2PPacket(Serial2, pollCommand);
      atFunctions.sendATCommand(Serial2, AT_SEMICONTINUOUS_PRECV_CONFIG_SET);
      actualMilis = millis();
    }
  }
  if(!frame.startsWith(ERROR)) {
    if(!pollCommand.startsWith(IRR_COMMAND)) {
      restCallResponse = restCall.sendFrameData(frame, STATION_TABLE, 3);
      logger -> log(restCall.getResponseCode(), restCallResponse, restCall.getDebugLevel(), RXSPA);
    } else {
      restCallResponse = restCall.sendFrameData(frame.substring(0, frame.indexOf(ETC)-2) + FRAME_CLOSE, STATION_TABLE, 3);
      logger -> log(restCall.getResponseCode(), restCallResponse, restCall.getDebugLevel(), RXSPA);
      
      restCallResponse = restCall.sendFrameData(IRR_PREFIX + frame.substring(frame.indexOf(ETC), frame.indexOf(WET_WEIGHT)-2) + FRAME_CLOSE, ETC_TABLE, 3);
      logger -> log(restCall.getResponseCode(), restCallResponse, restCall.getDebugLevel(), RXSPA);
      
      restCallResponse = restCall.sendFrameData(IRR_PREFIX + frame.substring(frame.indexOf(WET_WEIGHT), frame.length()), WET_WEIGHT_TABLE, 3);
      logger -> log(restCall.getResponseCode(), restCallResponse, restCall.getDebugLevel(), RXSPA);
    }
  } else {
    logger -> error(0, frame, ADCSPA);
  }
}

void pollAlarm() {
  sendPollCommand(POLL_COMMAND, &logger, 10000);
}

void irrAlarm() {
  sendPollCommand(restCall.getWeight(IRR_COMMAND), &logger, 50000);
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

  if (minutes.equals("55")) {
    Serial.printf("Ping response: %s\n", restCall.ping(5));
    delay(60000);
  }
  
  // Execute irrAlarm once a day at 00 hs
  if (hour.equals(IRR_HOUR) && !sendedDay.equals(day)) {
    sendedDay = day;
    sendedHour = hour;
    irrAlarm();
  }
  
  // Execute pollAlarm every one hour
  if (minutes.equals(POLL_MINUTES) && !hour.equals(sendedHour)) {
    sendedHour = hour;
    pollAlarm();
  }
}
