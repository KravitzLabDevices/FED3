/*
  Feeding experimentation device 3 (FED3)
  Classic FED3 script
  This script mimicks the class FED3, complete with menuing system for selecting programs

  alexxai@wustl.edu
  August, 2020
*/

#include <FED3.h>                //Include the FED3 library 
int sketch = 1;                  //Name the sketch with a unique integer
FED3 fed3 (sketch);              //Start the FED3 object

void setup() {
  fed3.begin();                  //Setup the FED3 hardware
  fed3.EnableSleep=false;                               //Set to false to inhibit sleeping to use the Serial port; Set to true to reduce battery power
  fed3.ClassicMenu();            //Run the classic FED3 menu
}

void loop() {
  fed3.run();                    //Call fed.run at least once per loop

////////////////////////////////////////////////////
// Write your behavioral program below this line  //
////////////////////////////////////////////////////

if (fed3.Ratio_Met) {            // If the ratio has been met (this depends on the program)
    fed3.ConditionedStimulus();  // Play conditioned stimulus (tone and lights)
    fed3.Feed();                 // Drop pellet

    ////////////////////////////////////////////////////
    // Use Serial.print statements for debugging
    ////////////////////////////////////////////////////
    Serial.println("Pellets   RightPokes   LeftPokes");
    Serial.print("   ");
    Serial.print(fed3.PelletCount);
    Serial.print("          ");
    Serial.print(fed3.RightCount);
    Serial.print("          ");
    Serial.println(fed3.LeftCount);
    Serial.println(" ");
  }
}
