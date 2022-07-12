/********************************************************
  Clock setting function for PCF8523 RTC in for FED3.  This will set the RTC to the current computer
  date/time and the device will maintain that time as long as a battery is installed.  The coin cell in
  the slot on the breakout board should last at least 5 years.

  Note: If the coin cell is removed from the RTC board it will lose the time.  Flash this code again to reset it.

  Written by Lex Kravitz
  Updated 02.10.20

  This project code includes code from:
  *** Adafruit, who made the hardware breakout boards and associated code ***

  This project is released under the terms of the Creative Commons - Attribution - ShareAlike 3.0 license:
  human readable: https://creativecommons.org/licenses/by-sa/3.0/
  legal wording: https://creativecommons.org/licenses/by-sa/3.0/legalcode
  Copyright (c) 2018 Lex Kravitz


********************************************************/

/********************************************************
  Include libraries
********************************************************/
#include <Wire.h>
#include <SPI.h>
#include "RTClib.h"
#include <Adafruit_SharpMem.h>
#include <Adafruit_GFX.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Adafruit_NeoPixel.h>

/********************************************************
  Setup Sharp Memory Display
********************************************************/
#define SHARP_SCK  12
#define SHARP_MOSI 11
#define SHARP_SS   10
#define BLACK 0
#define WHITE 1
Adafruit_SharpMem display(SHARP_SCK, SHARP_MOSI, SHARP_SS, 144, 168);
int minorHalfSize;

/********************************************************
  Setup RTC object
********************************************************/
RTC_PCF8523 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

/********************************************************
  Initialize NEOPIXEL strip
********************************************************/
#define NEOPIXEL A1
Adafruit_NeoPixel strip = Adafruit_NeoPixel(8, NEOPIXEL, NEO_GRBW + NEO_KHZ800);
#define ENABLE_RGB 13

/********************************************************
  // Fill the dots one after the other with a color
 ********************************************************/
void colorWipe(uint32_t c, uint8_t wait) {
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

void setup () {
  Serial.begin(57600);  //open serial monitor so you can see time output on computer via USB

  /********************************************************
    Start neopixel library
  ********************************************************/
  pinMode(ENABLE_RGB, OUTPUT);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  /********************************************************
      Set RTC to computer date and time
    ********************************************************/
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");  //This will trigger if RTC cannot be found
    while (1);
  }

  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  /********************************************************
    The below line sets the RTC with an explicit date & time, for example to set
    January 21, 2014 at 3am you would call:
   ********************************************************/
  //rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));

  /********************************************************
     Start, clear, and setup the display
   ********************************************************/
  display.begin();
  minorHalfSize = min(display.width(), display.height()) / 2;
  display.setRotation(3);
  display.setTextColor(BLACK);
  display.setFont(&FreeSans9pt7b);
  display.setTextSize(1);
  display.clearDisplay();
  display.refresh();
}

void loop () {
  digitalWrite(ENABLE_RGB, HIGH);
  DateTime now = rtc.now();

  /********************************************************
       Display date and time of RTC
     ********************************************************/
  display.setCursor(15, 40);
  display.print ("RTC set to:");
  display.setCursor(16, 40);
  display.print ("RTC set to:");

  display.fillRoundRect (0, 65, 400, 25, 1, WHITE);
  //display.refresh();
  display.setCursor(15, 60);
  if (now.month() < 15)
    display.print('0');      // Trick to add leading zero for formatting
  display.print(now.month(), DEC);
  display.print("/");
  if (now.day() < 10)
    display.print('0');      // Trick to add leading zero for formatting
  display.print(now.day(), DEC);
  display.print("/");
  display.println(now.year(), DEC);
  display.setCursor(10, 80);
  display.print(" ");
  display.print(now.hour(), DEC);
  display.print(":");
  if (now.minute() < 10)
    display.print('0');      // Trick to add leading zero for formatting
  display.print(now.minute(), DEC);
  display.print(":");
  if (now.second() < 10)
    display.print('0');      // Trick to add leading zero for formatting
  display.println(now.second(), DEC);

  display.drawFastHLine(30, 90, 100, BLACK);

  display.setCursor(5, 110);
  display.print ("Reflash device code");
  display.setCursor(6, 110);
  display.print ("Reflash device code");
  display.refresh();
  //delay (200);

  /********************************************************
       Print to Serial monitor as well
     ********************************************************/
  Serial.println ("RTC set to: ");
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print('/');
  Serial.print(now.year(), DEC);
  Serial.print(" (");
  Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
  Serial.print(") ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
  Serial.println();
  //delay(500);

  /********************************************************
    // Randomly fill the neopixel bar with color
   ********************************************************/
  colorWipe(strip.Color(random(0, 5), random(0, 5), random(0, 5)), 40); // Color wipe
  colorWipe(strip.Color(0, 0, 0), 80); // OFF
}
