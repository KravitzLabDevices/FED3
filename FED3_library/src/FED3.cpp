/*
  Feeding experimentation device 3 (FED3) library version 1.0.122
  Code by Lex Kravitz, adapted to this library by Eric Lin
  alexxai@wustl.edu
  erclin@ucdavis.edu
  Released in August of 2020

  FED was originally developed by Nguyen at al and published in 2016:
  https://www.ncbi.nlm.nih.gov/pubmed/27060385

  This device includes hardware and code from:
  *** Adafruit, who made the hardware breakout boards and associated code we used in FED ***

  Cavemoa's excellent examples of datalogging with the Adalogger:
  https://github.com/cavemoa/Feather-M0-Adalogger

  Arduino Time library http://playground.arduino.cc/code/time
  Maintained by Paul Stoffregen https://github.com/PaulStoffregen/Time

  This project is released under the terms of the Creative Commons - Attribution - ShareAlike 3.0 license:
  human readable: https://creativecommons.org/licenses/by-sa/3.0/
  legal wording: https://creativecommons.org/licenses/by-sa/3.0/legalcode
  Copyright (c) 2019, 2020 Lex Kravitz
*/


/********************************************************
  Include libraries
********************************************************/
#include "Arduino.h"
#include "FED3.h"

/********************************************************
  Start FED3 and RTC objects
********************************************************/
FED3 *pointerToFED3;
RTC_PCF8523 rtc;

/********************************************************
  Interrupt handlers
********************************************************/
static void outsidePelletTriggerHandler(void) {
  pointerToFED3->pelletTrigger();
}

static void outsideLeftTriggerHandler(void) {
  pointerToFED3->leftTrigger();
}

static void outsideRightTriggerHandler(void) {
  pointerToFED3->rightTrigger();
}

/**********************************************
  FED3 main loop 
**********************************************/
void FED3::run() {
  CheckPokes();
  UpdateDisplay();
  goToSleep();
}

/********************************************************
  Poke functions
********************************************************/
//Check pokes
void FED3::CheckPokes() {
  //if it is free feeding make Ratio_Met always true
  if (FEDmode == 0) Ratio_Met = true; 
  
  // If it's a timed feeding session only set Ratio_Met to true between these hours
  if (FEDmode == 11)  {
    DateTime now = rtc.now();
    if (now.hour() >= timedStart && now.hour() < timedEnd) {
      Ratio_Met = true;
    }
  }

  if (Left) {
    LeftCount ++;
    display.fillCircle(25, 59, 5, BLACK);
    display.refresh();
    pellet = false;
    logdata();
    if (activePoke == 1) {
      CheckRatio();
    }
    CheckReset();
  }

  if (Right) {
    RightCount ++;
    display.fillCircle(25, 79, 5, BLACK);
    display.refresh();
    pellet = false;
    logdata();
    if (activePoke == 0) {
      CheckRatio();
    }
    CheckReset();
  }
}

/********************************************************
  Check Ratio (this decides whether FED should dispense or not)
********************************************************/
void FED3::CheckRatio(){
  //Fixed ratio
  if (FEDmode < 4 and LeftCount % FR == 0 and LeftCount != 0) { //For fixed ratio sessions, test if the number of left counts is divisible by the FR
      Ratio_Met = true;
  }

  // Progressive ratio
  if (FEDmode == 4) {
      if (LeftCount >= (ratio)) { // if LeftCount is greater than or equal to the required ratio
        Ratio_Met = true;
      }
  }
}

/********************************************************
  Feed function drops a pellet
********************************************************/
void FED3::Feed() {
   UpdateDisplay();
   while (digitalRead (PELLET_WELL) == HIGH and PelletAvailable == false) {
      digitalWrite (MOTOR_ENABLE, HIGH);  //Enable motor driver

      for (int i = 1; i < 10; i++) {
        if (digitalRead (PELLET_WELL) == HIGH) {
          stepper.step(-30);
        }
      }
      digitalWrite (MOTOR_ENABLE, LOW);  //Disable motor driver

      dispenseTimer();
      numMotorTurns++;

      if (digitalRead (PELLET_WELL) == LOW) {
        Ratio_Met = false;
        PelletAvailable = true;
        PelletCount++;
        logdata();
        ratio = ratio + round ((5 * exp (0.2 * PelletCount)) - 5); // this is a formula from Richardson and Roberts (1996) https://www.ncbi.nlm.nih.gov/pubmed/8794935

      }
      
      //When to initiate jam clearing movements
      if (numMotorTurns == 5 or numMotorTurns == 15 or numMotorTurns == 25){
        MinorJam();
      }
      
      if (numMotorTurns == 10 or numMotorTurns == 30) {
        VibrateJam();
      }

      if (numMotorTurns == 20 or numMotorTurns == 40) {
        ClearJam();
      }
   }
}

/********************************************************
  Jam clearing functions
********************************************************/
void FED3::MinorJam(){
    digitalWrite (MOTOR_ENABLE, HIGH);  //Enable motor driver
    stepper.step(100); //minor adjustment to try to dislodget pellet
    ReleaseMotor ();
}

void FED3::VibrateJam() {
    numJamClears++;
    DisplayJamClear();
    if (digitalRead (PELLET_WELL) == HIGH) {
      delay (250); //simple debounce to ensure pellet is out for at least 250ms
      if (digitalRead (PELLET_WELL) == HIGH) {
        digitalWrite (MOTOR_ENABLE, HIGH);  //Enable motor driver
        for (int j = 0; j < 30; j++) {
          if (digitalRead (PELLET_WELL) == HIGH) {
            stepper.step(120);
            if (digitalRead (PELLET_WELL) == HIGH) {
              stepper.step(-60);
            }
          }
        }
        ReleaseMotor ();
      }
    }
  display.fillRoundRect (5, 15, 120, 15, 1, WHITE);  //erase the "Jam clear" text without clearing the entire screen by pasting a white box over it
}

