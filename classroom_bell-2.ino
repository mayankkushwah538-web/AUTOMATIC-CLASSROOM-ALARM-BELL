#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS3231 rtc;

const int btnSet  = 2;   // Switch 2: SET / next setting
const int btnUp   = 3;   // Switch 1: increase (wired to pin 3)
const int btnDown = 4;   // Switch 3: decrease (wired to pin 4)
const int buzzer  = 7;

// Settings (stored in EEPROM)
int firstHour = 8,  firstMin = 0;    // 1st bell
int lastHour  = 16, lastMin  = 0;    // last bell
int interval  = 45;                  // minutes between bells

// 0 = clock, 1 = first hour, 2 = first min, 3 = last hour, 4 = last min, 5 = interval
int setMode = 0;

void setup() {
  pinMode(btnSet, INPUT_PULLUP);
  pinMode(btnUp, INPUT_PULLUP);
  pinMode(btnDown, INPUT_PULLUP);
  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, LOW);

  lcd.init();
  lcd.backlight();

  if (!rtc.begin()) {
    lcd.print("RTC Not Found!");
    while (1);
  }

  // To set time: uncomment, upload, then comment again and re-upload.
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  loadSettings();
}

void loop() {
  DateTime now = rtc.now();

  // SET button: go to next setting
  if (digitalRead(btnSet) == LOW) {
    setMode++;
    if (setMode > 5) {          // finished all settings -> save, back to clock
      setMode = 0;
      saveSettings();
    }
    lcd.clear();
    delay(300);
  }

  switch (setMode) {
    case 1:
      lcd.setCursor(0, 0); lcd.print("1st Bell: HOUR  ");
      adjust(firstHour, 24);
      showValue(firstHour);
      break;
    case 2:
      lcd.setCursor(0, 0); lcd.print("1st Bell: MIN   ");
      adjust(firstMin, 60);
      showValue(firstMin);
      break;
    case 3:
      lcd.setCursor(0, 0); lcd.print("Last Bell: HOUR ");
      adjust(lastHour, 24);
      showValue(lastHour);
      break;
    case 4:
      lcd.setCursor(0, 0); lcd.print("Last Bell: MIN  ");
      adjust(lastMin, 60);
      showValue(lastMin);
      break;
    case 5:
      lcd.setCursor(0, 0); lcd.print("Period Interval ");
      adjustInterval();
      lcd.setCursor(0, 1); lcd.print("> "); lcd.print(interval); lcd.print(" min    ");
      break;
    default:
      displayClock(now);
      checkBell(now);
      break;
  }
}

// ---------- Helpers ----------
void adjust(int &value, int range) {
  if (digitalRead(btnUp) == LOW)   { value = (value + 1) % range; delay(200); }
  if (digitalRead(btnDown) == LOW) { value = (value + range - 1) % range; delay(200); }
}

void adjustInterval() {
  if (digitalRead(btnUp) == LOW)   { if (interval < 180) interval++; delay(200); }
  if (digitalRead(btnDown) == LOW) { if (interval > 1)   interval--; delay(200); }
}

void showValue(int v) {
  lcd.setCursor(0, 1); lcd.print("> ");
  if (v < 10) lcd.print('0');
  lcd.print(v); lcd.print("      ");
}

void print2(int v) {
  if (v < 10) lcd.print('0');
  lcd.print(v);
}

void displayClock(DateTime now) {
  // Line 1: current time
  lcd.setCursor(0, 0);
  lcd.print("Time: ");
  print2(now.hour()); lcd.print(':');
  print2(now.minute()); lcd.print(':');
  print2(now.second());
  lcd.print("  ");

  // Line 2: next bell (hidden while ringing)
  if (isRinging(now)) return;
  int next = nextBell(now.hour() * 60 + now.minute());
  lcd.setCursor(0, 1);
  lcd.print("Next: ");
  print2(next / 60); lcd.print(':'); print2(next % 60);
  lcd.print("     ");
}

bool isRinging(DateTime now) {
  int first = firstHour * 60 + firstMin;
  int last  = lastHour * 60 + lastMin;
  int nowMin = now.hour() * 60 + now.minute();
  return (nowMin >= first && nowMin <= last &&
          (nowMin - first) % interval == 0 &&
          now.second() < 10);               // 10-second bell
}

void checkBell(DateTime now) {
  if (isRinging(now)) {
    digitalWrite(buzzer, HIGH);
    lcd.setCursor(0, 1); lcd.print("** BELL RINGING *");
  } else {
    digitalWrite(buzzer, LOW);
  }
}

// Returns next bell time in minutes since midnight
int nextBell(int nowMin) {
  int first = firstHour * 60 + firstMin;
  int last  = lastHour * 60 + lastMin;
  if (nowMin < first) return first;
  int t = first + ((nowMin - first) / interval + 1) * interval;
  if (t <= last) return t;
  return first;                              // tomorrow's first bell
}

void saveSettings() {
  EEPROM.update(0, 123);                     // marker: data is valid
  EEPROM.update(1, firstHour); EEPROM.update(2, firstMin);
  EEPROM.update(3, lastHour);  EEPROM.update(4, lastMin);
  EEPROM.update(5, interval);
}

void loadSettings() {
  if (EEPROM.read(0) == 123) {
    firstHour = EEPROM.read(1); firstMin = EEPROM.read(2);
    lastHour  = EEPROM.read(3); lastMin  = EEPROM.read(4);
    interval  = EEPROM.read(5);
    if (interval < 1) interval = 1;
  }
}
