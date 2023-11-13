#include "Logger.hpp"

void Logger::config(String logHost, String user, String pass, int attempts) {
    this -> logHost = logHost;
    this -> user = user;
    this -> pass = pass;
    this -> attempts = attempts;
}

int Logger::log(int httpcode, String message, String level, String source){
    int n_attempt = 0;
    int httpResponseCode = 0;
    String frame =  "\">hc:" + String(httpcode) +
                    ";msg:" + message +
                    ";lv:" + level +
                    ";src:" + source + "<\"";
    String bodyRequest = "{\"fr\": " + frame + "}";
    try {
        do {
            n_attempt++;
            this -> http.clearAllCookies();
            this -> http.begin(this->logHost);
            this -> http.addHeader("Content-Type", "application/json");
            this -> http.setAuthorization(this->user.c_str(), this->pass.c_str());
            Serial.print("Logger bodyRequest: ");
            Serial.println(bodyRequest);
            Serial.printf("Attempt nº: %d.\n", n_attempt);
            httpResponseCode = this -> http.POST(bodyRequest);
            this -> http.end();
        } while(n_attempt > (this -> attempts) && httpResponseCode != 201);
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