/********************************************************
    ClearJam makes full rotations to try to dislodge a pellet jam.
********************************************************/
void FED3::ClearJam() {
    numJamClears++;
    DisplayJamClear();
    if (digitalRead (PELLET_WELL) == HIGH) {
      delay (250); //simple debounce to ensure pellet is out for at least 250ms
      if (digitalRead (PELLET_WELL) == HIGH) {
        digitalWrite (MOTOR_ENABLE, HIGH);  //Enable motor driver
        for (int i = 0; i < 21 + random(0, 20); i++) {
          if (digitalRead (PELLET_WELL) == HIGH) {
            stepper.step(-i * 4);
          }
        }
      }
      ReleaseMotor ();
    }
    if (digitalRead (PELLET_WELL) == HIGH) {
      delay (250); //simple debounce
      if (digitalRead (PELLET_WELL) == HIGH) {
        digitalWrite (MOTOR_ENABLE, HIGH);  //Enable motor driver
        for (int i = 0; i < 21 + random(0, 20); i++) {
          if (digitalRead (PELLET_WELL) == HIGH) {
            stepper.step(i * 4);
          }
        }
        ReleaseMotor ();
      }
    }
    display.fillRoundRect (5, 15, 120, 15, 1, WHITE);  //erase the "Jam clear" text without clearing the entire screen by pasting a white box over it
    numMotorTurns = 0;
}

//Holding both pokes for 1 second will reset the device
void FED3::CheckReset() {
  if (digitalRead(LEFT_POKE) == LOW & digitalRead(RIGHT_POKE) == LOW ) {
    delay(1000);
    if (digitalRead(LEFT_POKE) == LOW & digitalRead(RIGHT_POKE) == LOW ) {
      display.clearDisplay();
      display.setRotation(3);
      display.setTextColor(BLACK);
      display.setCursor(15, 40);
      display.setTextSize(1);
      display.println("Resetting FED...");
      display.refresh();
      tone(BUZZER, 5000, 400); delay(200); tone(BUZZER, 2000, 300); delay(200); tone(BUZZER, 4000, 600);
      colorWipe(strip.Color(2, 0, 0), 40); delay(100); // Color wipe
      colorWipe(strip.Color(2, 0, 2), 40); delay(100); // Color wipe
      colorWipe(strip.Color(0, 2, 2), 40); delay(100); // Color wipe
      colorWipe(strip.Color(0, 0, 0), 20); // OFF
      NVIC_SystemReset();      // processor software reset
    }
  }
}

/********************************************************
  NeoPixel and audio stimuli
********************************************************/
// Stimuli
void FED3::ConditionedStimulus() {
  tone (BUZZER, 4000, 300);
  colorWipe(strip.Color(0, 2, 2), 40); // Color wipe
  colorWipe(strip.Color(0, 0, 0), 20); // OFF
}

void FED3::RConditionedStim() {
  tone (BUZZER, 4000, 300);
  RcolorWipe(strip.Color(0, 2, 2), 40); // Color wipe
  RcolorWipe(strip.Color(0, 0, 0), 20); // OFF
}

void FED3::ErrorStim() {
  // Random noise to signal errors
  for (int i = 0; i < 30; i++) {
    tone (BUZZER, random(100, 200), 20);
    delay(5);
  }
}

void FED3::leftStimulus() {
  // Visual tracking stimulus - left
  strip.setPixelColor(0, strip.Color(2, 0, 2, 2) );
  strip.show();
}

void FED3::rightStimulus() {
  // Visual tracking stimulus - right
  strip.setPixelColor(7, strip.Color(2, 0, 2, 2) );
  strip.show();
}

//colorWipe does a color wipe from left to right
void FED3::colorWipe(uint32_t c, uint8_t wait) {
  digitalWrite (MOTOR_ENABLE, HIGH);  //ENABLE motor driver
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
  digitalWrite (MOTOR_ENABLE, LOW);  //DISABLE motor driver
}

//RcolorWipe does a color wipe from right to left
void FED3::RcolorWipe(uint32_t c, uint8_t wait) {  //reverse color wipe
  digitalWrite (MOTOR_ENABLE, HIGH);  //ENABLE RGB/motor driver
  for (uint16_t j = 0; j < strip.numPixels(); j++) {
    strip.setPixelColor(7 - j, c);
    strip.show();
    delay(wait);
    digitalWrite (MOTOR_ENABLE, LOW);  //DISABLE RGB/motor driver
  }
}

//Short helper function for blinking LEDs and BNC out port
void FED3::Blink(byte PIN, byte DELAY_MS, byte loops) {
  for (byte i = 0; i < loops; i++)  {
    digitalWrite(PIN, HIGH);
    delay(DELAY_MS);
    digitalWrite(PIN, LOW);
    delay(DELAY_MS);
  }
}

//Short helper function for pulsing on BNC out port
void FED3::BNC_out(byte DELAY_MS, byte loops) {
  for (byte i = 0; i < loops; i++)  {
    digitalWrite(A0, HIGH);
    delay(DELAY_MS);
    digitalWrite(A0, LOW);
    delay(DELAY_MS);
  }
}

