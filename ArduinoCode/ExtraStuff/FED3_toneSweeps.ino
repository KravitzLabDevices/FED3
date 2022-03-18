/*
 * Example code to run auditory sweeping stimuli on FED3
 * January 2022
 * alexxai@wustl.edu
*/

int tonePin = 14;  //Set to 0 for internal beeper, or 14 for output connection

void setup ()
{
  pinMode (tonePin, OUTPUT);
}

void loop() {
  int i;

  //Fast up sweep from 1Khz to 8Khz
  for (i = 1000; i <= 8000; i =  i + 100) {
    tone(tonePin , i);
    delay(2);
  }
  noTone(tonePin);
  delay (1000);

  //Fast down sweep from 8Khz to 1Khz
  for (i = 8000; i >= 1000; i = i - 100) {
    tone(tonePin, i);
    delay(2);
  }
  noTone(tonePin);
  delay (1000);

  //Slow up sweep from 1Khz to 8Khz
  for (i = 1000; i <= 8000; i =  i + 100) {
    tone(tonePin, i);
    delay(6);
  }
  noTone(tonePin);
  delay (1000);

  //Slow down sweep from 8Khz to 1Khz
  for (i = 8000; i >= 1000; i = i - 100) {
    tone(tonePin, i);
    delay(6);
  }
  noTone(tonePin);
  delay (1000);
}
