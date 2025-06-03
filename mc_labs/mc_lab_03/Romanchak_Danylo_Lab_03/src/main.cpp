#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define BUTTON_PIN D5
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const char* ssid = "Net";
const char* password = "11111112";
const char* serverURI = "http://eggplant-bristle-calendula.glitch.me";

ESP8266WebServer server(80);
String accessToken = "";
unsigned long lastNowPlayingFetch = 0;
const unsigned long nowPlayingInterval = 1000; // 1 second

unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;
int lastButtonState = HIGH;
int displayState = 0; // 0: Top5, 1: NowPlaying

void showMessage(const String& line1) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.cp437(true); // Enable full 256 char font
  display.println(line1);
  display.display();
  Serial.println(line1);
}

// === HTTP FETCH ===
void fetchAndDisplayTopTracks() {
  if (accessToken == "") return;

  HTTPClient http;
  WiFiClient client;
  http.begin(client, serverURI + String("/top5"));
  http.addHeader("Authorization", "Bearer " + accessToken);

  int httpCode = http.GET();
  if (httpCode == 200) {
    String payload = http.getString();
    DynamicJsonDocument doc(2048);
    deserializeJson(doc, payload);

    for (int i = 0; i < doc.size(); i++) {
      String name = doc[i]["name"].as<String>();
      String artist = doc[i]["artists"].as<String>();
      int rank = doc[i]["rank"].as<int>();

      unsigned long startMillis = millis();
      while (millis() - startMillis < 3000) {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.printf("TOP %d\n", rank);
        display.println(name);
        display.println("by:");
        display.println(artist);
        display.display();
        yield();
      }
    }
  } else {
    showMessage("Top5 HTTP error");
  }

  http.end();
}

void fetchAndDisplayNowPlaying() {
  HTTPClient http;
  WiFiClient client;
  http.begin(client, serverURI + String("/currently-playing"));
  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString();

    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);

    if (doc.containsKey("message")) {
      showMessage(doc["message"].as<String>());
      unsigned long startMillis = millis();
      while (millis() - startMillis < 3000) {
        yield();
      }
    } else {
      String name = doc["name"].as<String>();
      String artist = doc["artists"].as<String>();
      String album = doc["album"].as<String>();

      unsigned long startMillis = millis();
      while (millis() - startMillis < 3000) {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.println("Now Playing:");
        display.println(name);
        display.println("by:");
        display.println(artist);
        display.display();
        yield();
      }
    }
  } else {
    showMessage("NowPlaying Error");
  }

  http.end();
}

// === WEB SERVER HANDLERS ===
void handleRoot() {
  showMessage("Redirecting...");
  String html = "<html><body><h1>Redirecting to Spotify Login...</h1>"
                "<script>window.location='https://eggplant-bristle-calendula.glitch.me';</script></body></html>";
  server.send(200, "text/html", html);
}

void handlePage() {
  if (LittleFS.exists("/esp.html")) {
    File file = LittleFS.open("/esp.html", "r");
    String html = file.readString();
    file.close();
    html.replace("window.accessToken || \"\"", "\"" + accessToken + "\"");
    server.send(200, "text/html", html);
    showMessage("Page Served");
  } else {
    server.send(404, "text/plain", "Page not found");
    showMessage("Page Not Found");
  }
}

void handleCallback() {
  if (server.hasArg("access_token")) {
    accessToken = server.arg("access_token");
    Serial.println("Access Token: " + accessToken);
    showMessage("Token OK!");
    server.sendHeader("Location", "/page", true);
    server.send(302, "text/plain", "");
  } else {
    server.send(400, "text/html", "<html><body><h1>Failed to get access token</h1></body></html>");
    showMessage("Token Missing");
  }
}

void setup() {
  Serial.begin(115200);
  if (!LittleFS.begin()) {
    Serial.println("Failed to mount FS");
    return;
  }

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (true) {}
  }

  showMessage("Connecting WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  showMessage("Connected:\n" + WiFi.localIP().toString());

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  server.on("/", handleRoot);
  server.on("/page", handlePage);
  server.on("/callback", handleCallback);
  server.begin();
  Serial.println("Server started");
}

void loop() {
  server.handleClient();

  int buttonState = digitalRead(BUTTON_PIN);
  if (buttonState == LOW && lastButtonState == HIGH && (millis() - lastDebounceTime > debounceDelay)) {
    lastDebounceTime = millis();
    displayState = 1 - displayState;
    if (accessToken != "") {
      if (displayState == 0) {
        fetchAndDisplayTopTracks();
      } else {
        fetchAndDisplayNowPlaying();
        lastNowPlayingFetch = millis();
      }
    }
  }
  lastButtonState = buttonState;

  if (displayState == 1 && accessToken != "") {
    if (millis() - lastNowPlayingFetch >= nowPlayingInterval) {
      fetchAndDisplayNowPlaying();
      lastNowPlayingFetch = millis();
    }
  }
}