/********************************************************
  Display functions
********************************************************/
void FED3::UpdateDisplay() {
  //colorWipe(strip.Color(0, 0, 0), 5); // OFF

  display.setRotation(3);
  display.setTextColor(BLACK);
  display.setTextSize(1);

  display.setCursor(5, 15);
  display.print("FED:");
  display.println(FED);
  display.setCursor(6, 15);  // this doubling is a way to do bold type
  display.print("FED:");
  display.setTextSize(1);
  display.fillRoundRect (6, 20, 200, 22, 1, WHITE);  //erase the data on screen without clearing the entire screen by pasting a white box over it

  if (FEDmode == 0) {
    display.fillRoundRect (35, 24, 200, 50, 1, WHITE);  //erase the data on screen without clearing the entire screen by pasting a white box over it
    display.setCursor(22, 64);
    display.print("Free Feeding");
  }

  if (FEDmode == 11) {
    display.fillRoundRect (35, 24, 200, 50, 1, WHITE);  //erase the data on screen without clearing the entire screen by pasting a white box over it
    display.setCursor(22, 64);
    display.println("Timed Feeding");
    display.setCursor(22, 80);
    display.print(timedStart);
    display.print(" to ");
    display.print(timedEnd);
  }

  if (FEDmode < 4 & FEDmode != 0 & FEDmode != 11) {
    display.setCursor(5, 36);
    display.print("FR: ");
    display.setCursor(6, 36);
    display.print("FR: ");
    display.print(FR);
  }

  if (FEDmode == 4 & LeftCount == 0) { //Prog ratio, first poke
    display.setCursor(5, 36);
    display.print("PR: ");
    display.setCursor(6, 36);
    display.print("PR: ");
    display.fillRoundRect (35, 24, 200, 55, 1, WHITE);  //erase the data on screen without clearing the entire screen by pasting a white box over it
    display.print(1);
  }

  if (FEDmode == 4 & LeftCount != 0) { // Prog ratio, NOT first poke
    display.setCursor(5, 36);
    display.print("PR: ");
    display.setCursor(6, 36);
    display.print("PR: ");
    display.fillRoundRect (35, 24, 200, 55, 1, WHITE);  //erase the data on screen without clearing the entire screen by pasting a white box over it
    display.print(ratio - LeftCount);
  }

  if (FEDmode == 5) {
    display.setCursor(5, 36);
    display.print("Extinction");
  }

  if (FEDmode == 6) {
    display.setCursor(5, 36);
    display.print("Light tracking");
  }

  if (FEDmode == 7) {
    display.setCursor(5, 36);
    display.print("FR1 (reversed)");
  }

  if (FEDmode == 8 & RightCount == 0) { //Prog ratio, first poke
    display.setCursor(5, 36);
    display.print("PR(R): ");
    display.setCursor(6, 36);
    display.print("PR(R): ");
    display.fillRoundRect (55, 24, 200, 55, 1, WHITE);  //erase the data on screen without clearing the entire screen by pasting a white box over it
    display.print(1);
  }

  if (FEDmode == 8 & RightCount != 0) { // Prog ratio, NOT first poke
    display.setCursor(5, 36);
    display.print("PR(R): ");
    display.setCursor(6, 36);
    display.print("PR(R): ");
    display.fillRoundRect (55, 24, 200, 55, 1, WHITE);  //erase the data on screen without clearing the entire screen by pasting a white box over it
    display.print(ratio - RightCount);
  }

  if (FEDmode == 9) {
    display.setCursor(5, 36);
    display.print("Stim");
  }

  if (FEDmode == 10) {
    display.setCursor(5, 36);
    display.print("Stim (R)");
  }

  if (FEDmode != 11 & FEDmode != 0) { // don't erase this if it's a free or timed feeding session
    display.fillRoundRect (35, 42, 130, 80, 1, WHITE);  //erase the pellet data on screen without clearing the entire screen by pasting a white box over it
  }

  display.fillRoundRect (93, 90, 70, 20, 1, WHITE);  //erase the pellet data on screen without clearing the entire screen by pasting a white box over it

  if (FEDmode > 0 & FEDmode != 11) {
    display.setCursor(35, 65);
    display.setTextSize(1);
    display.print("Left: ");
    display.setCursor(95, 65);
    display.print(LeftCount);

    display.setCursor(35, 85);
    display.setTextSize(1);
    display.print("Right:  ");
    display.setCursor(95, 85);
    display.print(RightCount);
  }

  if (FEDmode != 5 && FEDmode != 9 && FEDmode != 10) {  //don't show pellets if extinction or opto session
    display.setCursor(35, 105);
    display.setTextSize(1);
    display.print("Pellets:");
    display.setCursor(95, 105);
    display.print(PelletCount);
    //    display.print("  ");
    //    display.print(rewardedTrial);

  }

  //  Battery graphic showing bars indicating voltage levels
  //Clear battery area and draw outline of battery, only do this when numMotorTurns = 0 so it doesn't flicker;
  if (numMotorTurns == 0) {
    display.fillRoundRect (117, 2, 40, 16, 3, WHITE);
    display.drawRoundRect (116, 1, 42, 18, 3, BLACK);
    display.drawRoundRect (157, 6, 6, 8, 2, BLACK);
  }
  //4 bars
  if (measuredvbat > 3.85 & numMotorTurns == 0) {
    display.fillRoundRect (120, 4, 7, 12, 1, BLACK);
    display.fillRoundRect (129, 4, 7, 12, 1, BLACK);
    display.fillRoundRect (138, 4, 7, 12, 1, BLACK);
    display.fillRoundRect (147, 4, 7, 12, 1, BLACK);
  }

  //3 bars
  else if (measuredvbat > 3.7 & numMotorTurns == 0) {
    display.fillRoundRect (119, 3, 26, 13, 1, WHITE);
    display.fillRoundRect (120, 4, 7, 12, 1, BLACK);
    display.fillRoundRect (129, 4, 7, 12, 1, BLACK);
    display.fillRoundRect (138, 4, 7, 12, 1, BLACK);
  }

  //2 bars
  else if (measuredvbat > 3.55 & numMotorTurns == 0) {
    display.fillRoundRect (119, 3, 26, 13, 1, WHITE);
    display.fillRoundRect (120, 4, 7, 12, 1, BLACK);
    display.fillRoundRect (129, 4, 7, 12, 1, BLACK);
  }

  //1 bar
  else if (& numMotorTurns == 0) {
    display.fillRoundRect (119, 3, 26, 13, 1, WHITE);
    display.fillRoundRect (120, 4, 7, 12, 1, BLACK);
  }


  //Box around data area of screen
  display.drawRoundRect (5, 45, 158, 70, 3, BLACK);

  // Print date and time at bottom of the screen
  DateTime now = rtc.now();
  display.setCursor(10, 135);
  display.fillRoundRect (0, 123, 200, 60, 1, WHITE);
  display.print(now.month());
  display.print("/");
  display.print(now.day());
  display.print("/");
  display.print(now.year());
  display.print("    ");
  if (now.hour() < 10)
    display.print('0');      // Trick to add leading zero for formatting
  display.print(now.hour());
  display.print(":");
  if (now.minute() < 10)
    display.print('0');      // Trick to add leading zero for formatting
  display.print(now.minute());

  // Poke and pellet indicator graphics
  if (FEDmode > 0 & FEDmode != 11) {
    //poke indicators
    if (digitalRead(RIGHT_POKE) == HIGH) {
      display.fillCircle(25, 79, 5, WHITE);
      display.drawCircle(25, 79, 5, BLACK);
    }

    if (digitalRead(LEFT_POKE) == HIGH) {
      display.fillCircle(25, 59, 5, WHITE);
      display.drawCircle(25, 59, 5, BLACK);
    }

    //indicate which poke is active with a filled triangle beside it
    if (FEDmode == 7 || FEDmode == 8 || FEDmode == 10) {
      activePoke = 0;
    }

    if (activePoke == 0 && Ratio_Met == false) {
      display.fillTriangle (12, 55, 18, 59, 12, 63, WHITE);
      display.fillTriangle (12, 75, 18, 79, 12, 83, BLACK);
    }

    if (activePoke == 1 && Ratio_Met == false) {
      display.fillTriangle (12, 75, 18, 79, 12, 83, WHITE);
      display.fillTriangle (12, 55, 18, 59, 12, 63, BLACK);
    }
  }

  if (FEDmode != 5 && FEDmode != 9 && FEDmode != 10) { //no need to show pellets if extinction or opto stim sessions
    if (digitalRead(PELLET_WELL) == HIGH) {
      display.fillCircle(25, 99, 5, WHITE);
      display.drawCircle(25, 99, 5, BLACK);
    }

    if (digitalRead(PELLET_WELL) == LOW) {
      display.fillCircle(25, 99, 5, BLACK);
      PelletAvailable = true;
    }
  }
  display.refresh();
}

