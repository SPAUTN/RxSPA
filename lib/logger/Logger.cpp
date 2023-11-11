#include "Logger.hpp"

void Logger::config(String logHost, String user, String pass) {
    this -> logHost = logHost;
    this -> user = user;
    this -> pass = pass;
}

int Logger::log(int httpcode, String message, String level, String source){
    int httpResponseCode = 0;
    String frame =  "\">hc:" + String(httpcode) +
                    ";msg:" + message +
                    ";lv:" + level +
                    ";src:" + source + "<\"";
    String bodyRequest = "{\"fr\": " + frame + "}";
    try {
        this -> http.clearAllCookies();
        this -> http.begin(this->logHost);
        this -> http.addHeader("Content-Type", "application/json");
        this -> http.setAuthorization(this->user.c_str(), this->pass.c_str());
        Serial.print("Logger bodyRequest: ");
        Serial.println(bodyRequest);
        httpResponseCode = this -> http.POST(bodyRequest);
        this -> http.end();
    } catch (const std::exception& e) {
        httpResponseCode = -1;
        Serial.println(e.what());
        this -> http.end();
    }
    return httpResponseCode;
}

int Logger::error(int httpcode, String message, String source) {
    return this -> log(httpcode, message, ERROR_LEVEL, source);
}

int Logger::info(int httpcode, String message, String source) {
    return this -> log(httpcode, message, INFO_LEVEL, source);
}

int Logger::debug(int httpcode, String message, String source) {
    return this -> log(httpcode, message, DEBUG_LEVEL, source);
}