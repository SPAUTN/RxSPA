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

#define DB_HOST "https://spautncluster.aks1.eastus2.azure.cratedb.net:4200/_sql"
#define DB_USER "serviceesp"
#define DB_PASS "Spautn2023pf"
#define DRY_WEIGHT_TABLE "spa.dryweights"
#define WET_WEIGHT_TABLE "spa.wetweights"

WiFiUDP ntpUDP;
WiFiManager wifiManager;
HTTPClient http;
String sendedHour = "xx";
int sendedMinutes = 0;

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

int parseRxData(long int rainMilimeters, int windSpeed,int windDirection,
                int leafMoisture,long int humidity, long int radiation,
                int temperature, int pressure, int weight){
  http.begin(DB_HOST);
  http.addHeader("Content-Type", "application/json");
  http.setAuthorization(DB_USER, DB_PASS);
  String sqlTemplate = "{\"stmt\": \"INSERT INTO spa.weatherstation (timestamp, windSpeed, windDirection, humidity, radiation, temperature, pressure, leafMoisture, pluviometer, weight) VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10) \",\"args\":";
  char buffer[100]; 

  sprintf(buffer, "[\"%s\", %d, %d, %d, %d, %d, %d, %d, %d, %d]}", getLocalTimeStamp().c_str(), windSpeed, windDirection, humidity, radiation, temperature, pressure, leafMoisture, rainMilimeters, weight);

  String finalData = sqlTemplate + buffer;
  
  int httpResponseCode = http.PUT(finalData);
  http.end();
  return httpResponseCode;
}


int parseWeightData(long int weight, String table){
  DynamicJsonDocument doc(512);
  http.begin(DB_HOST);
  http.addHeader("Content-Type", "application/json");
  http.setAuthorization(DB_USER, DB_PASS);
  String sqlTemplate = "{\"stmt\": \"INSERT INTO " + table + " (id, timestamp, weight) VALUES ($1, $2, $3) \",\"args\":";
  char buffer[100];

  http.POST("{\"stmt\":\"SELECT max(id) FROM " + table + "\"}");
  String queryResponse = http.getString();

  DeserializationError error = deserializeJson(doc, queryResponse);
  if (error) {
    Serial.printf("Error at trying to parse JSON of DB response: %s \n", http.getString());
    Serial.printf("Message error: %s \n", error.c_str());
  }
  String id_string_arr = doc["rows"];
  int id = id_string_arr.substring(2, id_string_arr.length() - 2).toInt() + 1;
  sprintf(buffer, "[%d,\"%s\", %d]}", id, getLocalTimeStamp().c_str(), weight);
  String finalData = sqlTemplate + buffer;
  int httpResponseCode = http.PUT(finalData);
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
  String jsonString = "";

  if(minutes % 2 == 0) {
    if(sendedMinutes != minutes){
      Serial.println("Polling to SPA...");
      if(currentTime.substring(11,19) == "00:00:00"){
        pollCommand = "IRR";
      } else {
        pollCommand = "POLL";
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
          jsonString = rxData;
        }
        if (actualMilis + 5000 <= millis()) {
          Serial.printf("\nResending %s command...", pollCommand);
          sendATCommand(Serial2, AT_P2P_CONFIG_TX_SET);
          sendP2PPacket(Serial2, pollCommand);
          sendATCommand(Serial2, AT_SEMICONTINUOUS_PRECV_CONFIG_SET);
          actualMilis = millis();
        }
      }

      do {
        DynamicJsonDocument doc(2048);
        DeserializationError error = deserializeJson(doc, jsonString);
        if (error) {
          Serial.printf("Error at trying to parse JSON: %s \n", jsonString);
          Serial.printf("Message error: %s \n", error.c_str());
        }

        int rainMilimeters = doc["rain_milimeters"];
        int windSpeed = doc["wind_speed"];
        int windDirection = doc["wind_direction"];
        int leafMoisture = doc["leaf_moisture"];
        long int humidity = doc["relative_humidity"];
        long int radiation = doc["solar_radiation"];
        int temperature = doc["temperature"];
        int pressure = doc["pressure"];
        int weight = doc["weight"];
        httpResponse = parseRxData(rainMilimeters, windSpeed, windDirection, leafMoisture, humidity, radiation, temperature, pressure, weight);
        logWrite(currentTime, httpResponse);
        
        int dryweight;
        int wetweight;

        try {
          dryweight = doc["dryweight"];
          wetweight = doc["wetweight"];
          Serial.print("DryWeight: ");
          Serial.println(dryweight);
          Serial.print("WetWeight: ");
          Serial.println(wetweight);
          httpResponse = parseWeightData(dryweight, DRY_WEIGHT_TABLE);
          logWrite(currentTime, httpResponse);
          httpResponse = parseWeightData(wetweight, WET_WEIGHT_TABLE);
          logWrite(currentTime, httpResponse);
        } catch(const std::exception& e) {
          Serial.println("No measures for weights before and after irrigations.");
        }

        
        Serial.print("Last sendedMinutes: ");
        Serial.println(sendedMinutes);
        sendedHour = hour;
        sendedMinutes = minutes;
        Serial.print(currentTime);
        Serial.print(" -> ");
        Serial.println(httpResponse);
        
        Serial.print("New sendedMinutes: ");
        Serial.println(sendedMinutes);
      } while (httpResponse != 200);
    }
  } 
}