//Display "Check SD Card!" if there is a card error
void FED3::DisplaySDError() {
  display.clearDisplay();
  display.setRotation(3);
  display.setTextColor(BLACK);
  display.setCursor(20, 40);
  display.setTextSize(2);
  display.println("   Check");
  display.setCursor(10, 60);
  display.println("  SD Card!");
  display.refresh();
}

//Display text when FED is logging data
void FED3::DisplaySDLogging() {
  display.fillRoundRect (6, 20, 200, 22, 1, WHITE);  //erase the data on screen without clearing the entire screen by pasting a white box over it
  display.setCursor(6, 36);
  display.print("Writing data");
  display.refresh();
}

//Display text when FED is clearing a jam
void FED3::DisplayJamClear() {
  display.fillRoundRect (6, 20, 200, 22, 1, WHITE);  //erase the data on screen without clearing the entire screen by pasting a white box over it
  display.setCursor(6, 36);
  display.print("Clearing jam");
  display.refresh();
}

//Display when FED is dispensing
void FED3::DisplayDispense() {
  display.fillRoundRect (6, 20, 200, 22, 1, WHITE);  //erase the data on screen without clearing the entire screen by pasting a white box over it
  display.setCursor(6, 36);
  display.print("Dispensing...");
  display.refresh();
}

//Display when FED is sleepnig
void FED3::DisplaySleep() {
  display.fillRoundRect (6, 20, 200, 22, 1, WHITE);  //erase the data on screen without clearing the entire screen by pasting a white box over it
  display.setCursor(6, 36);
  display.print("Standby");
  if (digitalRead(PELLET_WELL) == LOW) {
    display.fillCircle(25, 99, 5, BLACK);
  }
  display.refresh();
}

//Display pellet retreival interval
void FED3::DisplayRetrievalInt() {
  if (retInterval < 120) { //only display a retrieval interval if it's less than 2 minutes
    display.setCursor(120, 105);
    display.print (retInterval);
    display.print("s");
    display.refresh();
  }
}

/********************************************************
  Classic menu display
********************************************************/
void FED3::ClassicMenu () {
  //  0 Free feeding
  //  1 FR1
  //  2 FR3
  //  3 FR5
  //  4 Progressive Ratio
  //  5 Extinction
  //  6 Light tracking FR1 task
  //  7 FR1 (reversed)
  //  8 PR (reversed)
  //  9 self-stim
  //  10 self-stim (reversed)
  //  11 time feeding

  // Set FR based on FEDmode
  if (FEDmode == 0) FR = 0;  // free feeding
  if (FEDmode == 1) FR = 1;  // FR1 spatial tracking task
  if (FEDmode == 2) FR = 3;  // FR3
  if (FEDmode == 3) FR = 5; // FR5
  if (FEDmode == 4) FR = 99;  // Progressive Ratio
  if (FEDmode == 5) { // Extinction
    FR = 1;
    ReleaseMotor ();
    digitalWrite (MOTOR_ENABLE, LOW);  //Disable motor driver
  }
  if (FEDmode == 6) FR = 1;  // Light tracking
  if (FEDmode == 7) FR = 1; // FR1 (reversed)
  if (FEDmode == 8) FR = 1; // PR (reversed)
  if (FEDmode == 9) FR = 1; // self-stim
  if (FEDmode == 10) FR = 1; // self-stim (reversed)

  display.setFont(&FreeSans9pt7b);
  display.setRotation(3);
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.clearDisplay();

  display.setCursor(1, 135);
  display.print(filename);

  display.setFont(&FreeSans9pt7b);
  display.setCursor(5, 20);
  display.println("Select Program");
  display.fillRoundRect(0, 30, 160, 80, 1, WHITE);
  display.setCursor(10, 45);
  //Text to display selected FR ratio
  if (FEDmode == 0) display.print("Free feeding");
  if (FEDmode == 11) display.print("Timed feeding");
  if (FEDmode == 1 ||  FEDmode == 2 || FEDmode == 3) {
    display.print("Fixed Ratio:"); display.print(FR);
  }
  if (FEDmode == 4) display.print("Progressive Ratio");
  if (FEDmode == 5) display.print("Extinction");
  if (FEDmode == 6) display.print("Light tracking");
  if (FEDmode == 7) display.print("FR1");
  if (FEDmode == 8) display.print("Progressive Ratio");
  if (FEDmode == 9) display.print("Self-Stimulation");
  if (FEDmode == 10) display.print("Self-Stimulation");
  if (FEDmode == 8 || FEDmode == 7 || FEDmode == 10) display.setCursor(10, 65);
  if (FEDmode == 8 || FEDmode == 7 || FEDmode == 10) display.print("(reversed)");

  for (int i = -50; i < 200; i += 15) {
    display.setCursor(0, 40);
    //Draw animated mouse...
    display.fillRoundRect (i + 25, 77, 15, 10, 6, BLACK);  //head
    display.fillRoundRect (i + 22, 75, 8, 5, 3, BLACK);    //ear
    display.fillRoundRect (i + 30, 79, 1, 1, 1, WHITE);    //eye

    //movement of the mouse
    if ((i / 10) % 2 == 0) {
      display.fillRoundRect (i, 79, 32, 17, 10, BLACK);      //body
      display.drawFastHLine(i - 8, 80, 18, BLACK);        //tail
      display.drawFastHLine(i - 8, 81, 18, BLACK);
      display.drawFastHLine(i - 14, 79, 8, BLACK);
      display.drawFastHLine(i - 14, 80, 8, BLACK);

      display.fillRoundRect (i + 22, 94, 8, 4, 3, BLACK);  //front foot
      display.fillRoundRect (i , 92, 8, 6, 3, BLACK); //back foot
    }
    else {
      display.fillRoundRect (i + 2, 77, 30, 17, 10, BLACK);    //body
      display.drawFastHLine(i - 6, 86, 18, BLACK);        //tail
      display.drawFastHLine(i - 6, 85, 18, BLACK);
      display.drawFastHLine(i - 12, 87, 8, BLACK);
      display.drawFastHLine(i - 12, 86, 8, BLACK);

      display.fillRoundRect (i + 15, 94, 8, 4, 3, BLACK);  //foot
      display.fillRoundRect (i + 8, 92, 8, 6, 3, BLACK); //back foot
    }
    delay (80);
    display.refresh();
    display.fillRoundRect (i - 25, 75, 90, 35, 1, WHITE);
    previousFEDmode = FEDmode;
    previousFED = FED;
    if (digitalRead (LEFT_POKE) == LOW | digitalRead (RIGHT_POKE) == LOW) SelectMode();
  }
  ReadBatteryLevel();
  display.clearDisplay();
  display.refresh();
}

