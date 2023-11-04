#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Logger.hpp>

#define STATION_TABLE "spa.weatherstation"
#define WET_WEIGHT_TABLE "spa.wetweights"
#define ETC_TABLE "spa.etc"

#define INSERT_CONTEXT "/insert"
#define LOG_CONTEXT "/log"
#define ETCRAIN_CONTEXT "/etcrain"

#define ERROR_LEVEL "ERROR"
#define INFO_LEVEL "INFORMATION"
#define DEBUG_LEVEL "DEBUG"

class Api {
    public:
        Api();
        String sendFrameData(String frame, String table, int attempts);
        String getWeightAndRain(String command = "");
        void config(String dbHost, String dbUser, String dbPass);
        void setDebugLevel(String debugLevel);
        String getDebugLevel();
        void setResponseCode(int responseCode);
        int getResponseCode();
    private:
        String dbHost;
        String dbUser;
        String dbPass;
        HTTPClient http;
        String debugLevel;
        int responseCode;
};