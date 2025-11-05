#include <BleKeyboard.h>

// ---------- SETTINGS ----------
static const char*  DEVICE_NAME   = "ESP32 KB";
static const uint16_t BATTERY_PCT = 100;

// GPIO of a momentary button wired to GND (internal pull-up enabled).
// Pick one that exists on your board; 0 or 4 or 15 are common.
// Change this if needed.
const int BTN_PIN = 4;

// Wait this long after connect before we allow any send (lets CCCD finish)
const uint32_t SETTLE_MS   = 3000;
// Minimum time between message sends (debounce / host “breathing room”)
const uint32_t MIN_GAP_MS  = 2000;
// Delay between characters (slow + safe for old stacks)
const uint16_t KEY_GAP_MS  = 12;
// ------------------------------

BleKeyboard kb(DEVICE_NAME, "Espressif", BATTERY_PCT);

bool wasConnected   = false;
bool readyToSend    = false;
unsigned long connectedAt = 0;
unsigned long lastSendAt  = 0;

bool lastBtn = true;  // pull-up: HIGH = released

// Helper: rising-edge (press) with simple debounce
bool buttonPressed() {
  bool now = digitalRead(BTN_PIN);
  static unsigned long lastFlip = 0;
  if (now != lastBtn) {
    lastFlip = millis();
    lastBtn = now;
  }
  if (!now && (millis() - lastFlip) > 30) { // LOW and stable 30ms
    // wait for release to avoid repeats
    while (!digitalRead(BTN_PIN)) { delay(2); }
    delay(20);
    return true;
  }
  return false;
}

// Helper: send text safely, char-by-char
void sendText(const char* s) {
  Serial.print("[SEND] ");
  Serial.println(s);

  // Per-char pacing for maximum compatibility
  for (const char* p = s; *p; ++p) {
    kb.print(*p);
    delay(KEY_GAP_MS);
  }
  kb.write(KEY_RETURN); // newline
}

void setup() {
  pinMode(BTN_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  Serial.begin(115200);
  delay(100);
  Serial.println("\n=== BLE Keyboard: basic, on-demand sending ===");
  Serial.println("Starting kb.begin()...");
  kb.begin();
  Serial.println("kb.begin() done. Pair the host, focus a text field, then press the button.");
}

void loop() {
  const bool connected = kb.isConnected();

  // Edge: connected
  if (connected && !wasConnected) {
    Serial.println("[EVT] Connected");
    connectedAt = millis();
    readyToSend = false;
    digitalWrite(LED_BUILTIN, HIGH);
  }

  // Edge: disconnected
  if (!connected && wasConnected) {
    Serial.println("[EVT] Disconnected");
    digitalWrite(LED_BUILTIN, LOW);
    readyToSend = false;
  }

  wasConnected = connected;

  if (connected) {
    // Arm after settle period
    if (!readyToSend && (millis() - connectedAt) >= SETTLE_MS) {
      readyToSend = true;
      Serial.println("[INFO] Link settled. Waiting for button press...");
    }

    // On-demand send
    if (readyToSend && buttonPressed()) {
      const unsigned long now = millis();
      if (now - lastSendAt >= MIN_GAP_MS) {
        sendText("hello world");
        lastSendAt = now;
      } else {
        Serial.println("[SKIP] Too soon since last send; throttled.");
      }
    }
  }

  delay(5);
}
