#include <BleKeyboard.h>

// --------- Board-specific defaults ---------
#ifdef ARDUINO
  #include "sdkconfig.h"
#endif

// Default LED pin per target; change if your board differs.
#if defined(CONFIG_IDF_TARGET_ESP32S3)
  int LED_PIN = 38;      // many S3 dev boards use 38 for the white LED
#else
  int LED_PIN = 2;       // many ESP32 Dev Modules use GPIO2 (or have no LED)
#endif

// Button pin (momentary to GND, internal pull-up enabled)
int BTN_PIN = 4;
// -------------------------------------------

static const char*  DEVICE_NAME   = "ESP32 KB";
static const uint16_t BATTERY_PCT = 100;

const uint32_t SETTLE_MS  = 3000;  // wait after connect before first send
const uint32_t MIN_GAP_MS = 2000;  // min time between sends (debounce/throttle)
const uint16_t KEY_GAP_MS = 12;    // per-character pacing

BleKeyboard kb(DEVICE_NAME, "Espressif", BATTERY_PCT);

bool wasConnected   = false;
bool readyToSend    = false;
unsigned long connectedAt = 0;
unsigned long lastSendAt  = 0;

bool lastBtn = true;  // pull-up: HIGH = released

bool buttonPressed() {
  bool now = digitalRead(BTN_PIN);
  static unsigned long lastFlip = 0;
  if (now != lastBtn) { lastFlip = millis(); lastBtn = now; }
  if (!now && (millis() - lastFlip) > 30) { // stable LOW 30ms
    while (!digitalRead(BTN_PIN)) { delay(2); }  // wait for release
    delay(20);
    return true;
  }
  return false;
}

void sendText(const char* s) {
  Serial.print("[SEND] "); Serial.println(s);
  for (const char* p = s; *p; ++p) { kb.print(*p); delay(KEY_GAP_MS); }
  // No Enter by default (avoids toggling things in Settings windows)
  // kb.write(KEY_RETURN);
}

void setup() {
  pinMode(BTN_PIN, INPUT_PULLUP);

  // If your board has no LED, set LED_PIN = -1 to disable LED control.
  if (LED_PIN >= 0) { pinMode(LED_PIN, OUTPUT); digitalWrite(LED_PIN, LOW); }

  Serial.begin(115200);
  delay(100);
  Serial.println("\n=== BLE Keyboard: ESP32-S3 ↔ ESP32 Dev Module portable ===");
  Serial.printf("LED_PIN=%d  BTN_PIN=%d\n", LED_PIN, BTN_PIN);
  Serial.println("Starting kb.begin()...");
  kb.begin();
  Serial.println("kb.begin() done. Pair, focus a text field, then press the button.");
}

void loop() {
  const bool connected = kb.isConnected();

  // Rising edge: connected
  if (connected && !wasConnected) {
    Serial.println("[EVT] Connected");
    connectedAt = millis();
    readyToSend = false;
    if (LED_PIN >= 0) digitalWrite(LED_PIN, HIGH);
  }

  // Falling edge: disconnected
  if (!connected && wasConnected) {
    Serial.println("[EVT] Disconnected");
    if (LED_PIN >= 0) digitalWrite(LED_PIN, LOW);
    readyToSend = false;
  }

  wasConnected = connected;

  if (connected) {
    if (!readyToSend && (millis() - connectedAt) >= SETTLE_MS) {
      readyToSend = true;
      Serial.println("[INFO] Link settled. Waiting for button press...");
    }

    if (readyToSend && buttonPressed()) {
      const unsigned long now = millis();
      if (now - lastSendAt >= MIN_GAP_MS) {
        sendText("hello world");
        lastSendAt = now;
      } else {
        Serial.println("[SKIP] Throttled (too soon).");
      }
    }
  }

  delay(5);
}
