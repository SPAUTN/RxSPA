#include "Logger.hpp"

Logger::Logger() {}

void Logger::config(String logHost, String user, String pass) {
    this -> logHost = logHost;
    this -> user = user;
    this -> pass = pass;
}


int Logger::log(int httpcode, String message, String level){
    String frame = "{\"httpcode\": \"" + String(httpcode) + "\",\"message\": \"" + message + "\",\"level\":\"" + level + "\",\"source\":\"RXSPA\"}";
    http.begin(this->logHost);
    http.addHeader("Content-Type", "application/json");
    http.setAuthorization(this->user.c_str(), this->pass.c_str());
    String bodyRequest = "{\"frame\": " + frame + "}";
    Serial.print("Logger bodyRequest: ");
    Serial.println(bodyRequest);
    int httpResponseCode = http.POST(bodyRequest);
    http.end();
    return httpResponseCode;
}
