// EEECi 2026 | TechForge - Automatic Parking Management System
// AI-assisted reference implementation; not verified on the team's prototype.
// Assumptions: active-LOW IR modules, active-HIGH LEDs and active buzzer,
// 16x2 I2C LCD at 0x27. Verify wiring and servo angles before use.
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo gate;
const byte slots[] = {2, 3, 4, 5};
const byte entry = 6, exitSensor = 7;
const byte green = 8, yellow = 9, red = 10, buzzer = 11, servoPin = 12;
const int CLOSED = 0, OPEN = 90;
const unsigned long gateTime = 2500;
bool previousEntry = false, previousExit = false;
bool gateOpen = false, reserved = false, denied = false;
byte occupiedAtEntry = 0;
unsigned long openedAt = 0, reservedAt = 0;
int lastFree = -1;
void setup() {
  for (byte i=0; i<4; i++) pinMode(slots[i], INPUT_PULLUP);
  pinMode(entry, INPUT_PULLUP);
  pinMode(exitSensor, INPUT_PULLUP);
  pinMode(green, OUTPUT); pinMode(yellow, OUTPUT);
  pinMode(red, OUTPUT); pinMode(buzzer, OUTPUT);
  gate.attach(servoPin); gate.write(CLOSED);
  lcd.init(); lcd.backlight();
}
void loop() {
  unsigned long now = millis();
  byte occupied = 0;
  for (byte i=0; i<4; i++) if (digitalRead(slots[i]) == LOW) occupied++;
  bool entering = digitalRead(entry) == LOW;
  bool exiting = digitalRead(exitSensor) == LOW;
  // Reserve a space while an admitted car travels to its parking slot.
  if (reserved && (occupied > occupiedAtEntry || now-reservedAt >= 15000UL)) reserved = false;
  int freeSlots = 4 - occupied - (reserved ? 1 : 0);
  if (freeSlots < 0) freeSlots = 0;
  if (entering && !previousEntry) {
    denied = freeSlots == 0;
    if (!denied && !reserved && !gateOpen) {
      reserved = true; occupiedAtEntry = occupied; reservedAt = now;
      gate.write(OPEN); gateOpen = true; openedAt = now;
    }
  }
  // Exit detection requests a display refresh; slot sensors determine occupancy.
  if (exiting && !previousExit) lastFree = -1;
  if (gateOpen && !entering && now-openedAt >= gateTime) {
    gate.write(CLOSED); gateOpen = false;
  }
  if (!entering) denied = false;
  freeSlots = 4 - occupied - (reserved ? 1 : 0);
  if (freeSlots < 0) freeSlots = 0;
  digitalWrite(green, freeSlots >= 2);
  digitalWrite(yellow, freeSlots == 1);
  digitalWrite(red, freeSlots == 0);
  digitalWrite(buzzer, denied && entering && freeSlots == 0);
  if (freeSlots != lastFree) {
    lcd.setCursor(0,0); lcd.print("Free slots: "); lcd.print(freeSlots); lcd.print("   ");
    lcd.setCursor(0,1); lcd.print(freeSlots == 0 ? "PARKING FULL    " : "PARKING OPEN    ");
    lastFree = freeSlots;
  }
  previousEntry = entering; previousExit = exiting;
  delay(30);
}
