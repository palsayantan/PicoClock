#include <SPI.h>
#include <Wire.h>
#include <RTClib.h>                   // https://github.com/adafruit/RTClib
#include <Adafruit_SSD1306.h>         // https://github.com/adafruit/Adafruit_SSD1306
#include <U8g2_for_Adafruit_GFX.h>    // https://github.com/olikraus/U8g2_for_Adafruit_GFX
#include "logo.h"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
U8G2_FOR_ADAFRUIT_GFX oled;

bool TIME_FORMATTED = true;    // true = 12 hours format || false = 24 hours format
bool format;

RTC_DS3231 rtc;
int Second, Minute, Hour, Day, Month, Year;
float temp;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

const int btn1 = 14;          // Button 1
const int btn2 = 1;           // Button 2
const int buzzer = 16;        // Buzzer pin
const int vbat = 27;          // ADC pin
const int ps = 23;            // power saving pin

int frameDelay = 100;         // animation refrash rate
int debounceDelay = 300;      // Button debounce

int mode = 0;                 // set default clock interface
int secNow, mil, ss, mm;      // variables required by the stopwatch
int placeHolder, TH, TM, TS;  // variables required by the timer
float batValue, ADCmax = 2.1; // battery voltage via voltage devider 
unsigned long lastMillis, currentMillis, runTimer;

bool Clock = true, stopWatch, timer, settings;  // different modes, clock is the default
bool startLaps, setTimer, startTimer, sound, sett, battery;

void setup() {
  Serial.begin(115200);
  
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(btn1, INPUT_PULLUP);
  pinMode(btn2, INPUT_PULLUP);
  pinMode(buzzer, OUTPUT);
  pinMode(ps, OUTPUT);
  pinMode(vbat, INPUT);
  
  rtc.begin();
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  display.begin(SSD1306_SWITCHCAPVCC);
  oled.begin(display);                 // connect u8g2 procedures to Adafruit GFX
  display.clearDisplay();
  display.display();

  oled.setFontMode(1);                 // use u8g2 transparent mode (this is default)
  oled.setFontDirection(0);            // left to right (this is default)
  oled.setForegroundColor(WHITE);      // apply Adafruit GFX color

  //drawLogo();
  delay(1000);

  display.clearDisplay();
  display.drawBitmap(5, 0, pi, 32, 32, WHITE);
  display.display();
  delay(700);
  oled.setFont(u8g2_font_courB18_tf );      // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
  oled.setCursor(40, 14);
  oled.print("Pico");
  display.display();
  delay(300);
  oled.setCursor(40, 32);
  oled.print("Clock");
  display.display();
  delay(2000);

}

