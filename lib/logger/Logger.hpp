#include <Arduino.h>
#include <HTTPClient.h>
#include <thread>

#define ERROR_LEVEL "ERROR"
#define INFO_LEVEL "INFORMATION"
#define DEBUG_LEVEL "DEBUG"

#ifndef LOGGER_H
#define LOGGER_H
class Logger {
    public:
        /**
         * @brief Construct a new Logger object
        */
        Logger();

        /**
         * @brief Configure the logger
         * 
         * @param logHost The host to send the logs
         * @param user The user to authenticate on the host
         * @param pass The password to authenticate on the host
        */
        void config(String logHost, String user, String pass);

        /**
         * @brief Log a message to the backend
         * 
         * @param httpcode The http code of the request
         * @param message The message to log
         * @param level The level of the message
         * @return int The http code of the request
        */
        int log(int httpcode, String message, String level);
        
        /**
         * @brief Log a message to the backend in a new thread (parallel)
        */
        int logDaemon(int httpcode, String message, String level);
        
    private:
        String logHost;
        String user;
        String pass;
        HTTPClient http;
};
#endif