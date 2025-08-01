#include "Arduino.h"
#include "uRTCLib.h"
#include "SevSeg.h"
#include "pitches.h"

// Rotary Encoder Inputs
#define CLK A5
#define DT A4
#define SW 3

#define BUZZER_PIN 2

SevSeg sevseg;
uRTCLib rtc(0x68);

bool alarmSilencedThisHour = false;
unsigned long lastAlarmTime = 0;
const unsigned long alarmInterval = 10000; // Repeat every 10 seconds

int melodyIndex = 0;
unsigned long lastNoteTime = 0;
bool isAlarmPlaying = false;
bool alarmDismissed = false;

// Button debounce variables
bool lastButtonState = HIGH;
bool buttonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

int melody[] = {
  NOTE_E5, NOTE_E5, REST, NOTE_E5, REST, NOTE_C5, NOTE_E5,
  NOTE_G5, REST, NOTE_G4
};

int durations[] = {
  8, 8, 8, 8, 8, 8, 8,
  4, 4, 8
};

void setup() {
  Serial.begin(9600);
  URTCLIB_WIRE.begin();

  // Uncomment only once to set time
  rtc.set(30, 58, 10, 6, 1, 8, 25); // (sec, min, hr, wday, day, month, year)

  byte numDigits = 4;
  byte digitPins[] = {A2, A1, 4, 5};
  byte segmentPins[] = {6, 7, 8, 9, 10, 11, 12, 13};
  bool resistorsOnSegments = false;

  sevseg.begin(COMMON_CATHODE, 4, digitPins, segmentPins, resistorsOnSegments);
  sevseg.setBrightness(90);

  pinMode(CLK, INPUT);
  pinMode(DT, INPUT);
  pinMode(SW, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
}

void loop() {
  rtc.refresh();

  int hour = rtc.hour();
  int minute = rtc.minute();

  // 12-hour format
  int displayHour = hour % 12;
  if (displayHour == 0) displayHour = 12;

  // Format and display time (e.g. 0730)
  char timeStr[5];
  sprintf(timeStr, "%02d.%02d", displayHour, minute);
  sevseg.setChars(timeStr);
  sevseg.refreshDisplay();

  // --- Button debounce code ---
  int reading = digitalRead(SW);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;

      if (buttonState == LOW) {  // Button pressed
        Serial.println("Button pressed — dismiss alarm");
        alarmDismissed = true;
        isAlarmPlaying = false;
        noTone(BUZZER_PIN);
      }
    }
  }

  lastButtonState = reading;
  // --- End button debounce ---

  // Check if it's time to trigger the alarm (9:00–9:59 AM in your code)
  if (hour == 7 && !alarmDismissed) {
    isAlarmPlaying = true;
  } else if (hour != 7) {
    alarmDismissed = false;  // reset for next day
    isAlarmPlaying = false;
  }

  // Play alarm non-blocking
  if (isAlarmPlaying) {
    unsigned long now = millis();
    int size = sizeof(durations) / sizeof(int);

    if (melodyIndex < size) {
      if (now - lastNoteTime >= (1000 / durations[melodyIndex]) * 1.30) {
        int duration = 1000 / durations[melodyIndex];
        tone(BUZZER_PIN, melody[melodyIndex], duration);
        lastNoteTime = now;
        melodyIndex++;
      }
    } else {
      melodyIndex = 0;  // loop melody
    }
  }
}
