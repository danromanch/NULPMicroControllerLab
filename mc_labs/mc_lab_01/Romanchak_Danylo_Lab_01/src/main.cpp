#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#define SSID "Net"
#define PASSWORD "11111112"
#define ST1 100
#define ST2 1000
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
}button_t;

led_t ledDBlue = {14, LOW, nullptr};
led_t ledYellow = {12, LOW, nullptr};
led_t ledRed = {13, LOW, nullptr};
button_t button = {0, LOW, LOW};
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

void releaseEndpoint(AsyncWebServerRequest *request){
    STEP_INCREMENT(stepTime);
    request->send_P(200, "text/html", "ok");
}

void wifiSetup() {
    WiFi.begin(SSID, PASSWORD);
    LittleFS.begin();
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
    server.on("/release", HTTP_GET, releaseEndpoint);
    server.begin();
}

void setup() {
    pinSetup();
    ledSetup();
    wifiSetup();
}

void loop() {
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
