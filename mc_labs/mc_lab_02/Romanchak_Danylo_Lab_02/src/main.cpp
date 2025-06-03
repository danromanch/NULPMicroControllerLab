#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#define SSID "ESP8266_Server"
#define PASSWORD "12345678"
#define ST1 100
#define ST2 1000
#define HOLD_TIME 500
#define STEP_INCREMENT(x) x > ST1 ? x -= ST1 : x = ST2

typedef struct led_s {
    unsigned short pin;
    unsigned short status;
    led_s* nextLed;
}led_t;

typedef struct button_s {
    unsigned short pin;
    unsigned short status;
    unsigned short previousStatus;
    uint32_t pressStart;
}button_t;

led_t ledDBlue = {14, LOW, nullptr};
led_t ledYellow = {12, LOW, nullptr};
led_t ledRed = {13, LOW, nullptr};
button_t button = {0, LOW, LOW, 0};
button_t buttonNazar = {0, LOW, LOW, 0};
led_t* currentLed = nullptr;
uint32_t lastStepTime = 0;
unsigned short stepTime = 1000;
uint32_t lastButtonTime = 0;
unsigned short debounceTime = 50;
AsyncWebServer server(80);

void ledSetup() {
    ledDBlue.nextLed = &ledYellow;
    ledYellow.nextLed = &ledRed;
    ledRed.nextLed = &ledDBlue;
    currentLed = &ledRed;
}

void pinSetup() {
    pinMode(ledDBlue.pin, OUTPUT);
    pinMode(ledYellow.pin, OUTPUT);
    pinMode(ledRed.pin, OUTPUT);
    pinMode(button.pin, INPUT);
}

void lightLED() {
    currentLed->status = LOW;
    digitalWrite(currentLed->pin, currentLed->status);
    currentLed = currentLed->nextLed;
    currentLed->status = HIGH;
    digitalWrite(currentLed->pin, currentLed->status);
    lastStepTime = millis();
}

void handlerLed() {
    led_t* startLed = &ledDBlue;
    if (millis() - lastStepTime > debounceTime) {
        while (startLed->nextLed !=nullptr) {
            currentLed->status = HIGH;
            digitalWrite(startLed->pin, currentLed->status);
            currentLed->status = LOW;
            digitalWrite(startLed->pin, currentLed->status);
            startLed = startLed->nextLed;
        }
    }
}

void releaseEndpoint(AsyncWebServerRequest *request){
    STEP_INCREMENT(stepTime);
    request->send_P(200, "text/html", "ok");
}

void pressNazar(AsyncWebServerRequest *request) {
    buttonNazar.status = HIGH;
    request->send_P(200, "text/html", "ok");
}

void releaseNazar(AsyncWebServerRequest *request) {
    buttonNazar.status = LOW;
    request->send_P(200, "text/html", "ok");
}


void ledStatusEndpoint(AsyncWebServerRequest *request) {
    String json = "{";
    json += "\"blue\": " + String(ledDBlue.status == HIGH ? "true" : "false") + ",";
    json += "\"yellow\": " + String(ledYellow.status == HIGH ? "true" : "false") + ",";
    json += "\"red\": " + String(ledRed.status == HIGH ? "true" : "false");
    json += "}";

    request->send(200, "application/json", json);
}


void wifiSetup() {
    WiFi.begin(SSID, PASSWORD);
    LittleFS.begin();
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
    server.on("/release", HTTP_GET, releaseEndpoint);
    server.on("/pressNazar", HTTP_GET, pressNazar);
    server.on("/releaseNazar", HTTP_GET, releaseNazar);
    server.on("/ledstatus", HTTP_GET, ledStatusEndpoint);
    server.begin();
}

void setup() {
    Serial.begin(115200, SERIAL_8N1);
    pinSetup();
    ledSetup();
    wifiSetup();
}

void send() {
    if (buttonNazar.status == HIGH && buttonNazar.previousStatus == LOW) {
        buttonNazar.previousStatus = HIGH;
        buttonNazar.pressStart = millis();
    } else if (buttonNazar.status == HIGH && buttonNazar.previousStatus == HIGH) {
        if (millis() - buttonNazar.pressStart > HOLD_TIME) {
            Serial.print("k");
        }
    } else if (buttonNazar.status == LOW && buttonNazar.previousStatus == HIGH) {
        buttonNazar.previousStatus = LOW;
    }
}

void receive() {
    if (Serial.available()) {
        uint8_t reader = Serial.read();
        Serial.println(reader);
        if (reader == 'k') {
            STEP_INCREMENT(stepTime);
        }
    }
}

void loop() {
    send();
    receive();
    if (millis() - lastStepTime > stepTime) {
        lightLED();
    }
    button.status = digitalRead(button.pin);
    if (millis() - lastButtonTime > debounceTime) {
        if (button.status == HIGH && button.previousStatus == LOW) {
            button.previousStatus = button.status;
            STEP_INCREMENT(stepTime);
            lastButtonTime = millis();
        } else if (button.status == LOW && button.previousStatus == HIGH) {
            button.previousStatus = button.status;
        }
    }
}
