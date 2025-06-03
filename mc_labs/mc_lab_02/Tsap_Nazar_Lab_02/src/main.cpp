#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

#define BUTTON_PIN 13  
#define L1_PIN 0 //red led
#define L2_PIN 14 //blue led
#define L3_PIN 12 // green led
#define STEP_TIME 500 
#define HOLD_TIME 500

#define ON(pin)  ((pin) == 0 ? LOW : HIGH)
#define OFF(pin) ((pin) == 0 ? HIGH : LOW)
#define CAN_SWITCH() (millis() - lastChangeTime >= STEP_TIME) ? true : false

AsyncWebServer server(80);

typedef struct led_s{
  unsigned short pin;
  unsigned short status;
  led_s* next;
} led_t;

typedef struct button_s {
  unsigned short pin;
  unsigned short status;
  unsigned short previousStatus;
  uint32_t pressStartTime;
}button_t;

led_t l1 = {L1_PIN, false, nullptr};
led_t l2 = {L2_PIN, false, nullptr};  
led_t l3 = {L3_PIN, false, nullptr}; 
button_t button = {BUTTON_PIN, false, HIGH, 0};

unsigned short physicsBtnState = HIGH;
unsigned short webBtnState = HIGH;
unsigned short sendSignal = false;
unsigned short signalSent = false;
unsigned short algoState = false;

bool firstStart = true;
uint32_t lastChangeTime = 0;
unsigned long lastReceivedTime = 0;
led_t* currentLed = nullptr;

led_t* firstPattern[] = {&l3, &l2, &l1, &l2, &l3};
uint8_t firstStep = 0;


void pressEndpoint(AsyncWebServerRequest *request) {
  webBtnState = LOW;
  request->send(200, "text/plain", "ok");
}

void releaseAlgo1Endpoint(AsyncWebServerRequest *request) {
  webBtnState = HIGH;
  request->send(200, "text/plain", "ok");
}

void releaseAlgo2Endpoint(AsyncWebServerRequest *request) {
  sendSignal = true;
  request->send(200, "text/plain", "ok");
}


void ledStatusEndpoint(AsyncWebServerRequest *request) {
  String json = "{";
  json += "\"red\": " + String(l1.status == HIGH ? "false" : "true") + ",";
  json += "\"blue\": " + String(l2.status == HIGH ? "true" : "false") + ",";
  json += "\"green\": " + String(l3.status == HIGH ? "true" : "false");
  json += "}";

  request->send(200, "application/json", json);
}

void wifiSetup(){
  WiFi.softAP("ESP8266_Server", "12345678");
  Serial.println("Access Point Started");
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);


  LittleFS.begin();
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
  server.on("/press", HTTP_GET, pressEndpoint);

  server.on("/releaseAlgo1", HTTP_GET, releaseAlgo1Endpoint);

  server.on("/releaseAlgo2", HTTP_GET, releaseAlgo2Endpoint);

  server.on("/status", HTTP_GET, ledStatusEndpoint);

  server.begin();
}


void ledSetup() {
  l1.next = &l2;
  l2.next = &l3;
  l3.next = &l1;
  currentLed = &l3;
  l1.status = HIGH; 
  digitalWrite(l1.pin, l1.status); 
}

void pinSetup(){
  pinMode(BUTTON_PIN, INPUT);
  pinMode(l1.pin, OUTPUT);
  pinMode(l2.pin, OUTPUT);
  pinMode(l3.pin, OUTPUT);
}

void handleUnhold(){
  currentLed->status = OFF(currentLed->pin);
  digitalWrite(currentLed->pin, currentLed->status);
  currentLed = &l3;
  firstStart = !firstStart;
  firstStep = 0;
}

void sendUART(){
  if (sendSignal){
    Serial.print("k");
    sendSignal = false;
  }
}

void receiveUART() {
  if (Serial.available()) {
    uint8_t received = Serial.read();
    if (received == 'k') {
      algoState = true;
      lastReceivedTime = millis();
    }
  } 

  if (algoState && (millis() - lastReceivedTime > 500)) {
    handleUnhold();
    algoState = false;
  }
}

void switchLed() {
  if (currentLed) {
    currentLed->status = OFF(currentLed->pin);
    digitalWrite(currentLed->pin, currentLed->status);
  }

  if (firstStart) {
    currentLed = firstPattern[firstStep++];
    firstStep %= 5;
  } else {
    currentLed = currentLed->next;
  }

  currentLed->status = ON(currentLed->pin);
  digitalWrite(currentLed->pin, currentLed->status);
}

unsigned short checkHolding(button_t *btn){
  if (btn->status && btn->previousStatus == HIGH) {
    btn->previousStatus = LOW;
    btn->pressStartTime = millis();
  }
  else if (!btn->status && btn->previousStatus == LOW){
    btn->previousStatus = HIGH;
    handleUnhold();
  }
  else if (btn->status && btn->previousStatus == LOW && (millis() - btn->pressStartTime >= HOLD_TIME)){
    return true;
  }

  return false;
}

void setup() {
  Serial.begin(115200, SERIAL_8N1);
  pinSetup();
  ledSetup();
  wifiSetup();
}

void loop() {
  sendUART();
  receiveUART(); 
  physicsBtnState = !digitalRead(BUTTON_PIN);
  Serial.println(physicsBtnState);
  button.status = !(physicsBtnState && webBtnState);
  if (checkHolding(&button) || algoState){
    if (CAN_SWITCH()){
      switchLed();
      lastChangeTime = millis();
    }
  } 
}