/********************************************************
  SD card functions
********************************************************/
//Function to call for logging data
void FED3::logdata() {
  DisplaySDLogging();
  WriteToSD();
}

// Create new files on uSD for FED3 settings
void FED3::CreateFile() {
  // see if the card is present and can be initialized:
  if (!SD.begin(cardSelect, SD_SCK_MHZ(4))) {
    error(2);
  }

  // create files if they dont exist and grab device name and ratio
  configfile = SD.open("DeviceNumber.csv", FILE_WRITE);
  configfile = SD.open("DeviceNumber.csv", FILE_READ);
  FED = configfile.parseInt();
  configfile.close();

  ratiofile = SD.open("FEDmode.csv", FILE_WRITE);
  ratiofile = SD.open("FEDmode.csv", FILE_READ);
  FEDmode = ratiofile.parseInt();
  ratiofile.close();

  startfile = SD.open("start.csv", FILE_WRITE);
  startfile = SD.open("start.csv", FILE_READ);
  timedStart = startfile.parseInt();
  startfile.close();

  stopfile = SD.open("stop.csv", FILE_WRITE);
  stopfile = SD.open("stop.csv", FILE_READ);
  timedEnd = stopfile.parseInt();
  stopfile.close();

  // Name filename in format F###_MMDDYYNN, where MM is month, DD is day, YY is year, and NN is an incrementing number for the number of files initialized each day
  strcpy(filename, "FED_____________.CSV");  // placeholder filename
  getFilename(filename);
}

//Create a new datafile
void FED3::CreateDataFile () {
  getFilename(filename);
  logfile = SD.open(filename, FILE_WRITE);
  if ( ! logfile ) {
    error(3);
  }
}

//Write the header to the datafile
void FED3::writeHeader() {
  // Write data header to file of uSD.
  logfile.println("MM:DD:YYYY hh:mm:ss,FED_Version,Device_Number,Battery_Voltage,Motor_Turns,Session_Type,Event,Active_Poke,Left_Poke_Count,Right_Poke_Count,Pellet_Count,Retrieval_Time");
}

//write a configfile (this contains the FED device number)
void FED3::writeConfigFile() {
  configfile = SD.open("DeviceNumber.csv", FILE_WRITE);
  configfile.rewind();
  configfile.println(FED);
  configfile.flush();
  configfile.close();
}

//write a FEDmode file (this contains the last used FEDmode)
void FED3::writeFEDmode() {
  ratiofile = SD.open("FEDmode.csv", FILE_WRITE);
  ratiofile.rewind();
  ratiofile.println(FEDmode);
  ratiofile.flush();
  ratiofile.close();

  startfile = SD.open("start.csv", FILE_WRITE);
  startfile.rewind();
  startfile.println(timedStart);
  startfile.flush();
  startfile.close();

  stopfile = SD.open("stop.csv", FILE_WRITE);
  stopfile.rewind();
  stopfile.println(timedEnd);
  stopfile.flush();
  stopfile.close();
}

