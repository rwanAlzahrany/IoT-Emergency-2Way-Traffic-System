// ================= TRAFFIC CONTROLLER NODEMCU =================
// 2-WAY Traffic Light (North–South)
// Density-Based Control using IR Sensors
// Emergency Override + ThingSpeak Upload (FINAL VERSION)

// -------------------------------------------------------------

#include <ESP8266WiFi.h>
#include <espnow.h>

// ---------------- WIFI SETTINGS ----------------
const char* WIFI_SSID = "soso";
const char* WIFI_PASS = "Nesreen2004Soso";

// ---------------- THINGSPEAK SETTINGS ----------------
const char* TS_HOST = "api.thingspeak.com";
String TS_WRITE_KEY = "MW4I92HSGZ5QAV5Z";

// ---------------- TRAFFIC LIGHT PINS ----------------
// NORTH
const int N_RED    = D1;
const int N_YELLOW = D2;
const int N_GREEN  = D5;

// SOUTH
const int S_RED    = D6;
const int S_YELLOW = D7;
const int S_GREEN  = D3;

// ---------------- IR SENSOR PINS ----------------
const int IR_N = D0;   // North density sensor
const int IR_S = D4;   // South density sensor 

// ---------------- TIMING (ms) ----------------
const unsigned long GREEN_TIME    = 3000;
const unsigned long YELLOW_TIME   = 1500;
const unsigned long DENSITY_TIME  = 2000;
const unsigned long DENSITY_EXTRA = 4000;

// ---------------- STATE ----------------
bool emergencyActive = false;
bool northTurn = true;
bool inYellow = false;

unsigned long phaseStart = 0;
unsigned long phaseDuration = 0;

// ---------------- DENSITY STATE ----------------
unsigned long northBlockedSince = 0;
unsigned long southBlockedSince = 0;

bool northDense = false;
bool southDense = false;

// ---------------- THINGSPEAK EXTRA DATA ----------------
// field3, field4, field5
int currentGreenDirection = 0;      // 0 = North, 1 = South
unsigned long currentGreenTime = 0; // ms
int systemMode = 0;                 // 0=NORMAL, 1=DENSE, 2=EMERGENCY

// ---------------- ESP-NOW STRUCT ----------------
typedef struct {
  int emergency;
} messageData;

// ---------------- ESP-NOW RECEIVE ----------------
void onDataRecv(uint8_t *mac, uint8_t *data, uint8_t len) {
  messageData msg;
  memcpy(&msg, data, sizeof(msg));
  emergencyActive = (msg.emergency == 1);

  Serial.print("🚨 Emergency received: ");
  Serial.println(emergencyActive);
}

// ---------------- WIFI CONNECT ----------------
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n✔ WiFi Connected");
  Serial.println(WiFi.localIP());
}

// ---------------- THINGSPEAK UPLOAD ----------------
unsigned long lastUpload = 0;

void uploadThingSpeak() {
  if (millis() - lastUpload < 20000) return;
  lastUpload = millis();

  WiFiClient client;
  if (!client.connect(TS_HOST, 80)) {
    Serial.println("❌ ThingSpeak connection failed");
    return;
  }

  String postData = "api_key=" + TS_WRITE_KEY;

  // KEEP YOUR OLD DATA (field1 & field2)
  postData += "&field1=0";
  postData += "&field2=0";

  // NEW DATA
  postData += "&field3=" + String(currentGreenDirection);
  postData += "&field4=" + String(currentGreenTime);
  postData += "&field5=" + String(systemMode);

  Serial.println("========== THINGSPEAK UPLOAD ==========");
  Serial.print("Green Direction : ");
  Serial.println(currentGreenDirection == 0 ? "NORTH" : "SOUTH");

  Serial.print("Green Time (ms) : ");
  Serial.println(currentGreenTime);

  Serial.print("System Mode     : ");
  if (systemMode == 0) Serial.println("NORMAL");
  else if (systemMode == 1) Serial.println("DENSE");
  else Serial.println("EMERGENCY");

  client.println("POST /update HTTP/1.1");
  client.println("Host: api.thingspeak.com");
  client.println("Connection: close");
  client.println("Content-Type: application/x-www-form-urlencoded");
  client.print("Content-Length: ");
  client.println(postData.length());
  client.println();
  client.print(postData);

  client.stop();
  Serial.println("📊 ThingSpeak updated");
  Serial.println("======================================");
}

