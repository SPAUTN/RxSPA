#include <Arduino.h>
#include <WiFiManager.h>
#include <NTPClient.h>
#include <HTTPClient.h>
#include <Utils.h>
#include <esp_random.h>

#define AT_CONTINUOUS_PRECV_CONFIG_SET "AT+PRECV=65534" // 65534 for continuous receive, 65535 for continuous receive until one reception.
#define utcOffsetInSeconds 10800
#define POOL_NTP_URL "pool.ntp.org"

#define DB_HOST "https://spautncluster.aks1.eastus2.azure.cratedb.net:4200/_sql"
#define DB_USER "serviceesp"
#define DB_PASS "Spautn2023pf"

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, POOL_NTP_URL);
bool initialized = false;
WiFiManager wifiManager;
HTTPClient http;
// test variable
String sendedHour = "xx";
int sendedMinutes = 0;

String getLocalTimeStamp(NTPClient timeClient) {
  timeClient.update();
  time_t currentTime = timeClient.getEpochTime();

  // Verificar si el valor de tiempo Unix es coherente
  while (currentTime < 1682646677 || currentTime > 2000000000) {
    timeClient.forceUpdate();
    currentTime = timeClient.getEpochTime();
  }
  struct tm timeInfo;
  localtime_r(&currentTime, &timeInfo);
  
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
  String currentTime = getLocalTimeStamp(timeClient);
  String sqlTemplate = "{\"stmt\": \"INSERT INTO spa.weatherstation (timestamp, windSpeed, windDirection, humidity, radiation, temperature, pressure, leafMoisture, pluviometer, weight) VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10) \",\"args\":";
  char buffer[100]; 

  sprintf(buffer, "[\"%s\", %d, %d, %d, %d, %d, %d, %d, %d, %d]}", currentTime.c_str(), windSpeed, windDirection, humidity, radiation, temperature, pressure, leafMoisture, pluviometer, weight);

  String finalData = sqlTemplate + buffer;
  
  int httpResponseCode = http.PUT(finalData);
  http.end();
  return httpResponseCode;
}

int logWrite(String timestamp, int httpCode){
  http.begin(DB_HOST);
  http.addHeader("Content-Type", "application/json");
  http.setAuthorization("admin", "BmY8bcMNbCgrsHDBmY8bcMNbCgrsHD");
  String sqlTemplate = "{\"stmt\": \"INSERT INTO spa.logs (timestamp, httpCode) VALUES ($1, $2) \",\"args\":";
  char buffer[100]; 

  sprintf(buffer, "[\"%s\", %d]}", timestamp.c_str(), httpCode);

  String finalData = sqlTemplate + buffer;
  
  int httpResponseCode = http.PUT(finalData);
  http.end();
  return httpResponseCode;
}

void setup() {
  Serial.begin(115200);
  // Serial2.begin(115200);
  wifiManager.autoConnect();
  // String atCommandResetResponse = sendATCommand(Serial2, AT_RESET);
  // String atConfigSetP2PResponse = sendATCommand(Serial2, AT_P2P_CONFIG_SET);
  // String atConfigGetP2PResponse = sendATCommand(Serial2, AT_P2P_CONFIG_GET);
  // String atConfigContRecvResponse = sendATCommand(Serial2, AT_CONTINUOUS_PRECV_CONFIG_SET);
}

void loop() {
  // String rxData = readSerial(Serial2);
  // rxData.trim();
  // rxData = hexToASCII(rxData.substring(rxData.lastIndexOf(':')+1));
  // Serial.print("Received: ");
  // Serial.println(rxData);

  //ParseData

  // SendData
  //int httpResponseCode = parseRxData(windSpeed, windDirection, humidity, radiation, temperature, pressure, leafMoisture, pluviometer, weight);
  
  // Get httpResponseCode and make decision
  
  // Only for Test -- Send only once per hour
  int httpResponse;
  String currentTime = getLocalTimeStamp(timeClient);
  String hour = currentTime.substring(11,13);
  int minutes = atoi(currentTime.substring(14,16).c_str());

  if(minutes % 2 == 0 && sendedMinutes != minutes) {
    do {
      httpResponse = parseRxData(esp_random() % 100, esp_random() % 100, esp_random() % 100, esp_random() % 100, esp_random() % 100, esp_random() % 100, esp_random() % 100, esp_random() % 100, esp_random() % 100);
      // Serial.print("RESPONSE: ");
      // Serial.println(httpResponse);
      // Serial.print("TIME: ");
      // Serial.print(currentTime);

      // log table writing
      logWrite(currentTime, httpResponse);
      // Serial.print("Last sendedHour: ");
      // Serial.println(sendedHour);
      
      Serial.print("Last sendedMinutes: ");
      Serial.println(sendedMinutes);
      sendedHour = hour;
      sendedMinutes = minutes;
      Serial.print(currentTime);
      Serial.print(" -> ");
      Serial.println(httpResponse);
      // Serial.print("New sendedHour: ");
      // Serial.println(sendedHour);
      
      Serial.print("New sendedMinutes: ");
      Serial.println(sendedMinutes);
    } while (httpResponse != 200);
  }
}
