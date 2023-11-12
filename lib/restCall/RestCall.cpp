#include "RestCall.hpp"

void RestCall::config(String apiUrl, String dbUser, String dbPass) {
    this -> apiUrl = apiUrl;
    this -> dbUser = dbUser;
    this -> dbPass = dbPass;
}

void RestCall::setDebugLevel(String debugLevel) {
    this -> debugLevel = debugLevel;
}

String RestCall::getDebugLevel() {
    return this -> debugLevel;
}

void RestCall::setResponseCode(int responseCode) {
    this -> responseCode = responseCode;
}

int RestCall::getResponseCode() {
    return this -> responseCode;
}

String RestCall::sendFrameData(String frame, String table, int attempts){
    int n_attemp = 0;   
    String log_message;
    int httpCode;
    try {
        this -> http.clearAllCookies();
        Serial.print("Frame to send: ");
        Serial.println(frame);
        String bodyRequest = "{\"tb\": \"" + table + "\",\"fr\": \"" + frame + "\"}";
        Serial.print("Body request: ");
        this -> http.begin(this->apiUrl + String(INSERT_CONTEXT));
        this -> http.addHeader("Content-Type", "application/json");
        this -> http.setAuthorization(this -> dbUser.c_str(), this -> dbPass.c_str());
        do {
            n_attemp++;
            Serial.println(bodyRequest);
            httpCode = http.POST(bodyRequest);
            String httpMessage = this -> http.getString();
            httpMessage = httpMessage.substring(12, httpMessage.length()-2);
            log_message = "Inserting on table: " + table + " - http message: <" + httpMessage + "> - on inserting frame";
            Serial.print("HTTP Response code: ");
            Serial.print(httpCode);
            Serial.print(" on attemp number ");
            Serial.println(n_attemp);
        } while (n_attemp > attempts && httpCode != 201);
        this -> http.end();
        Serial.printf("ERROR: %s\n - Reattempting...",log_message);
        this -> setResponseCode(httpCode);
        this -> setDebugLevel( httpCode == 201 ? INFO_LEVEL : ERROR_LEVEL );
    } catch (const std::exception& e) {
        log_message = e.what();
        this -> setResponseCode(httpCode);
        this -> setDebugLevel(ERROR_LEVEL);
        this -> http.end();
    }
    
    return log_message + "http response code: " + String(httpCode) + " on attempts: " + String(n_attemp);
}

String RestCall::getWeightAndRain(String command) {
    HTTPClient http;
    String wetweightAndRainValues = "";
    this -> http.clearAllCookies();
    this -> http.begin(this->apiUrl + String(ETCRAIN_CONTEXT));
    this -> http.addHeader("Content-Type", "application/json");
    this -> http.setAuthorization(this -> dbUser.c_str(), this -> dbPass.c_str());
    
    int httpCode = this -> http.GET();

    if (httpCode == 200) {
        String responseBody = http.getString();
        this -> http.end();
        Serial.print("Query received: ");
        Serial.println(responseBody);
        const size_t capacity = JSON_OBJECT_SIZE(2) + 40;
        DynamicJsonDocument doc(capacity);
        deserializeJson(doc, responseBody);
        double wetweight = doc["wetweight"];
        double cumulative_rain = doc["cumulative_rain"];
        wetweightAndRainValues = String(wetweight, 2) + ";" + String(cumulative_rain, 2);
        Serial.println(wetweightAndRainValues);
    }
    this -> setResponseCode(httpCode);
    this -> setDebugLevel( httpCode == 200 ? INFO_LEVEL : ERROR_LEVEL );
    return command + ";" + wetweightAndRainValues + ";";
}