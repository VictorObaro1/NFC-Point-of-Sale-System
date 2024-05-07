//#include "WiFi.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>


//#include "Arduino.h"
//#include "FS.h"
//#include "LITTLEFS.h"
//#include "LittleFS.h"
//#include <HTTPClient.h>
#include <ArduinoJson.h>
WiFiClient client;
//#define FORMAT_LITTLEFS_IF_FAILED true
#define led 13
const char* ssid = "Nokia G20";
const char* password = "mypasscode50";
//const char* serverName = "https://www.nfc-payment-api.vercel.app/api/account/tx";
const char* serverName = "https://nfc-payment-api.vercel.app/api/account/tx";
String cardNumber;
String amount;
String transactionType;
String deviceId;
unsigned int count = 0;
bool flag;

unsigned long previousMillis = 0;
unsigned long interval = 30000;

StaticJsonDocument<200> doc1;

void postDataTOServer() {
  //Serial.println("Posting to server");
  if (WiFi.status() == WL_CONNECTED) {
    std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
    // Ignore SSL certificate validation
    client->setInsecure();
    //String requestBody;
    char requestBody[128] = {};
    serializeJson(doc1, requestBody);
    doc1.clear();
    HTTPClient https;
    https.begin(*client, serverName);
    https.addHeader("Content-Type", "application/json");
    int httpResponseCode = https.POST(requestBody);
    if (httpResponseCode > 0) {
      String response;
      response = https.getString();
      Serial.println(httpResponseCode);
      DeserializationError err = deserializeJson(doc1, response);
      serializeJson(doc1, Serial);
    }
    else {
      Serial.println(httpResponseCode);
      String str;
      str = "{\"statuscode\":400,\"message\":\"Unable to connect\"}";
      //int strLen = str.length() + 1;
      //char response [strLen];
      //str.toCharArray(response, strLen);
      //response = "{\"statuscode\":400,\"message\":\"An Error Occured\"}";
      DeserializationError err = deserializeJson(doc1, str);
      serializeJson(doc1, Serial);
    }
  }

  else if (WiFi.status() != WL_CONNECTED) {
    String response;
    response = "{\"statuscode\":400,\"message\":\"No WiFi\"}";
    DeserializationError errr = deserializeJson(doc1, response);
    serializeJson(doc1, Serial);
    doc1.clear();
  }
}

void setup()
{
  Serial.begin(115200);
  pinMode(led, OUTPUT);
  WiFi.disconnect(true);
  delay(500);

  WiFi.begin(ssid, password);

  if (WiFi.status() != WL_CONNECTED)
  {
    digitalWrite(led, LOW);
  }
}

void loop() {
  unsigned long currentMillis = millis();
  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >= interval)) {
    //Serial.print(millis());
    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    digitalWrite(led, LOW);
    WiFi.reconnect();
    previousMillis = currentMillis;
    flag = true;
  }

  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(led, HIGH);
    delay(2000);
    
    while (flag) {
      flag = false;
    }
  }
  DeserializationError error = deserializeJson(doc1, Serial);
  if (doc1["transactionType"] == "make" || doc1["transactionType"] == "receive") {
    postDataTOServer();
  }
}