void loop () {
  DateTime now = rtc.now();
  Second = now.second();
  Minute = now.minute();
  Hour  = now.hour();
  Day = now.day();
  Month = now.month();
  Year = now.year();
  temp = rtc.getTemperature();

  float v = analogRead(vbat);     // read ADC value
  batValue = (v * ADCmax) / 1023;

  if (TIME_FORMATTED) {
    if (Hour >= 13) {
      Hour = Hour - 12;
      format = true;
    }
    if (Hour == 00) {
      Hour = 12;
      format = false;
    }
    else {
      Hour = Hour;
      format = false;
    }
  }
  else Hour = Hour;

  // turn the display off when both buttons are pressed
  while (digitalRead(btn1) == LOW && digitalRead(btn2) == LOW) {
    display.ssd1306_command(SSD1306_DISPLAYOFF);
    digitalWrite(ps, LOW);
    delay(1000);
  }

  // Change display functions
  if (digitalRead(btn1) == LOW) {
    if (Clock) {
      Clock = false;
      stopWatch = true;
    }
    else if (stopWatch) {
      stopWatch = false;
      timer = true;
    }
    else if (timer) {
      timer = false;
      settings = true;
    }
    else if (settings) {
      settings = false;
      Clock = true;
    }
    if (sound) {
      digitalWrite(buzzer, HIGH);
      delay(50);
      digitalWrite(buzzer, LOW);
    }
    delay(debounceDelay);
  }

  if (Clock) {
    if (digitalRead(btn2) == LOW) {
      mode++;
      if (mode > 4) mode = 0;
      if (sound) {
        digitalWrite(buzzer, HIGH);
        delay(10);
        digitalWrite(buzzer, LOW);
      }
      delay(debounceDelay);
    }
    if (secNow != Second) {
      switch (mode) {
        case 4:
          TIME_FORMATTED = false;
          display.clearDisplay();
          oled.setFont(u8g2_font_freedoomr25_mn);
          oled.setCursor(20, 32);

          if (Hour < 10) oled.print(0);
          oled.print(Hour);

          if (Second % 2 == 0) oled.print(":");
          else  oled.print(" ");

          if (Minute < 10) oled.print(0);
          oled.print(Minute);
          break;

        case 3:
          TIME_FORMATTED = true;
          display.clearDisplay();
          oled.setFontDirection(1);
          oled.setFont(u8g2_font_battery19_tn);
          oled.setCursor(100, 0);
          oled.print(int(batValue));        // Battery voltage bar
          oled.setFontDirection(0);

          oled.setFont(u8g2_font_crox1h_tr );

          oled.setCursor(5, 20);
          oled.print(Hour);
          oled.print(":");
          if (Minute < 10) oled.print(0);
          oled.print(Minute);
          oled.print(":");
          if (Second < 10) oled.print(0);
          oled.print(Second);
          oled.setCursor(55, 20);
          oled.print("|  ");
          oled.print(Day);
          oled.print("/");
          oled.print(Month);
          oled.print("/");
          oled.print(Year);

          oled.setCursor(5, 31);
          oled.print(temp);
          oled.print("°C");
          oled.setCursor(55, 31);
          oled.print("|  ");
          oled.print(daysOfTheWeek[now.dayOfTheWeek()]);
          break;

        case 2:
          TIME_FORMATTED = true;
          display.clearDisplay();
          oled.setFont(u8g2_font_luBIS14_te );
          oled.setCursor(15, 14);

          oled.print(Hour);

          if (Second % 2 == 0) oled.print(":");
          else  oled.print(" ");

          if (Minute < 10) oled.print(0);
          oled.print(Minute);

          if (Second % 2 == 0) oled.print(":");
          else  oled.print(" ");

          if (Second < 10) oled.print(0);
          oled.print(Second);

          oled.setCursor(0, 32);
          oled.print(Day);
          oled.print("/");
          oled.print(Month);
          oled.print("/");
          oled.print(Year);
          break;

        case 1:
          TIME_FORMATTED = true;
          display.clearDisplay();
          oled.setFont(u8g2_font_helvB24_tn );
          oled.setCursor(0, 30);

          oled.print(Hour);

          if (Second % 2 == 0) oled.print(":");
          else  oled.print(" ");

          if (Minute < 10) oled.print(0);
          oled.print(Minute);

          if (Second % 2 == 0) oled.print(":");
          else  oled.print(" ");

          if (Second < 10) oled.print(0);
          oled.print(Second);
          break;

        case 0:
          TIME_FORMATTED = true;
          display.clearDisplay();
          oled.setFont(u8g2_font_lucasarts_scumm_subtitle_o_tf );

          if (format) {
            oled.setCursor(90, 22);
            oled.print("AM");
          }
          else {
            oled.setCursor(90, 22);
            oled.print("PM");
          }

          oled.setFont(u8g2_font_bubble_tn );
          oled.setCursor(0, 24);

          oled.print(Hour);

          if (Second % 2 == 0) oled.print(":");
          else  oled.print(" ");

          if (Minute < 10) oled.print(0);
          oled.print(Minute);
          break;
      }
      display.display();
      secNow = Second;
    }
  }

  if (stopWatch) {
    display.clearDisplay();
    oled.setFont(u8g2_font_lucasarts_scumm_subtitle_o_tr );
    oled.setCursor(15, 13);
    oled.print("Stop");
    oled.setCursor(10, 31);
    oled.print("Watch");
    display.drawBitmap(80, 0, StopWatch, 32, 32, WHITE);
    display.display();
    if (digitalRead(btn2) == LOW) {
      if (sound) {
        digitalWrite(buzzer, HIGH);
        delay(25);
        digitalWrite(buzzer, LOW);
        delay(50);
        digitalWrite(buzzer, HIGH);
        delay(25);
        digitalWrite(buzzer, LOW);
      }
      delay(debounceDelay);
      display.clearDisplay();
      display.display();
      oled.setFont(u8g2_font_logisoso16_tn );
      lastMillis = millis();
      startLaps = true;
    }
    while (startLaps) {
      if (millis() - lastMillis != currentMillis) {
        mil = currentMillis % 1000;
        int tsec = (currentMillis / 1000) % 3600;
        ss = tsec % 60;
        mm = tsec / 60;

        display.fillRect(30, 0, 75, 16, BLACK);
        oled.setCursor(30, 16);
        if (mm < 10) oled.print(0);
        oled.print(mm);
        oled.print(":");
        if (ss < 10) oled.print(0);
        oled.print(ss);
        oled.print(":");
        oled.print(mil);
        display.display();
        currentMillis = millis() - lastMillis;
      }
      // Record laps
      if (digitalRead(btn2) == LOW) {
        display.fillRect(40, 23, 50, 9, BLACK);
        oled.setCursor(40, 32);
        oled.setFont(u8g2_font_7x13_tf  );
        oled.print(mm);
        oled.print(":");
        oled.print(ss);
        oled.print(":");
        oled.print(mil);
        display.display();
        oled.setFont(u8g2_font_logisoso16_tn );
      }
      // Reset laps
      if (digitalRead(btn1) == LOW) {
        startLaps = false;
        mm = 0; ss = 0; mil = 0;
        display.clearDisplay();
        oled.setCursor(30, 16);
        oled.print("00:00:00");
        display.display();
        if (sound) {
          digitalWrite(buzzer, HIGH);
          delay(50);
          digitalWrite(buzzer, LOW);
          delay(30);
          digitalWrite(buzzer, HIGH);
          delay(20);
          digitalWrite(buzzer, LOW);
        }
        delay(debounceDelay);
      }
    }
  }

  if (timer) {
    display.clearDisplay();
    oled.setFont(u8g2_font_lucasarts_scumm_subtitle_o_tr );
    oled.setCursor(10, 13);
    oled.print("Timer");
    oled.setCursor(10, 31);
    oled.print("Clock");
    display.drawBitmap(80, 0, Timer, 32, 32, WHITE);
    display.display();
    if (digitalRead(btn2) == LOW) {
      TH = 0;
      TM = 0;
      TS = 0;
      setTimer = true;
      if (sound) {
        digitalWrite(buzzer, HIGH);
        delay(30);
        digitalWrite(buzzer, LOW);
        delay(40);
        digitalWrite(buzzer, HIGH);
        delay(30);
        digitalWrite(buzzer, LOW);
      }
      delay(debounceDelay);
    }
    // Set timer
    while (setTimer) {
      display.clearDisplay();
      oled.setFont(u8g2_font_lubB19_tn );
      oled.setCursor(10, 24);
      if (TH < 10) oled.print(0);
      oled.print(TH);
      oled.print(":");
      oled.setCursor(50, 24);
      if (TM < 10) oled.print(0);
      oled.print(TM);
      oled.print(":");
      oled.setCursor(90, 24);
      if (TS < 10) oled.print(0);
      oled.print(TS);
      display.display();

      // Change cursor position
      if (digitalRead(btn1) == LOW) {
        placeHolder++;
        if (placeHolder > 4) placeHolder = 0;

        if (placeHolder == 0) display.fillRect(0, 0, 127, 31, 0);
        if (placeHolder == 1) display.fillRect(90, 30, 30, 3, 1);
        if (placeHolder == 2) display.fillRect(50, 30, 30, 3, 1);
        if (placeHolder == 3) display.fillRect(10, 30, 30, 3, 1);
        if (placeHolder == 4) display.drawRect(0, 0, 127, 29, 1);
        display.display();
        if (sound) {
          digitalWrite(buzzer, HIGH);
          delay(20);
          digitalWrite(buzzer, LOW);
        }
        delay(debounceDelay);
      }

      // Change timer values
      if (digitalRead(btn2) == LOW) {
        delay(100);
        if (placeHolder == 1) {
          TS++;
          if (TS > 59) TS = 0;
        }
        if (placeHolder == 2) {
          TM++;
          if (TM > 59) TM = 0;
        }
        if (placeHolder == 3) {
          TH++;
          if (TH > 12) TH = 0;
        }

        // start timer
        if (placeHolder == 4) {
          runTimer = (TH * 3600) + (TM * 60) + TS;
          if (sound) {
            digitalWrite(buzzer, HIGH);
            delay(40);
            digitalWrite(buzzer, LOW);
            delay(20);
            digitalWrite(buzzer, HIGH);
            delay(40);
            digitalWrite(buzzer, LOW);
          }
          startTimer = true;
        }

        // reset tiemer
        if (placeHolder == 0) {
          runTimer = 0;
          startTimer = false;
          setTimer = false;
          if (sound) {
            digitalWrite(buzzer, HIGH);
            delay(30);
            digitalWrite(buzzer, LOW);
            delay(50);
            digitalWrite(buzzer, HIGH);
            delay(20);
            digitalWrite(buzzer, LOW);
            delay(50);
            digitalWrite(buzzer, HIGH);
            delay(50);
            digitalWrite(buzzer, LOW);
          }
          delay(debounceDelay);
        }
      }

      // Run timer
      if (startTimer) {
        runTimer--;
        if (runTimer <= 0 || digitalRead(btn1) == LOW) {
          startTimer = false;
          setTimer = false;
          oled.setFont(u8g2_font_luIS14_te  );
          for (int i = 0; i < 6; i++) {
            display.clearDisplay();
            display.display();
            if (sound) {
              digitalWrite(buzzer, HIGH);
              delay(100);
              digitalWrite(buzzer, LOW);
              delay(100);
              digitalWrite(buzzer, HIGH);
              delay(100);
              digitalWrite(buzzer, LOW);
              delay(100);
              digitalWrite(buzzer, HIGH);
            }
            delay(200);
            oled.setCursor(30, 14);
            oled.print("Timer");
            oled.setCursor(20, 31);
            oled.print("Finished!");
            display.display();
            digitalWrite(buzzer, LOW);
            delay(500);
          }
        }
        TH = runTimer / 3600;
        TM = runTimer / 60;
        TS = runTimer % 60;
        delay(1000);
      }
    }

    // return
    if (digitalRead(btn1) == LOW) {
      timer = false;
      settings = true;
      if (sound) {
        digitalWrite(buzzer, HIGH);
        delay(100);
        digitalWrite(buzzer, LOW);
      }
      delay(debounceDelay);
    }
  }

  if (settings) {
    display.clearDisplay();
    oled.setFont(u8g2_font_lucasarts_scumm_subtitle_o_tr );
    oled.setCursor(10, 23);
    oled.print("Settings");
    display.drawBitmap(95, 8, setting, 22, 22, WHITE);
    display.display();
    if (digitalRead(btn2) == LOW) {
      sett = true;
      delay(debounceDelay);
    }
    while (sett) {
      display.clearDisplay();
      oled.setFont(u8g2_font_profont22_tf );
      oled.setCursor(10, 23);
      oled.print("Sound:");

      // change sound settings
      if (digitalRead(btn2) == LOW) {
        if (sound) {
          sound = false;
          display.drawBitmap(95, 3, soundOff, 25, 25, WHITE);
          display.display();
          delay(debounceDelay);
        }
        else {
          sound = true;
          display.drawBitmap(95, 3, soundOn, 25, 25, WHITE);
          display.display();
          digitalWrite(buzzer, HIGH);
          delay(50);
          digitalWrite(buzzer, LOW);
          delay(50);
          digitalWrite(buzzer, HIGH);
          delay(50);
          digitalWrite(buzzer, LOW);
          delay(50);
        }
      }
      if (digitalRead(btn1) == LOW) {
        sett = false;
        battery = true;
        delay(debounceDelay);
      }

      // Show battery status
      while (battery) {
        display.clearDisplay();
        oled.setFont(u8g2_font_crox1h_tr);
        oled.setCursor(25, 10);
        oled.print("Battery Status");
        oled.setCursor(15, 28);
        float v = analogRead(vbat);
        Serial.println(v);
        batValue = (v * ADCmax) / 1023;
        oled.print(batValue);
        oled.print("v");
        oled.setCursor(85, 28);
        int percentage = (batValue / ADCmax) * 100;
        oled.print(percentage);

        display.display();
        delay(debounceDelay);
        if (digitalRead(btn1) == LOW) {
          battery = false;
          settings = false;
          Clock = true;
          delay(debounceDelay);
        }
      }
    }
  }
}

