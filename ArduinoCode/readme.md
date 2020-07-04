Once you have the FED3 electronics it's time to update and modify the code!  To do so, follow these steps (also available with pictures [here](https://hackaday.io/project/106885/instructions)):

1. Install the [Arduino IDE](arduino.cc).  Install the latest version.
2. Read about the [Adafruit M0 Adalogger](https://learn.adafruit.com/adafruit-feather-m0-adalogger/) microcontroller that is inside FED3.  TL/DR: You need to perform a [few updates](https://learn.adafruit.com/adafruit-feather-m0-adalogger/setup) to the Arduino IDE to be able to communicate with this board. 
2b. IMPORTANT: There is an incompatibility between the FED code and Adafruit SAMD board packages >1.5.5.  We don't know why, but while we try to figure it out, just install version 1.5.5 of the Adafruit SAMD Board package.
3. Install the relevant libraries. The  simplest will be to download the prepared zip file (FED3libraries.zip above).  Extract these libraries to your Arduino libraries director, for most people this will be: /Documents/Arduino/libraries
4. Set the real time clock (RTC) on the FED3.  Open the FED3_SetClock sketch above in the Arduino IDE and flash this code to the M0.  You should see a message on the screen with the correct local time, indicating that the RTC was set correctly. 
5. Flash the latest FED3 code above. You should now be able to use FED3!