//Write to SD card
void FED3::WriteToSD() {
  // Print data and time followed by pellet count and motorturns to SD card
  DateTime now = rtc.now();
  logfile.print(now.month());
  logfile.print("/");
  logfile.print(now.day());
  logfile.print("/");
  logfile.print(now.year());
  logfile.print(" ");
  logfile.print(now.hour());
  logfile.print(":");
  if (now.minute() < 10)
    logfile.print('0');      // Trick to add leading zero for formatting
  logfile.print(now.minute());
  logfile.print(":");
  if (now.second() < 10)
    logfile.print('0');      // Trick to add leading zero for formatting
  logfile.print(now.second());
  logfile.print(",");

  logfile.print(VER); // Print device name
  logfile.print("sketch");
  logfile.print(sketch);
  logfile.print(",");

  logfile.print(FED); // Print device name
  logfile.print(",");

  logfile.print(measuredvbat); // Print battery voltage
  logfile.print(",");

  logfile.print((numJamClears * 10) + numMotorTurns); // Print the number of attempts to dispense a pellet, including through jam clears
  numMotorTurns = 0; //reset numMotorTurns
  numJamClears = 0; // reset numJamClears
  logfile.print(",");

  if (FEDmode == 4) {
    logfile.print("PR");
    logfile.print(round((5 * exp (0.2 * PelletCount)) - 5)); // Print current PR ratio
    logfile.print(",");
  }

  else if (FEDmode == 8) {
    logfile.print("PR_reversed");
    logfile.print(round((5 * exp (0.2 * PelletCount)) - 5)); // Print current PR ratio
    logfile.print(",");
  }

  else if (FEDmode == 0) {
    logfile.print("FED"); // Print trial type
    logfile.print(",");
  }

  else if (FEDmode == 5) {
    logfile.print("Extinction"); // Print trial type
    logfile.print(",");
  }

  else if (FEDmode == 6) {
    logfile.print("FR1_Light_tracking"); // Print trial type
    logfile.print(",");
  }

  else if (FEDmode == 7) {
    logfile.print("FR1_reversed"); // Print trial type
    logfile.print(",");
  }

  else if (FEDmode == 9) {
    logfile.print("Self_stim"); // Print trial type
    logfile.print(",");
  }

  else if (FEDmode == 10) {
    logfile.print("Self_stim_reversed"); // Print trial type
    logfile.print(",");
  }

  else if (FEDmode == 11) {
    logfile.print("Timed_"); // Print trial type
    logfile.print(timedStart); // Print trial type
    logfile.print("to"); // Print trial type
    logfile.print(timedEnd); // Print trial type
    logfile.print(",");
  }

  else {
    logfile.print("FR"); // Print ratio
    logfile.print(FR); // Print ratio
    logfile.print(",");
  }

  // Print event type
  if (pellet == true ) logfile.print("Pellet"); // If this event is a pellet retrieval print "Pellet"
  if (pellet == false ) logfile.print("Poke"); // If this event is not a pellet retrieval print "Poke"
  logfile.print(",");

  if (FEDmode == 6 || FEDmode == 7 || FEDmode == 8 || FEDmode == 10) {
    if (activePoke == 0)  logfile.print("Right"); //
    if (activePoke == 1)  logfile.print("Left"); //
  }
  else {
    logfile.print("Left");
  }
  logfile.print(",");

  logfile.print(LeftCount); // Print Left poke count
  logfile.print(",");

  logfile.print(RightCount); // Print Right poke count
  logfile.print(",");

  logfile.print(PelletCount); // print Pellet counts
  logfile.print(",");

  if (pellet  == false ) {
    logfile.println(sqrt (-1)); // print NaN if it's not a pellet line!
  }

  else if (retInterval < 120 ) {  // only log retrieval intervals below 2 minutes
    logfile.println(retInterval); // print interval between pellet dispensing and being taken
  }

  else if (retInterval >= 120) {
    logfile.println("Timed_out"); // print "Timed_out" if retreival interval is >120
  }

  else {
    logfile.println("Error"); // print error if value is < 0 (this shouldn't ever happen)
  }

  Blink(GREEN_LED, 100, 2);
  logfile.flush();
  // logfile.close();
}

// If any errors are detected with the SD card upon boot this function
// will blink both LEDs on the Feather M0, turn the NeoPixel into red wipe pattern,
// and display "Check SD Card" on the screen
void FED3::error(uint8_t errno) {
  DisplaySDError();
  while (1) {
    uint8_t i;
    for (i = 0; i < errno; i++) {
      Blink(GREEN_LED, 25, 2);
      colorWipe(strip.Color(5, 0, 0), 25); // RED
    }
    for (i = errno; i < 10; i++) {
      colorWipe(strip.Color(0, 0, 0), 25); // clear
    }
    CheckReset();
  }
}


// This function creates a unique filename for each file that
// starts with "FED", then the date in MMDDYY,
// then an incrementing number for each new file created on the same date
void FED3::getFilename(char *filename) {
  DateTime now = rtc.now();

  filename[3] = FED / 100 + '0';
  filename[4] = FED / 10 + '0';
  filename[5] = FED % 10 + '0';
  filename[7] = now.month() / 10 + '0';
  filename[8] = now.month() % 10 + '0';
  filename[9] = now.day() / 10 + '0';
  filename[10] = now.day() % 10 + '0';
  filename[11] = (now.year() - 2000) / 10 + '0';
  filename[12] = (now.year() - 2000) % 10 + '0';
  filename[16] = '.';
  filename[17] = 'C';
  filename[18] = 'S';
  filename[19] = 'V';
  for (uint8_t i = 0; i < 100; i++) {
    filename[14] = '0' + i / 10;
    filename[15] = '0' + i % 10;

    if (! SD.exists(filename)) {
      break;
    }
  }
  return;
}

