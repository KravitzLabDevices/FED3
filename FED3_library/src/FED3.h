/*
Feeding experimentation device 3 (FED3) library version 1.0.1
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

#ifndef FED3_H
#define FED3_H

#define VER "1.0.1"

#include <Adafruit_BusIO_Register.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_I2CRegister.h>
#include <Adafruit_SPIDevice.h>
#include <Wire.h>
#include <SPI.h>
#include <Stepper.h>
#include <ArduinoLowPower.h>
#include "RTClib.h"  
#include <SdFat.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SharpMem.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Adafruit_NeoPixel.h>

// Pin definitions
#define NEOPIXEL        A1   // Neopixel
#define MOTOR_ENABLE    13
#define GREEN_LED       8
#define PELLET_WELL     1
#define LEFT_POKE       6
#define RIGHT_POKE      5
#define BUZZER          0    // 0 to activate beeper, 99 to turn it off and stop annoying people on the train
#define VBATPIN         A7
#define cardSelect      4
#define BNC_OUT         A0
#define SHARP_SCK       12   // Sharp memory display
#define SHARP_MOSI      11   // Sharp memory display
#define SHARP_SS        10   // Sharp memory display

#define BLACK 0
#define WHITE 1
#define STEPS 2038

extern bool Left;

class FED3 {
    // Members
    public:
        FED3(int rev = 0);
        void begin();
        void classInterruptHandler(void);
        void run();

        // SD logging
        SdFat SD;
        File logfile;       // Create file object
        File ratiofile;     // Create another file object
        File configfile;    // Create another file object
        File startfile;     // Create another file object
        File stopfile;      // Create another file object
        char filename[15];  // Array for file name data logged to named in setup
        void logdata();
        void CreateFile();
        void CreateDataFile ();
        void writeHeader();
        void writeConfigFile();
        void writeFEDmode();
        void WriteToSD();
        void error(uint8_t errno);
        void getFilename(char *filename);

        // Battery
        float measuredvbat = 1.00;
        void ReadBatteryLevel();

        // Neopixel
        void colorWipe(uint32_t c, uint8_t wait);
        void RcolorWipe(uint32_t c, uint8_t wait);
        void Blink(byte PIN, byte DELAY_MS, byte loops);
        void BNC_out(byte DELAY_MS, byte loops);

        // Display functions
        void UpdateDisplay();
        void DisplaySDError();
        void DisplaySDLogging();
        void DisplayJamClear();
        void DisplayDispense();
        void DisplaySleep();
        void DisplayRetrievalInt();
        
        // Startup menu function
        void ClassicMenu();
        void versionDisplay();

        // Motor
        void ReleaseMotor();
        int numMotorTurns = 0;
        int numJamClears = 0;

        // Set FED
        void SelectMode();
        void SetDeviceNumber();

        // Stimuli
        void ConditionedStimulus();
        void RConditionedStim();
        void ErrorStim();
        void leftStimulus();
        void rightStimulus();
        
        // Functions
        void CheckRatio();
        void CheckPokes();
        void CheckReset();
        void Feed();
        void dispenseTimer();
        void pelletTrigger();
        void leftTrigger();
        void rightTrigger();
        void goToSleep();
        void Timeout();
        
        //jam movements
        void ClearJam();
        void VibrateJam();
        void MinorJam();

        //Timeout duration in seconds
        const byte timeout = 0; //timeout between trials in seconds
        int timedStart = 18; //hour to start the timed Feeding session, out of 24 hour clock
        int timedEnd = 21; //hour to start the timed Feeding session, out of 24 hour clock

        // mode variables
        int FED;
        int FR = 1;
        byte FEDmode;
        byte previousFEDmode = FEDmode;

        // event counters
        int LeftCount = 0;
        int RightCount = 0;
        int PelletCount = 0;

        // state variables
        bool activePoke = 1;  // 0 for right, 1 for left, defaults to left poke active
        bool Left = false;
        bool Right = false;
        bool PelletAvailable = false;
        bool pellet = false;  // this true for pellet event, false for poke event

        // pellet stats
        int retInterval = 0;
        int pelletTime = 0;

        // flags
        bool CountReady = false;
        bool LeftReady = false;
        bool RightReady = false;
        bool TimeoutReady = true;
        bool OutReady = false;
        bool Ratio_Met = false;
        bool EnableSleep = true;

        int EndTime = 0;
        int ratio = 1;
        int sketch = 0;
        int previousFR = FR;
        int previousFED = FED;

        bool SetFED = false;
        bool setTimed = false;
        
        // Neopixel strip
        Adafruit_NeoPixel strip = Adafruit_NeoPixel(8, NEOPIXEL, NEO_GRBW + NEO_KHZ800);
        // Display
        Adafruit_SharpMem display = Adafruit_SharpMem(SHARP_SCK, SHARP_MOSI, SHARP_SS, 144, 168);
        // Stepper
        Stepper stepper = Stepper(STEPS, A2, A3, A4, A5);

    private:
        static FED3* staticFED;
        static void updatePelletTriggerISR();
        static void updateLeftTriggerISR();
        static void updateRightTriggerISR();
};

#endif