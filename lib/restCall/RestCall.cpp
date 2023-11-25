#include "RestCall.hpp"

void RestCall::config(String apiUrl, String dbUser, String dbPass) {
    this -> apiUrl = apiUrl;
    this -> dbUser = dbUser;
    this -> dbPass = dbPass;
    this -> http.setTimeout(60000);
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
            Serial.printf(" on attemp number %d. ", n_attemp);
            if(httpCode != 201) {
                Serial.printf("ERROR: %d\n - Reattempting...",httpCode);
            } else {
                Serial.println("Response: " + httpMessage);
            }
        } while (n_attemp <= attempts && httpCode != 201);
        this -> http.end();
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

String RestCall::getWeight(String command) {
    int n_attempt = 0;
    int attempts = 4;
    int httpCode;
    double wetweight = -1;
    String wetweightAndRainValues = "";

        this -> http.clearAllCookies();
        this -> http.begin(this->apiUrl + String(ETCRAIN_CONTEXT));
        this -> http.addHeader("Content-Type", "application/json");
        this -> http.setAuthorization(this -> dbUser.c_str(), this -> dbPass.c_str());

        do {
            n_attempt++;
            httpCode = this -> http.GET();
            Serial.printf("HTTP Response code: %d\n", httpCode);
          
            if (httpCode == 200) {
                String responseBody = this -> http.getString();
                Serial.print("Query received: ");
                Serial.println(responseBody);
                const size_t capacity = JSON_OBJECT_SIZE(1) + 40;
                DynamicJsonDocument doc(capacity);
                deserializeJson(doc, responseBody);
                wetweight = doc["wetweight"];
                Serial.println(wetweight);
            } else {
                Serial.printf("ERROR: %d - Reattempting...", httpCode);
            }
        } while (n_attempt <= attempts && httpCode != 200);

        this -> http.end();
        this -> setResponseCode(httpCode);
        this -> setDebugLevel(httpCode == 200 ? INFO_LEVEL : ERROR_LEVEL);
        
    return command + ";" + wetweight + ";";
}

String RestCall::ping(int attempts) {
    int n_attempt = 0;
    int httpCode = 0;
    this -> http.clearAllCookies();
    this -> http.begin(this->apiUrl + String(PING_CONTEXT));
    this -> http.addHeader("Content-Type", "application/json");
    this -> http.setAuthorization(this -> dbUser.c_str(), this -> dbPass.c_str());
    do {
        httpCode = this -> http.GET();
        n_attempt++;
    } while(n_attempt <= attempts && httpCode != 200);

}