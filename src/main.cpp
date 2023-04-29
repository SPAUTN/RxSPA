#include <Arduino.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <esp_sntp.h>
#include <esp_random.h>
#include <Utils.h>

#define AT_CONTINUOUS_PRECV_CONFIG_SET "AT+PRECV=65534" // 65534 for continuous receive, 65535 for continuous receive until one reception.
#define utcOffsetInSeconds 10800
#define POOL_NTP_URL "pool.ntp.org"

#define DB_HOST "https://spautncluster.aks1.eastus2.azure.cratedb.net:4200/_sql"
#define DB_USER "serviceesp"
#define DB_PASS "Spautn2023pf"

WiFiUDP ntpUDP;
bool initialized = false;
WiFiManager wifiManager;
HTTPClient http;

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

int parseRxData(int windSpeed, int windDirection, long int humidity, long int radiation, int temperature, int pressure, int leafMoisture, long int pluviometer, int weight){
  http.begin(DB_HOST);
  http.addHeader("Content-Type", "application/json");
  http.setAuthorization("admin", "BmY8bcMNbCgrsHDBmY8bcMNbCgrsHD");
  String currentTime = getLocalTimeStamp();
  String sqlTemplate = "{\"stmt\": \"INSERT INTO spa.weatherstation (timestamp, windSpeed, windDirection, humidity, radiation, temperature, pressure, leafMoisture, pluviometer, weight) VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10) \",\"args\":";
  char buffer[100]; 

  sprintf(buffer, "[\"%s\", %d, %d, %d, %d, %d, %d, %d, %d, %d]}", currentTime.c_str(), windSpeed, windDirection, humidity, radiation, temperature, pressure, leafMoisture, pluviometer, weight);

  String finalData = sqlTemplate + buffer;
  
  int httpResponseCode = http.PUT(finalData);
  http.end();
  return httpResponseCode;
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

  //ParseData

  // SendData
  //int httpResponseCode = parseRxData(windSpeed, windDirection, humidity, radiation, temperature, pressure, leafMoisture, pluviometer, weight);
  
  // Get httpResponseCode and make decision
  
  // Only for Test
  // Serial.print("RESPONSE: ");
  // Serial.println(parseRxData(esp_random() % 100, esp_random() % 100, esp_random() % 100, esp_random() % 100, esp_random() % 100, esp_random() % 100, esp_random() % 100, esp_random() % 100, esp_random() % 100));
  // delay(500);
  
}