// ---------------- LIGHT CONTROL ----------------
void setNorthGreen() {
  digitalWrite(N_GREEN, HIGH);
  digitalWrite(N_YELLOW, LOW);
  digitalWrite(N_RED, LOW);

  digitalWrite(S_GREEN, LOW);
  digitalWrite(S_YELLOW, LOW);
  digitalWrite(S_RED, HIGH);

  currentGreenDirection = 0;
}

void setSouthGreen() {
  digitalWrite(S_GREEN, HIGH);
  digitalWrite(S_YELLOW, LOW);
  digitalWrite(S_RED, LOW);

  digitalWrite(N_GREEN, LOW);
  digitalWrite(N_YELLOW, LOW);
  digitalWrite(N_RED, HIGH);

  currentGreenDirection = 1;
}

void setNorthYellow() {
  digitalWrite(N_GREEN, LOW);
  digitalWrite(N_YELLOW, HIGH);
}

void setSouthYellow() {
  digitalWrite(S_GREEN, LOW);
  digitalWrite(S_YELLOW, HIGH);
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);

  pinMode(N_RED, OUTPUT);
  pinMode(N_YELLOW, OUTPUT);
  pinMode(N_GREEN, OUTPUT);
  pinMode(S_RED, OUTPUT);
  pinMode(S_YELLOW, OUTPUT);
  pinMode(S_GREEN, OUTPUT);

  pinMode(IR_N, INPUT_PULLUP);
  pinMode(IR_S, INPUT_PULLUP);

  connectWiFi();

  if (esp_now_init() != 0) {
    Serial.println("❌ ESP-NOW init failed");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(onDataRecv);

  setNorthGreen();
  currentGreenTime = GREEN_TIME;
  systemMode = 0;

  phaseStart = millis();
  phaseDuration = GREEN_TIME;

  Serial.println("🚦 Traffic Controller READY");
}

// ---------------- LOOP ----------------
void loop() {

  // ---------- DENSITY DETECTION ----------
  bool irN = digitalRead(IR_N);
  bool irS = digitalRead(IR_S);

  if (irN == LOW) {
    if (northBlockedSince == 0) northBlockedSince = millis();
    if (millis() - northBlockedSince >= DENSITY_TIME) northDense = true;
  } else {
    northBlockedSince = 0;
    northDense = false;
  }

  if (irS == LOW) {
    if (southBlockedSince == 0) southBlockedSince = millis();
    if (millis() - southBlockedSince >= DENSITY_TIME) southDense = true;
  } else {
    southBlockedSince = 0;
    southDense = false;
  }

  uploadThingSpeak();

  // ---------- EMERGENCY MODE ----------
  if (emergencyActive) {
    systemMode = 2;
    currentGreenTime = GREEN_TIME;
    setNorthGreen();
    return;
  }

  // ---------- TRAFFIC PHASE CONTROL ----------
  unsigned long now = millis();
  if (now - phaseStart < phaseDuration) return;

  phaseStart = now;

  if (!inYellow) {
    inYellow = true;
    phaseDuration = YELLOW_TIME;
    if (northTurn) setNorthYellow();
    else setSouthYellow();
  } 
  else {
    inYellow = false;
    northTurn = !northTurn;

    if (northTurn) {
      setNorthGreen();
      currentGreenTime = GREEN_TIME + (northDense ? DENSITY_EXTRA : 0);
      systemMode = northDense ? 1 : 0;
      phaseDuration = currentGreenTime;
    } else {
      setSouthGreen();
      currentGreenTime = GREEN_TIME + (southDense ? DENSITY_EXTRA : 0);
      systemMode = southDense ? 1 : 0;
      phaseDuration = currentGreenTime;
    }
  }
}
