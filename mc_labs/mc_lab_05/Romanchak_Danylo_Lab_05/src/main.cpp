#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include "WiFiManager.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>

#define MOTION_PIN D4
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET     -1
#define SCREEN_ADDRESS 0x3C

const char* endpoint = "http://pouncing-efficacious-cobbler.glitch.me/send";
unsigned long lastMotionTime = 0;
const unsigned long motionInterval = 4000;

String emailRecipient = "";
const char* emailSubject = "Motion Alert";
const char* emailText = "Motion was detected by your ESP8266 sensor!";

ESP8266WebServer server(80);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

unsigned long lastDisplayRefresh = 0;
const unsigned long displayRefreshInterval = 10000;
bool showingIpMessage = true;

void displayMessage(String line1, String line2 = "", bool isIpMessage = false) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(line1);
  display.setCursor(0, 16);
  display.println(line2);
  display.display();

  showingIpMessage = isIpMessage;
}

void displayIP() {
  String ipStr = WiFi.localIP().toString();
  displayMessage("IP: " + ipStr, "Motion Sensor Ready", true);
}

void handleRoot() {
  String html = "<html><body><form action='/email' method='POST'>"
                "<label for='email'>Enter your email:</label><br>"
                "<input type='email' id='email' name='email' required><br><br>"
                "<input type='submit' value='Submit'>"
                "</form></body></html>";
  server.send(200, "text/html", html);
}

void handleEmail() {
  String email = "";
  if (server.hasArg("email")) {
    email = server.arg("email");
    emailRecipient = email;
  }
  server.send(200, "text/html", "<html><body>Thank you! Your email was received.<br><a href='/'>Back</a></body></html>");
}

void setup() {
  pinMode(MOTION_PIN, INPUT);

  Wire.begin(D2, D1);  // SDA=D2 (GPIO4), SCL=D1 (GPIO5)

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
  } else {
    displayMessage("Starting up...", "Initializing WiFi");
    delay(1000);
  }

  WiFiManager wifiManager;
  wifiManager.setDebugOutput(true);
  wifiManager.setConfigPortalTimeout(180);

  wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
  wifiManager.setMinimumSignalQuality(30);

  displayMessage("Connecting to WiFi", "Please wait...");

  if (!wifiManager.autoConnect("MotionSensor-Setup")) {
    displayMessage("WiFi Connection", "Failed! Restarting...");
    delay(3000);
    ESP.restart();
    delay(5000);
  }

  displayIP();

  server.on("/", HTTP_GET, handleRoot);
  server.on("/email", HTTP_POST, handleEmail);
  server.begin();
}

void loop() {
  server.handleClient();

  int motion = digitalRead(MOTION_PIN);
  unsigned long now = millis();
  if (motion == HIGH && (now - lastMotionTime > motionInterval)) {

    displayMessage("Motion Detected!", "Sending alert...");

    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      WiFiClient client;
      http.begin(client, endpoint);
      http.addHeader("Content-Type", "application/json"); // Specify JSON content type

      StaticJsonDocument<200> jsonDoc;
      jsonDoc["to"] = emailRecipient;
      jsonDoc["subject"] = emailSubject;
      jsonDoc["text"] = emailText;
      String requestBody;
      serializeJson(jsonDoc, requestBody);

      int httpCode = http.POST(requestBody);


      if (httpCode > 0) {
        displayMessage("Motion Alert Sent!", "HTTP code: " + String(httpCode));
      } else {
        displayMessage("Failed to send", "Error: " + String(httpCode));
      }

      http.end();
    } else {
      displayMessage("Motion Detected!", "WiFi not connected");
    }
    lastMotionTime = now;
  }

  static unsigned long lastNoMotionPrint = 0;
  if (motion == LOW) {
    unsigned long currentMillis = millis();
    if (currentMillis - lastNoMotionPrint > 1000) {
      lastNoMotionPrint = currentMillis;
    }
  }

  unsigned long currentMillis = millis();
  if (currentMillis - lastDisplayRefresh > displayRefreshInterval) {
    lastDisplayRefresh = currentMillis;
    if (showingIpMessage) {
      displayIP();
    } else {
      String ipStr = WiFi.localIP().toString();
      displayMessage(ipStr);
    }
  }
}