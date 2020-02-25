#include <MozziGuts.h>
#include <Oscil.h>
#include <LiquidCrystal.h>
#include <mozzi_fixmath.h>
#include <mozzi_midi.h>
#include <RollingAverage.h>
#include <ADSR.h>

/*
   AUDIO INCLUDES
*/

#include <tables/sin2048_int8.h>
#include <tables/cos2048_int8.h>
#include <tables/square_no_alias_2048_int8.h>
#include <tables/triangle_hermes_2048_int8.h>

/*
   DEFINES
*/

#define CONTROL_RATE 128

/*
   PIN DECLARATIONS
*/

// LCD
const int rsPin = 53;
const int ePin = 52;
const int d4Pin = 43;
const int d5Pin = 42;
const int d6Pin = 41;
const int d7Pin = 40;

// JOY
const int joyXPin = A8;
const int joyYPin = A9;
const int joySWPin = 3;

// LASERS

// PHOTORESISTORS
const int photoResistorPin1 = A10;
const int photoResistorPin2 = A11;
const int photoResistorPin3 = A6;
const int photoResistorPin4 = A7;

// DISTANCE SENSOR
const int trigPin = 2;
const int echoPin = 4;

// TESTING ONLY
// BUTTONS
const int buttonPin1 = 7;
const int buttonPin2 = 8;
const int buttonPin3 = 9;
const int buttonPin4 = 6;

// SOUND PARAMETERS

int ATTACK = 10;
int DECAY = 128;
int SUSTAIN = 128;
int RELEASE = 0;
int ATTACK_LEVEL = 255;
int DECAY_LEVEL = 200;
int octaveShift = 48 ;
int gain = 255;

/*
   GLOBALS
*/
LiquidCrystal lcd(rsPin, ePin, d4Pin, d5Pin, d6Pin, d7Pin);

int xAxis, yAxis, sw;

/*
   AUDIO
*/

Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin(SQUARE_NO_ALIAS_2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin2(SQUARE_NO_ALIAS_2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin3(SQUARE_NO_ALIAS_2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin4(SQUARE_NO_ALIAS_2048_DATA);
Oscil <SQUARE_NO_ALIAS_2048_NUM_CELLS, AUDIO_RATE> aSquare(SQUARE_NO_ALIAS_2048_DATA);
Oscil <COS2048_NUM_CELLS, AUDIO_RATE> aCos(COS2048_DATA);

ADSR<CONTROL_RATE, AUDIO_RATE> env;

RollingAverage <int, 16> distAvg;


int midiTable[5][4] = {
  69, 72, 76, 79,
  71, 74, 77, 81,
  72, 76, 79, 83,
  74, 77, 81, 84,
  76, 79, 83, 86
};

float freqTable[5][4];

/*
 * DIST SENSOR
 */

int duration;
int dist;



byte currentGain = 0;
byte currentGain1 = 0;
byte currentGain2 = 0;
byte currentGain3 = 0;
byte currentGain4 = 0;

/*
 * BUTTONS
 */

int buttonValue1;
int buttonValue2;
int buttonValue3;
int buttonValue4;

/*
   MENU
*/
unsigned int cursorPosition = 0;
bool menuChanged = false;
const int noLevels = 7;

const char menuStrings[noLevels][16] = {
  "Guitarduino",
  "Gain",
  "Attack",
  "Decay",
  "Sustain",
  "Release",
  "Octave"
};

int* menuLevels[noLevels] = {
  NULL,
  &gain,
  &ATTACK,
  &DECAY,
  &SUSTAIN,
  &RELEASE,
  &octaveShift
};

unsigned int mozziStartDelay = 100;
unsigned long lastMenuUpdate;
bool mozziStopped = false;



void printMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(menuStrings[cursorPosition]);

  if (cursorPosition) {
    lcd.setCursor(8, 0);

    lcd.print(*menuLevels[cursorPosition]);
  }

  lcd.setCursor(0, 1);
  lcd.print(menuStrings[cursorPosition + 1]);

  if (cursorPosition + 1) {
    lcd.setCursor(8, 1);
    lcd.print(*menuLevels[cursorPosition + 1]);
  }

  lcd.setCursor(0, 0);

  lastMenuUpdate = millis();

//  env.setTimes(2000, DECAY, SUSTAIN, 255);
}

bool xDown = true;
bool yDown = true;

const int lowThreshold = 200;
const int highThreshold = 800;

void controlMenu() {
  if (xAxis > highThreshold && cursorPosition && !xDown) {
    (*(menuLevels[cursorPosition]))++;
    menuChanged = true;
    xDown = true;
  }
  else if (xAxis < lowThreshold && cursorPosition && !xDown) {
    (*(menuLevels[cursorPosition]))--;
    menuChanged = true;
    xDown = true;
  }
  else if (xAxis > lowThreshold && xAxis < highThreshold) {
    xDown = false;
  }

  if (yAxis > highThreshold && !yDown) {
    cursorPosition++;
    if (cursorPosition >= noLevels) {
      cursorPosition = 0;
    }
    menuChanged = true;
    yDown = true;
  }
  else if (yAxis < lowThreshold && !yDown) {
    cursorPosition--;
    if (cursorPosition < 0) {
      cursorPosition = noLevels - 1;
    }
    menuChanged = true;
    yDown = true;
  }
  else if (yAxis > lowThreshold && yAxis < highThreshold) {
    yDown = false;
  }
}

bool noteStopped = true;



void setup() {
  startMozzi(CONTROL_RATE);

  // PINS

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  pinMode(joyXPin, INPUT);
  pinMode(joyYPin, INPUT);
  pinMode(joySWPin, INPUT_PULLUP);

  pinMode(photoResistorPin1, INPUT);
  pinMode(photoResistorPin2, INPUT);
  pinMode(photoResistorPin3, INPUT);
  pinMode(photoResistorPin4, INPUT);

  // TESTING ONLY
//  pinMode(buttonPin1, INPUT_PULLUP);
//  pinMode(buttonPin2, INPUT_PULLUP);
//  pinMode(buttonPin3, INPUT_PULLUP);
//  pinMode(buttonPin4, INPUT_PULLUP);

  // LCD
  lcd.begin(16, 2);

  lcd.cursor();

  printMenu();

  aSin.setFreq(440);
  aSquare.setFreq(440);

  // CALCULATE MIDI -> FREQ
  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < 4; j++) {
      freqTable[i][j] = mtof(midiTable[i][j] - octaveShift);
    }
  }

  env.setLevels(64, 52, 50, 0);
  env.setTimes(50, DECAY, 3000, 300);
//  env.setReleaseTime(2000);
//  env.setAttackTime(1000);
//  env.setSustainTime(500);
//  env.setReleaseTime(3000);
}



