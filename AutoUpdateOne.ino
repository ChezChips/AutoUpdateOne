#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClientSecure.h>

// ---------------------------
// WiFi Login
// ---------------------------
const char* ssid = "Verizon-791L-17DB";
const char* password = "6ddcb3d3";

// ---------------------------
// GitHub URLs for YOUR repo
// ---------------------------

// Checks for latest commit
const char* github_api =
  "https://api.github.com/repos/ChezChips/AutoUpdateOne/commits/main";

// Downloads firmware.bin
const char* firmware_url =
  "https://raw.githubusercontent.com/ChezChips/AutoUpdateOne/main/firmware.bin";

// ---------------------------
// Update timing
// ---------------------------
unsigned long lastCheck = 0;
const unsigned long checkInterval = 10000; // 10 seconds

String lastCommit = "";
WiFiClientSecure client;

// ---------------------------
// LED pin (NodeMCU built‑in LED = D4 = GPIO2)
// ---------------------------
const int ledPin = 2;

void setup() {
  Serial.begin(115200);

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH); // LED off (inverted logic)

  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected!");
  client.setInsecure(); // GitHub uses HTTPS
}

void loop() {
  // ---------------------------
  // Your LED behavior
  // ---------------------------
  digitalWrite(ledPin, LOW);   // LED ON
  delay(1000);                 // 5 seconds
  digitalWrite(ledPin, HIGH);  // LED OFF
  delay(1000);

  // ---------------------------
  // Auto-update check
  // ---------------------------
  unsigned long now = millis();
  if (now - lastCheck >= checkInterval) {
    lastCheck = now;
    checkGitHub();
  }
}

void checkGitHub() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient https;
  Serial.println("Checking GitHub for updates...");

  if (https.begin(client, github_api)) {
    https.addHeader("User-Agent", "ESP8266");
    int httpCode = https.GET();

    if (httpCode == HTTP_CODE_OK) {
      String payload = https.getString();

      int shaIndex = payload.indexOf("\"sha\":\"");
      if (shaIndex > 0) {
        String sha = payload.substring(shaIndex + 7, shaIndex + 47);

        if (sha != lastCommit) {
          Serial.println("New firmware detected!");
          Serial.println("Commit SHA: " + sha);

          lastCommit = sha;
          performOTA();
        } else {
          Serial.println("No new updates.");
        }
      }
    } else {
      Serial.printf("GitHub API error: %d\n", httpCode);
    }

    https.end();
  }
}

void performOTA() {
  Serial.println("Starting OTA update...");

  WiFiClientSecure updateClient;
  updateClient.setInsecure();

  t_httpUpdate_return ret = ESPhttpUpdate.update(updateClient, firmware_url);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("OTA failed: %s\n",
        ESPhttpUpdate.getLastErrorString().c_str());
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("No update available.");
      break;

    case HTTP_UPDATE_OK:
      Serial.println("Update successful! Rebooting...");
      break;
  }
}
