/*
  ============================================================
  SMART IRRIGATION SYSTEM - Aloe Vera, Hibiscus, Croton
  ============================================================
  Hardware:
    - Arduino Nano
    - 3x Capacitive Soil Moisture Sensor (analog)
    - 4-Channel 5V Relay Module -> 3x 12V DC Solenoid Valve
    - 16x2 I2C LCD Display
    - LM2596 Buck Converter (12V -> 5V for Nano/relay/LCD)
    - 12V 5A DC Adapter (powers solenoids + buck converter)

  Wiring:
    Sensors (AOUT):
      Aloe     -> A0
      Hibiscus -> A1
      Croton   -> A2
      (VCC -> 5V, GND -> GND, all sensors)

    Relay module (IN pins):
      Aloe valve relay     -> D2
      Hibiscus valve relay -> D3
      Croton valve relay   -> D4
      (Relay VCC -> 5V, Relay GND -> common GND)
      (4th relay channel unused)

    LCD (I2C):
      SDA -> A4
      SCL -> A5
      VCC -> 5V, GND -> GND

    Power:
      12V adapter -> LM2596 IN, and -> Relay COM (switched) side for solenoids
      LM2596 OUT (trimmed to 5V) -> Arduino 5V pin, relay VCC, LCD VCC
      ALL GNDs (12V supply, buck converter, Arduino, relay, sensors) must be
      common/shared, or the relay switching will be unreliable.

  IMPORTANT - before uploading:
    1. Install library: "LiquidCrystal I2C" (by Frank de Brabander) via
       Library Manager. Wire.h is built in.
    2. Run an I2C scanner sketch once if the LCD shows nothing - your
       LCD's address may be 0x27 or 0x3F, not necessarily what's below.
    3. Check whether your relay module is ACTIVE LOW or ACTIVE HIGH
       (most cheap 4-channel modules are ACTIVE LOW - relay energizes
       when the IN pin is pulled LOW). Set RELAY_ACTIVE_LOW below to
       match your module. If pumps run backwards (on when should be
       off), flip this constant.
  ============================================================
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---------------- LCD CONFIG ----------------
// Change 0x27 to 0x3F if your LCD doesn't show text
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---------------- RELAY CONFIG ----------------
#define RELAY_ACTIVE_LOW true   // true = relay turns ON when pin is LOW (most common)

#if RELAY_ACTIVE_LOW
  #define RELAY_ON  LOW
  #define RELAY_OFF HIGH
#else
  #define RELAY_ON  HIGH
  #define RELAY_OFF LOW
#endif

// ---------------- PIN ASSIGNMENTS ----------------
const uint8_t SENSOR_PIN[3]  = { A0, A1, A2 };   // Aloe, Hibiscus, Croton
const uint8_t RELAY_PIN[3]   = { 2, 3, 4 };      // Aloe, Hibiscus, Croton
const char* PLANT_NAME[3]    = { "Aloe", "Hibiscus", "Croton" };

// ---------------- CALIBRATION (from your measured data) ----------------
// Raw ADC values: dry = higher number, wet = lower number
const int DRY_RAW[3] = { 320, 369, 587 };   // Aloe, Hibiscus, Croton
const int WET_RAW[3] = { 227, 231, 470 };   // Aloe, Hibiscus, Croton

// ---------------- WATERING THRESHOLDS (% moisture) ----------------
// Pump turns ON when moisture falls BELOW the "low" value.
// Pump turns OFF when moisture rises ABOVE the "high" value.
// Gap between low/high = hysteresis, prevents relay chattering.
// >>> TUNE THESE to your plants' actual needs once the system is running <<<
const int THRESH_LOW[3]  = { 20, 50, 40 };  // Aloe, Hibiscus, Croton - turn ON below this
const int THRESH_HIGH[3] = { 30, 65, 55 };  // Aloe, Hibiscus, Croton - turn OFF above this

// ---------------- SAFETY LIMIT ----------------
// Max time a valve is allowed to stay open continuously (ms).
// Protects against a stuck sensor reading or wiring fault causing flooding.
const unsigned long MAX_WATER_TIME = 30000UL; // 30 seconds

// ---------------- STATE ----------------
bool pumpState[3] = { false, false, false };
unsigned long pumpStartTime[3] = { 0, 0, 0 };
int moisturePercent[3] = { 0, 0, 0 };

unsigned long lastSensorRead = 0;
const unsigned long SENSOR_INTERVAL = 1000; // read sensors every 1s

unsigned long lastLcdSwitch = 0;
const unsigned long LCD_INTERVAL = 2000; // rotate LCD display every 2s
uint8_t lcdIndex = 0;

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(9600);

  for (uint8_t i = 0; i < 3; i++) {
    pinMode(RELAY_PIN[i], OUTPUT);
    digitalWrite(RELAY_PIN[i], RELAY_OFF);
  }

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Smart Irrigation");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");
  delay(1500);
  lcd.clear();
}

// ---------------- HELPER: read one sensor with light smoothing ----------------
int readSensorRaw(uint8_t pin) {
  long sum = 0;
  const uint8_t samples = 8;
  for (uint8_t i = 0; i < samples; i++) {
    sum += analogRead(pin);
    delay(5);
  }
  return sum / samples;
}

// ---------------- HELPER: convert raw ADC to moisture % ----------------
int rawToPercent(int raw, int dry, int wet) {
  // dry -> 0%, wet -> 100%
  long pct = map(raw, dry, wet, 0, 100);
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  return (int)pct;
}

// ---------------- MAIN LOOP ----------------
void loop() {
  unsigned long now = millis();

  // --- 1. Read sensors + update pump logic every SENSOR_INTERVAL ---
  if (now - lastSensorRead >= SENSOR_INTERVAL) {
    lastSensorRead = now;

    for (uint8_t i = 0; i < 3; i++) {
      int raw = readSensorRaw(SENSOR_PIN[i]);
      moisturePercent[i] = rawToPercent(raw, DRY_RAW[i], WET_RAW[i]);

      Serial.print(PLANT_NAME[i]);
      Serial.print(" raw=");
      Serial.print(raw);
      Serial.print(" moisture=");
      Serial.print(moisturePercent[i]);
      Serial.println("%");

      // Hysteresis control
      if (!pumpState[i] && moisturePercent[i] < THRESH_LOW[i]) {
        pumpState[i] = true;
        pumpStartTime[i] = now;
        digitalWrite(RELAY_PIN[i], RELAY_ON);
        Serial.print(">> "); Serial.print(PLANT_NAME[i]); Serial.println(" pump ON");
      }
      else if (pumpState[i] && moisturePercent[i] > THRESH_HIGH[i]) {
        pumpState[i] = false;
        digitalWrite(RELAY_PIN[i], RELAY_OFF);
        Serial.print(">> "); Serial.print(PLANT_NAME[i]); Serial.println(" pump OFF (target reached)");
      }

      // Safety cutoff: force OFF if valve has been open too long
      if (pumpState[i] && (now - pumpStartTime[i] > MAX_WATER_TIME)) {
        pumpState[i] = false;
        digitalWrite(RELAY_PIN[i], RELAY_OFF);
        Serial.print(">> "); Serial.print(PLANT_NAME[i]); Serial.println(" pump OFF (safety timeout)");
      }
    }
  }

  // --- 2. Rotate LCD display between the 3 plants ---
  if (now - lastLcdSwitch >= LCD_INTERVAL) {
    lastLcdSwitch = now;
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print(PLANT_NAME[lcdIndex]);
    lcd.print(" ");
    lcd.print(moisturePercent[lcdIndex]);
    lcd.print("%");

    lcd.setCursor(0, 1);
    lcd.print("Pump: ");
    lcd.print(pumpState[lcdIndex] ? "ON " : "OFF");

    lcdIndex = (lcdIndex + 1) % 3;
  }
}