// Set FEDMode
void FED3::SelectMode() {
  // Mode select on startup screen

  // FEDmodes:
  // 0 Free feeding
  // 1 FR1
  // 2 FR3
  // 3 FR5
  // 4 Progressive Ratio
  // 5 Extinction
  // 6 Light tracking FR1 task
  // 7 FR1 (reversed)
  // 8 PR (reversed)
  // 9 Optogenetic stimulation
  // 10 Optogenetic stimulation (reversed)
  // 11 Timed free feeding

  // Set FR based on FEDmode
  if (FEDmode == 0 || FEDmode == 11) FR = 0;  // free feeding or timed feeding
  if (FEDmode == 1 || FEDmode == 7) FR = 1; // FR1 spatial task
  if (FEDmode == 2) FR = 3;  // FR3
  if (FEDmode == 3) FR = 5; // FR5
  if (FEDmode == 4 || FEDmode == 8) FR = 99; // Progressive Ratio
  if (FEDmode == 5 || FEDmode == 9 || FEDmode == 10) { // Extinction or optogenetic
    FR = 1;
    ReleaseMotor ();
    digitalWrite (MOTOR_ENABLE, LOW);  //Disable motor driver
  }
  if (FEDmode == 6) FR = 1; // FR1 light tracking task

  previousFEDmode = FEDmode;

  if ((digitalRead(LEFT_POKE) == LOW) && (digitalRead(RIGHT_POKE) == LOW)) {
    tone (BUZZER, 3000, 500);
    colorWipe(strip.Color(2, 2, 2), 40); // Color wipe
    colorWipe(strip.Color(0, 0, 0), 20); // OFF
    EndTime = millis();
    SetFED = true;
    SetDeviceNumber();
  }

  //Set FEDMode
  else if (digitalRead(LEFT_POKE) == LOW) {
    EndTime = millis();
    FEDmode -= 1;
    tone (BUZZER, 2500, 200);
    colorWipe(strip.Color(2, 0, 2), 40); // Color wipe
    colorWipe(strip.Color(0, 0, 0), 20); // OFF
    if (FEDmode == -1) FEDmode = 11;
  }

  else if (digitalRead(RIGHT_POKE) == LOW) {
    EndTime = millis();
    FEDmode += 1;
    tone (BUZZER, 2500, 200);
    colorWipe(strip.Color(2, 2, 0), 40); // Color wipe
    colorWipe(strip.Color(0, 0, 0), 20); // OFF
    if (FEDmode == 12) FEDmode = 0;
  }

  if (FEDmode < 0) FEDmode = 0;
  if (FEDmode > 11) FEDmode = 11;

  display.setTextSize(1);
  display.setCursor(5, 20);
  display.println ("Select Program");
  display.fillRoundRect (0, 30, 160, 80, 1, WHITE);
  display.setCursor(10, 45);
  //Text to display selected FR ratio
  if (FEDmode == 0) display.print("Free feeding");
  if (FEDmode == 1 ||  FEDmode == 2 || FEDmode == 3) {
    display.print("Fixed Ratio:"); display.print(FR);
  }
  if (FEDmode == 4) display.print("Progressive Ratio");
  if (FEDmode == 5) display.print("Extinction");
  if (FEDmode == 6) display.print("Light tracking");
  if (FEDmode == 7) display.print("FR1");
  if (FEDmode == 8) display.print("Progressive Ratio");
  if (FEDmode == 9 || FEDmode == 10) display.print("Self-Stimulation");
  if (FEDmode == 8 || FEDmode == 7 || FEDmode == 10) display.setCursor(10, 65);
  if (FEDmode == 8 || FEDmode == 7 || FEDmode == 10) display.print("(reversed)");
  if (FEDmode == 11) display.print("Timed feeding");

  display.refresh();

  while (millis() - EndTime < 2000) {
    SelectMode();
  }

  display.setCursor(5, 90);
  display.println("...Selected!");
  delay (500);
  display.refresh();

  writeFEDmode();
  delay (200);
  NVIC_SystemReset();      // processor software reset
}

// Change device number
void FED3::SetDeviceNumber() {
  // This code is activated when both pokes are pressed simultaneously
  // from the start screen, allowing the user to set the device #
  // of the FED right from the device

  while (SetFED == true) {
    //adjust FED device number
    display.fillRoundRect (0, 0, 200, 80, 0, WHITE);
    display.setCursor(5, 46);
    display.println("Set Device Number");

    display.fillRoundRect (36, 122, 30, 18, 0, WHITE);
    display.fillRoundRect (65, 122, 140, 18, 0, WHITE);

    delay (100);
    display.refresh();

    display.setCursor(35, 135);
    if (FED < 100 & FED >= 10) {
      display.print ("0");
    }
    if (FED < 10) {
      display.print ("00");
    }
    display.print (FED);

    delay (100);
    display.refresh();

    if (digitalRead(RIGHT_POKE) == LOW) {
      FED += 1;
      EndTime = millis();
      if (FED > 700) {
        FED = 700;
      }
    }

    if (digitalRead(LEFT_POKE) == LOW) {
      FED -= 1;
      EndTime = millis();
      if (FED < 1) {
        FED = 0;
      }
    }
    if (millis() - EndTime > 3000) {  // if 3 seconds passes confirm device #
      SetFED = false;
      setTimed = true;
      display.setCursor(5, 70);
      display.println("...Set!");
      delay (500);
      display.refresh();
      EndTime = millis();

      while (setTimed == true) {
        // set timed feeding start and stop
        display.fillRoundRect (5, 56, 120, 18, 0, WHITE);
        delay (200);
        display.refresh();

        display.fillRoundRect (0, 0, 200, 80, 0, WHITE);
        display.setCursor(5, 46);
        display.println("Set Timed Feeding");
        display.setCursor(15, 70);
        display.print(timedStart);
        display.print(":00 - ");
        display.print(timedEnd);
        display.print(":00");
        delay (50);
        display.refresh();

        if (digitalRead(LEFT_POKE) == LOW) {
          timedStart += 1;
          EndTime = millis();
          if (timedStart > 24) {
            timedStart = 0;
          }
          if (timedStart > timedEnd) {
            timedEnd = timedStart + 1;
          }
        }

        if (digitalRead(RIGHT_POKE) == LOW) {
          timedEnd += 1;
          EndTime = millis();
          if (timedEnd > 24) {
            timedEnd = 0;
          }
          if (timedStart > timedEnd) {
            timedStart = timedEnd - 1;
          }
        }
        if (millis() - EndTime > 3000) {  // if 3 seconds passes confirm time settings
          setTimed = false;
          display.setCursor(5, 95);
          display.println("...Timing set!");
          delay (1000);
          display.refresh();
        }
      }
      writeFEDmode();
      writeConfigFile();
      NVIC_SystemReset();      // processor software reset

    }
  }
}

//Timeout function
void FED3::Timeout() {
  if (TimeoutReady == true && PelletAvailable == true) {
    for (int k = 0; k <= timeout; k++) {
      delay (1000);
      display.fillRoundRect (5, 20, 100, 25, 1, WHITE);  //erase the data on screen without clearing the entire screen by pasting a white box over it
      display.setCursor(6, 36);
      display.print("Timeout: ");
      display.print(timeout - k);
      display.refresh();

      Serial.println();
      Serial.println ("Timeout:");
      Serial.print(timeout - k);
      Serial.print(" sec");
      Serial.println();
    }
    TimeoutReady = false;
    display.fillRoundRect (5, 20, 100, 25, 1, WHITE);  //erase the data on screen without clearing the entire screen by pasting a white box over it
    UpdateDisplay();
  }
}