void drawLogo() {
  display.drawBitmap(14, 0, pcbway43, 101, 32, WHITE); display.display(); delay(frameDelay);
  display.drawBitmap(14, 0, pcbway42, 101, 32, WHITE); display.display(); delay(frameDelay);
  display.drawBitmap(14, 0, pcbway41, 101, 32, WHITE); display.display(); delay(frameDelay);
  display.drawBitmap(14, 0, pcbway40, 101, 32, WHITE); display.display(); delay(frameDelay);
  display.drawBitmap(14, 0, pcbway39, 101, 32, WHITE); display.display(); delay(frameDelay);
  display.drawBitmap(14, 0, pcbway38, 101, 32, WHITE); display.display(); delay(frameDelay);
  display.drawBitmap(14, 0, pcbway37, 101, 32, WHITE); display.display(); delay(frameDelay);
  display.drawBitmap(14, 0, pcbway36, 101, 32, WHITE); display.display(); delay(frameDelay);
  display.drawBitmap(14, 0, pcbway35, 101, 32, WHITE); display.display(); delay(frameDelay);
  display.drawBitmap(14, 0, pcbway34, 101, 32, WHITE); display.display(); delay(frameDelay);
  display.drawBitmap(14, 0, pcbway33, 101, 32, WHITE); display.display(); delay(frameDelay);
  display.drawBitmap(14, 0, pcbway32, 101, 32, WHITE); display.display(); delay(frameDelay);
  display.drawBitmap(14, 0, pcbway31, 101, 32, WHITE); display.display(); delay(frameDelay);
  display.drawBitmap(14, 0, pcbway30, 101, 32, WHITE); display.display(); delay(frameDelay);
  display.drawBitmap(14, 0, pcbway29, 101, 32, WHITE); display.display(); delay(frameDelay);
  display.drawBitmap(14, 0, pcbway28, 101, 32, WHITE); display.display(); delay(frameDelay);
  display.drawBitmap(14, 0, pcbway27, 101, 32, WHITE); display.display(); delay(frameDelay);
  display.drawBitmap(14, 0, pcbway26, 101, 32, WHITE); display.display(); delay(frameDelay);
  display.drawBitmap(14, 0, pcbway25, 101, 32, WHITE); display.display(); delay(frameDelay);
  display.drawBitmap(14, 0, pcbway24, 101, 32, WHITE); display.display(); delay(frameDelay);
  display.drawBitmap(14, 0, pcbway23, 101, 32, WHITE); display.display(); delay(frameDelay);
  display.drawBitmap(14, 0, pcbway22, 101, 32, WHITE); display.display(); delay(frameDelay);
  display.drawBitmap(14, 0, pcbway21, 101, 32, WHITE); display.display(); delay(frameDelay);
  display.drawBitmap(14, 0, pcbway20, 101, 32, WHITE); display.display(); delay(frameDelay);
  display.drawBitmap(14, 0, pcbway19, 101, 32, WHITE); display.display(); delay(frameDelay);
  display.drawBitmap(14, 0, pcbway18, 101, 32, WHITE); display.display(); delay(frameDelay);
  display.drawBitmap(14, 0, pcbway17, 101, 32, WHITE); display.display(); delay(frameDelay);
  display.drawBitmap(14, 0, pcbway16, 101, 32, WHITE); display.display(); delay(frameDelay);
  display.drawBitmap(14, 0, pcbway15, 101, 32, WHITE); display.display(); delay(frameDelay);
  display.drawBitmap(14, 0, pcbway14, 101, 32, WHITE); display.display(); delay(frameDelay);
  display.drawBitmap(14, 0, pcbway13, 101, 32, WHITE); display.display(); delay(frameDelay);
  display.drawBitmap(14, 0, pcbway12, 101, 32, WHITE); display.display(); delay(frameDelay);
  display.drawBitmap(14, 0, pcbway11, 101, 32, WHITE); display.display(); delay(frameDelay);
  display.drawBitmap(14, 0, pcbway10, 101, 32, WHITE); display.display(); delay(frameDelay);
  display.drawBitmap(14, 0, pcbway9, 101, 32, WHITE); display.display(); delay(frameDelay);
  display.drawBitmap(14, 0, pcbway8, 101, 32, WHITE); display.display(); delay(frameDelay);
  display.drawBitmap(14, 0, pcbway7, 101, 32, WHITE); display.display(); delay(frameDelay);
  display.drawBitmap(14, 0, pcbway6, 101, 32, WHITE); display.display(); delay(frameDelay);
  display.drawBitmap(14, 0, pcbway5, 101, 32, WHITE); display.display(); delay(frameDelay);
  display.drawBitmap(14, 0, pcbway4, 101, 32, WHITE); display.display(); delay(frameDelay);
  display.drawBitmap(14, 0, pcbway3, 101, 32, WHITE); display.display(); delay(frameDelay);
  display.drawBitmap(14, 0, pcbway2, 101, 32, WHITE); display.display(); delay(frameDelay);
  display.drawBitmap(14, 0, pcbway1, 101, 32, WHITE); display.display(); delay(frameDelay);
}
