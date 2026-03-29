// ==================== EMERGENCY VEHICLE UNIT (ESP8266) ====================
// BUTTON + BUZZER + IFTTT GMAIL + ESP-NOW SENDER + WIFI TEST (NON-BLOCKING)

#include <ESP8266WiFi.h>
#include <espnow.h>

// ---------------- WIFI SETTINGS ----------------
const char* WIFI_SSID = "soso";
const char* WIFI_PASS = "Nesreen2004Soso";

// ---------------- IFTTT SETTINGS ----------------
String IFTTT_KEY   = "c76Gu_5_KIF83VxPTpA_s0";
String IFTTT_EVENT = "emergency_trigger";

// ---------------- BUTTON + BUZZER ----------------
const int buttonPin = D1;
const int buzzerPin = D2;

bool emergencyActive = false;
bool emailSent = false;
int lastButton = HIGH;

// ------------- TRAFFIC CONTROLLER MAC ADDRESS -------------
uint8_t trafficMAC[] = {0x50, 0x02, 0x91, 0xDF, 0xAA, 0x56};

// ------------ ESP-NOW MESSAGE STRUCTURE ------------
typedef struct {
  int emergency;
} messageData;

messageData msg;

// ------------ WIFI CONNECT FUNCTION ------------
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("Connecting to WiFi");
  int tries = 0;

  while (WiFi.status() != WL_CONNECTED && tries < 20) {
    delay(500);
    Serial.print(".");
    tries++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("✔ WiFi Connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("✘ WiFi FAILED.");
  }
}

// ------------ WIFI STATUS TEST (EVERY 5 SECONDS) ------------
unsigned long lastWiFiTest = 0;
void testWiFiStatus() {
  if (millis() - lastWiFiTest > 5000) {
    lastWiFiTest = millis();
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("[WiFi STATUS] Connected! IP: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("[WiFi STATUS] NOT connected!");
    }
  }
}

// ------------ SEND EMAIL ALERT USING IFTTT (NON-BLOCKING) ------------
void sendIFTTT() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ WiFi not connected, cannot send IFTTT");
    return;
  }

  WiFiClient client;
  String body = "{\"value1\":\"EMERGENCY ON\"}";
  String url = "/trigger/" + IFTTT_EVENT + "/with/key/" + IFTTT_KEY;

  Serial.println("🌐 Sending IFTTT request...");

  if (!client.connect("maker.ifttt.com", 80)) {
    Serial.println("❌ IFTTT Connection Failed!");
    return;
  }

  client.println("POST " + url + " HTTP/1.1");
  client.println("Host: maker.ifttt.com");
  client.println("Content-Type: application/json");
  client.println("Content-Length: " + String(body.length()));
  client.println("Connection: close");
  client.println();
  client.println(body);

  client.stop();  // 🔥 DO NOT WAIT
  Serial.println("📧 IFTTT trigger sent");
}

// ------------ SEND EMERGENCY STATE VIA ESP-NOW ------------
void sendEmergencyState() {
  msg.emergency = emergencyActive ? 1 : 0;
  esp_now_send(trafficMAC, (uint8_t*)&msg, sizeof(msg));
}

// ----------------------- SETUP ------------------------
void setup() {
  Serial.begin(115200);

  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);

  connectWiFi();

  if (esp_now_init() != 0) {
    Serial.println("❌ ESP-NOW init failed!");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_add_peer(trafficMAC, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);

  Serial.println("🚑 Emergency Vehicle Ready.");
}

// ------------------------ LOOP ------------------------
void loop() {

  testWiFiStatus();

  int reading = digitalRead(buttonPin);

  // Toggle emergency mode on button press
  if (reading == LOW && lastButton == HIGH) {
    emergencyActive = !emergencyActive;

    if (emergencyActive) {
      digitalWrite(buzzerPin, HIGH);

      if (!emailSent) {
        sendIFTTT();      // SEND EMAIL ONCE
        emailSent = true;
      }

    } else {
      digitalWrite(buzzerPin, LOW);
      emailSent = false;  // allow email next time
    }

    sendEmergencyState();
    delay(300); // debounce
  }

  lastButton = reading;
}