void updateControl() {
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(100);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  dist = (duration/2) / 29.1;

//  Serial.println(dist);

   buttonValue1 = mozziAnalogRead(photoResistorPin1);
  buttonValue2 = mozziAnalogRead(photoResistorPin2);
  buttonValue3 = mozziAnalogRead(photoResistorPin3);
  buttonValue4 = mozziAnalogRead(photoResistorPin4);

  env.update();

  Serial.println(buttonValue3);

  if (buttonValue1 < 850 && noteStopped) {
    env.noteOn();
    currentGain = 255;
    noteStopped = false;

  }

  if (buttonValue2 < 850 && noteStopped) {
    env.noteOn();
    currentGain = 255;
    noteStopped = false;
  }

//  if (buttonValue3 < 850 && noteStopped) {
//    env.noteOn();
//    currentGain = 255;
//    noteStopped = false;
//  }

  if (buttonValue4 < 600 && noteStopped) {
    env.noteOn();
    currentGain = 255;
    noteStopped = false;
  }

  if (buttonValue1 > 850 && buttonValue2 > 850 && buttonValue4 > 600) {
    env.noteOff();
    noteStopped = true;
    
   
  }

  if (currentGain > 3) {
      currentGain -= 4;
    }
    else {
      currentGain = 0;
    }

  xAxis = mozziAnalogRead(joyXPin);
  yAxis = mozziAnalogRead(joyYPin);
  sw = digitalRead(joySWPin);
  
  dist = distAvg.next(dist);

  if (dist <= 4 && (env.playing() || noteStopped)) {
    // ROOT A4
    aSin.setFreq(freqTable[4][0]);
    aSquare.setFreq(freqTable[0][0]);

    aSin2.setFreq(freqTable[4][1]);
    aSin3.setFreq(freqTable[4][2]);
    aSin4.setFreq(freqTable[4][3]);
  }
  else if (dist <= 7 && (env.playing() || noteStopped)) {
    // ROOT B4
    aSin.setFreq(freqTable[3][0]);
    aSquare.setFreq(freqTable[1][0]);

    aSin2.setFreq(freqTable[3][1]);
    aSin3.setFreq(freqTable[3][2]);
    aSin4.setFreq(freqTable[3][3]);
  }
  else if (dist <= 10 && (env.playing() || noteStopped)) {
    // ROOT C4
    aSin.setFreq(freqTable[2][0]);
    aSquare.setFreq(freqTable[2][0]);

    aSin2.setFreq(freqTable[2][1]);
    aSin3.setFreq(freqTable[2][2]);
    aSin4.setFreq(freqTable[2][3]);
  }
  else if (dist <= 13 && (env.playing() || noteStopped)) {
    // ROOT D4
    aSin.setFreq(freqTable[1][0]);
    aSquare.setFreq(freqTable[3][0]);

    aSin2.setFreq(freqTable[1][1]);
    aSin3.setFreq(freqTable[1][2]);
    aSin4.setFreq(freqTable[1][3]);
  }
  else if (dist <= 16 && (env.playing() || noteStopped)) {
    // ROOT E4
    aSin.setFreq(freqTable[0][0]);
    aSquare.setFreq(freqTable[4][0]);

    aSin2.setFreq(freqTable[0][1]);
    aSin3.setFreq(freqTable[0][2]);
    aSin4.setFreq(freqTable[0][3]);
  }

  
 

}

int updateAudio() {
//  return ((int) aSin.next() * currentGain1) >> 8;
//  return (((int) aSin.next()) * currentGain1)>>8;
//      return (3*((long)aSin.next() * currentGain1 + aSin2.next() * currentGain2 +((aSin3.next() * currentGain3)>>1)
//    +((aSin4.next() * currentGain4)>>2)) >>3)>>8;

  int currentSample = 0;

  if (buttonValue1 < 850) {
    currentSample += aSin.next();
  }
  else if (buttonValue2 < 850) {
    currentSample += aSin2.next();
  }
  else if (buttonValue3 < 850) {
    currentSample += aSin3.next();
  }
  else if (buttonValue4 < 850) {
    currentSample += aSin4.next();
  }
  
  currentSample *= env.next();

  return (((int) currentSample>>6));
}

void loop() {
  

  controlMenu();

  if (menuChanged) {
    stopMozzi();
    mozziStopped = true;
    printMenu();
    menuChanged = false;
    startMozzi(CONTROL_RATE);
  }
  
  audioHook();
}
