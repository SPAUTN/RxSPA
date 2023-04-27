#include <Arduino.h>
#include <WiFiManager.h>
#include <NTPClient.h>
#include <Utils.h>

#define AT_CONTINUOUS_PRECV_CONFIG_SET "AT+PRECV=65534" // 65534 for continuous receive, 65535 for continuous receive until one reception.
#define utcOffsetInSeconds 10800
#define POOL_NTP_URL "pool.ntp.org"

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, POOL_NTP_URL, -utcOffsetInSeconds);
bool initialized = false;
WiFiManager wifiManager;

String getLocalTimeStamp(NTPClient timeClient) {
  timeClient.update();
  time_t currentTime = timeClient.getEpochTime();

  // Verificar si el valor de tiempo Unix es negativo
  while (currentTime < 0) {
    timeClient.forceUpdate();
    currentTime = timeClient.getEpochTime();
  }
  struct tm timeInfo;
  localtime_r(&currentTime, &timeInfo);
  
  char timeString[30];
  snprintf(timeString, sizeof(timeString), "%04d-%02d-%02dT%02d:%02d:%02d%+03d:%02d", 
    timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday,
    timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec,
    (int) (utcOffsetInSeconds / 3600), abs((int) (utcOffsetInSeconds / 60) % 60));
  
  return timeString;
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200);
  wifiManager.autoConnect();
  String atCommandResetResponse = sendATCommand(Serial2, AT_RESET);
  String atConfigSetP2PResponse = sendATCommand(Serial2, AT_P2P_CONFIG_SET);
  String atConfigGetP2PResponse = sendATCommand(Serial2, AT_P2P_CONFIG_GET);
  String atConfigContRecvResponse = sendATCommand(Serial2, AT_CONTINUOUS_PRECV_CONFIG_SET);
}

void loop() {
      String rxData = readSerial(Serial2);
      rxData.trim();
      rxData = hexToASCII(rxData.substring(rxData.lastIndexOf(':')+1));
      Serial.print("Received: ");
      Serial.println(rxData);
      
      delay(1000);
      String currentTime = getLocalTimeStamp(timeClient);
      
}
