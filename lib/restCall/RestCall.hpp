#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Logger.hpp>

#define STATION_TABLE "spa.weatherstation"
#define WET_WEIGHT_TABLE "spa.wetweights"
#define ETC_TABLE "spa.etc"

#define INSERT_CONTEXT "/insert"
#define LOG_CONTEXT "/log"
#define ETCRAIN_CONTEXT "/etc"
#define PING_CONTEXT "/ping"

#define ERROR_LEVEL "ERROR"
#define INFO_LEVEL "INFORMATION"
#define DEBUG_LEVEL "DEBUG"

class RestCall {
    public:

        /**
         * @brief Configure the rest call
         * 
         * @param apiUrl The host to send the data
         * @param dbUser The user to authenticate on the host
         * @param dbPass The password to authenticate on the host
        */
        void config(String apiUrl, String dbUser, String dbPass);        
        
        /**
         * @brief Get the weight and rain values from the backend
         * 
         * @param command The command to send to the backend
         * @return String The response of the backend
        */
        String getWeight(String command);
        
        /**
         * @brief Send a frame to the backend
         * 
         * @param frame The frame to send
         * @param table The table to insert the frame
         * @param attempts The number of attempts to send the frame
         * @return String The response of the backend
        */
        String sendFrameData(String frame, String table, int attempts);

        /**
         * @brief Send a ping to the backend for up the server
         * 
        */
        String ping(int attempts);

        /**
         * @brief Set the debug level
         * 
         * @param debugLevel The debug level to set
         * @return String The debug level
        */
        void setDebugLevel(String debugLevel);

        /**
         * @brief Get the debug level
         * 
         * @return String The debug level
        */
        String getDebugLevel();

        /**
         * @brief Set the response code
         * 
         * @param responseCode The response code to set
         * @return int The response code
        */
        void setResponseCode(int responseCode);

        /**
         * @brief Get the response code
         * 
         * @return int The response code
        */
        int getResponseCode();

    private:
        String apiUrl;
        String dbUser;
        String dbPass;
        HTTPClient http;
        String debugLevel;
        int responseCode;
};