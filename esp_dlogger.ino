#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SoftwareSerial.h>
#include "FS.h"
#include "SD.h"

#define RO 16
#define DI 17

SoftwareSerial slave(RO, DI);

AsyncWebServer server(80);

// WiFi credentials
const char* ssid = "realme 6i";
const char* password = "12349876";

// Parameters for web server
const char* PARAM_DELAY = "delay";

// Delay variable to store delay value
unsigned long customDelay = 1000; // Default delay value

// HTML web page with delay parameter input field
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>ESP Input Form</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  </head><body>
  <form action="/get">
    <h1>Delay (ms) : <input type="text" name="delay"></h1>
    <input type="submit" value="Set Delay">
  </form>
</body></html>)rawliteral";

void setup() {
  Serial.begin(115200);

  // Initialize SD card
  if (!SD.begin(5)) {
    Serial.println("Card Mount Failed");
    return;
  }

  // Initialize Modbus
  slave.begin(9600);
  Serial.println("Slave node is ready");

  // Connect to WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi Failed!");
    return;
  }
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Configure web server routes
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request) {
    String delayValue;
    if (request->hasParam(PARAM_DELAY)) {
      delayValue = request->getParam(PARAM_DELAY)->value();
      customDelay = delayValue.toInt(); // Update custom delay value
      Serial.println("New delay value set: " + delayValue);
    }
    request->send(200, "text/html", "Delay parameter set to: " + delayValue);
  });

  server.onNotFound([](AsyncWebServerRequest *request){
    request->send(404, "text/plain", "Not found");
  });

  server.begin();
}

void loop() {
  if (slave.available()) {
    int pulse = slave.read();
    int bpm = map(pulse, 0, 1024, 50, 150); // Map the 10-bit sensor value to BPM range (50-150 BPM)
    Serial.println("Received BPM: " + String(bpm));

    // Write BPM data to SD card
    writeBPMDataToSDCard(bpm);
  }

  delay(customDelay); // Use custom delay value received from web server
}

void writeBPMDataToSDCard(int bpm) {
  // Open or create a file to append BPM data
  File file = SD.open("/bpm_data.txt", FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }

  // Append BPM data to the file
  file.println(bpm);

  // Close the file
  file.close();
}
