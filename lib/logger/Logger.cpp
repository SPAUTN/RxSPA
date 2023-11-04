#include "Logger.hpp"

Logger::Logger() {}

void Logger::config(String logHost, String user, String pass) {
    this -> logHost = logHost;
    this -> user = user;
    this -> pass = pass;
}



int Logger::log(int httpcode, String message, String level){
    // Execute the following code in a new thread
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

int Logger::logDaemon(int httpcode, String message, String level){
    // Execute the log method in a new thread
    std::thread t1(&Logger::writeLog, this, httpcode, message, level);
    t1.detach();
    return 0;
}