//Read battery level
void FED3::ReadBatteryLevel() {
  measuredvbat = analogRead(VBATPIN);
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  measuredvbat /= 1024; // convert to voltage
}

//Function for cutting off the Feed loop if a pellet is detected
void FED3::dispenseTimer() {
  for (int i = 1; i < 300; i++) {
    if (digitalRead (PELLET_WELL) == HIGH && PelletAvailable == false) {
      delay(10);
    }
  }
}

//What happens when pellet is detected
void FED3::pelletTrigger() {
  if (digitalRead(PELLET_WELL) == LOW) {
    PelletAvailable = true;
  }
  else {
    PelletAvailable = false;
    BNC_out(500,1);
  }
}

//What happens when left poke is poked
void FED3::leftTrigger() {
  if (digitalRead(LEFT_POKE) == HIGH) {
    Left = false;
  }
  else {
    Left = true;
  }
}

//What happens when right poke is poked
void FED3::rightTrigger() {
  if (digitalRead(RIGHT_POKE) == HIGH) {
    Right = false;
  }
  else {
    Right = true;
  }
}

/**********************************************
  Functions to control processor sleeping
**********************************************/
void FED3::goToSleep () {
  ReleaseMotor();
  UpdateDisplay();
  if (EnableSleep==true){
    LowPower.sleep(1000);  //Comment this out if you want to use the Serial connection, LowPower.sleep breaks it
  }
}

//Pull all motor pins low to de-energize stepper and save power, also disable motor driver with the EN pin
void FED3::ReleaseMotor () {
  digitalWrite(A2, LOW);
  digitalWrite(A3, LOW);
  digitalWrite(A4, LOW);
  digitalWrite(A5, LOW);
  digitalWrite(MOTOR_ENABLE, LOW);  //Disable motor driver
}

/********************************************************
  initialize FED3 object
********************************************************/
FED3::FED3(int rev) {
  if (rev) {
    sketch = rev;
  }
}

/********************************************************
  Startup settings, to be run in Arduino setup()
********************************************************/
//  dateTime function
void dateTime(uint16_t* date, uint16_t* time) {
  DateTime now = rtc.now();
  // return date using FAT_DATE macro to format fields
  *date = FAT_DATE(now.year(), now.month(), now.day());

  // return time using FAT_TIME macro to format fields
  *time = FAT_TIME(now.hour(), now.minute(), now.second());
}

void FED3::versionDisplay(){
  display.setFont(&FreeSans9pt7b);
  display.setRotation(3);
  display.setTextSize(3);
  display.setTextColor(BLACK);
  display.clearDisplay();
  display.setCursor(15, 60);
  display.print("FED3");
 
  display.setTextSize(1);
  display.setCursor(1, 135);
  display.print("v: ");
  display.print(VER);
  display.print("_sketch");
  display.print(sketch);
  display.refresh();
    
  //Display FED verison number at startup
  for (int i = -50; i < 200; i += 15) {
    //Draw animated mouse...

    display.fillRoundRect (i + 25, 77, 15, 10, 6, BLACK);    //head
    display.fillRoundRect (i + 22, 75, 8, 5, 3, BLACK);      //ear
    display.fillRoundRect (i + 30, 79, 1, 1, 1, WHITE);      //eye

    //movement of the mouse
    if ((i / 10) % 2 == 0) {
      display.fillRoundRect (i, 79, 32, 17, 10, BLACK);      //body
      display.drawFastHLine(i - 8, 80, 18, BLACK);           //tail
      display.drawFastHLine(i - 8, 81, 18, BLACK);
      display.drawFastHLine(i - 14, 79, 8, BLACK);
      display.drawFastHLine(i - 14, 80, 8, BLACK);

      display.fillRoundRect (i + 22, 94, 8, 4, 3, BLACK);    //front foot
      display.fillRoundRect (i , 92, 8, 6, 3, BLACK);        //back foot
    }
    else {
      display.fillRoundRect (i + 2, 77, 30, 17, 10, BLACK);  //body
      display.drawFastHLine(i - 6, 86, 18, BLACK);           //tail
      display.drawFastHLine(i - 6, 85, 18, BLACK);
      display.drawFastHLine(i - 12, 87, 8, BLACK);
      display.drawFastHLine(i - 12, 86, 8, BLACK);

      display.fillRoundRect (i + 15, 94, 8, 4, 3, BLACK);    //foot
      display.fillRoundRect (i + 8, 92, 8, 6, 3, BLACK);     //back foot
    }

    display.refresh();
    delay (80);
    display.fillRoundRect (i - 25, 75, 80, 35, 1, WHITE);
  }
}

void FED3::begin() {
  Serial.begin(9600);
  
  // Initialize pins
  pinMode(PELLET_WELL, INPUT);
  pinMode(LEFT_POKE, INPUT);
  pinMode(RIGHT_POKE, INPUT);
  pinMode(MOTOR_ENABLE, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(A2, OUTPUT);
  pinMode(A3, OUTPUT);
  pinMode(A4, OUTPUT);
  pinMode(A5, OUTPUT);
  pinMode(BNC_OUT, OUTPUT);

  // Initialize RTC
  rtc.begin();

  // Initialize Neopixels
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  // Initialize stepper
  stepper.setSpeed(12);

  // Initialize display
  display.begin();
  const int minorHalfSize = min(display.width(), display.height()) / 2;
  display.setFont(&FreeSans9pt7b);
  versionDisplay();

  // Initialize SD card and create the datafile
  SdFile::dateTimeCallback(dateTime);
  CreateFile();

  // Initialize interrupts
  pointerToFED3 = this;
  LowPower.attachInterruptWakeup(digitalPinToInterrupt(PELLET_WELL), outsidePelletTriggerHandler, CHANGE);
  LowPower.attachInterruptWakeup(digitalPinToInterrupt(LEFT_POKE), outsideLeftTriggerHandler, CHANGE);
  LowPower.attachInterruptWakeup(digitalPinToInterrupt(RIGHT_POKE), outsideRightTriggerHandler, CHANGE);

  // Create data file for current session
  CreateDataFile();
  writeHeader();
  EndTime = 0;
  
  // Clear display
  display.clearDisplay();
  display.refresh();

}