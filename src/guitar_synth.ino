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

//  These are the used sounds
#include <tables/sin2048_int8.h>
#include <tables/cos2048_int8.h>
#include <tables/square_no_alias_2048_int8.h>
#include <tables/triangle_hermes_2048_int8.h>

/*
   DEFINES
*/

#define CONTROL_RATE 128

// calibrate these depending on your setup
#define PHOTORES1_TOLERANCE 850
#define PHOTORES2_TOLERANCE 850
#define PHOTORES3_TOLERANCE 850
#define PHOTORES4_TOLERANCE 850
#define PITCH_LIMIT1 4
#define PITCH_LIMIT2 7
#define PITCH_LIMIT3 10
#define PITCH_LIMIT4 13
#define PITCH_LIMIT5 16

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

// modify these to change the sound of your instrument
int ATTACK = 50;
int DECAY = 128;
int SUSTAIN = 3000;
int RELEASE = 300;
int ATTACK_LEVEL = 64;
int DECAY_LEVEL = 52;
int RELEASE_LEVEL = 0;
int SUSTAIN_LEVEL = 50;
int octaveShift = 48;
int gain = 255;

/*
   GLOBALS
*/
LiquidCrystal lcd(rsPin, ePin, d4Pin, d5Pin, d6Pin, d7Pin);

int xAxis, yAxis, sw;

/*
   AUDIO
*/

// oscilator declarations, one for each string
Oscil<SIN2048_NUM_CELLS, AUDIO_RATE> aSin(SIN2048_DATA);
Oscil<SIN2048_NUM_CELLS, AUDIO_RATE> aSin2(SIN2048_DATA);
Oscil<SIN2048_NUM_CELLS, AUDIO_RATE> aSin3(SIN2048_DATA);
Oscil<SIN2048_NUM_CELLS, AUDIO_RATE> aSin4(SIN2048_DATA);

ADSR<CONTROL_RATE, AUDIO_RATE> env;

RollingAverage<int, 16> distAvg;

// Note mapping, change it how you want
int midiTable[5][4] = {69, 72, 76, 79, 71, 74, 77, 81, 72, 76,
                       79, 83, 74, 77, 81, 84, 76, 79, 83, 86};

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
// number of menu options
const int noLevels = 7;

const char menuStrings[noLevels][16] = {
    "Guitarduino", "Gain", "Attack", "Decay", "Sustain", "Release", "Octave"};

int* menuLevels[noLevels] = {NULL,     &gain,    &ATTACK,     &DECAY,
                             &SUSTAIN, &RELEASE, &octaveShift};

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
  } else if (xAxis < lowThreshold && cursorPosition && !xDown) {
    (*(menuLevels[cursorPosition]))--;
    menuChanged = true;
    xDown = true;
  } else if (xAxis > lowThreshold && xAxis < highThreshold) {
    xDown = false;
  }

  if (yAxis > highThreshold && !yDown) {
    cursorPosition++;
    if (cursorPosition >= noLevels) {
      cursorPosition = 0;
    }
    menuChanged = true;
    yDown = true;
  } else if (yAxis < lowThreshold && !yDown) {
    cursorPosition--;
    if (cursorPosition < 0) {
      cursorPosition = noLevels - 1;
    }
    menuChanged = true;
    yDown = true;
  } else if (yAxis > lowThreshold && yAxis < highThreshold) {
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

  // LCD
  lcd.begin(16, 2);

  lcd.cursor();

  printMenu();

  aSin.setFreq(440);

  // CALCULATE MIDI -> FREQ
  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < 4; j++) {
      freqTable[i][j] = mtof(midiTable[i][j] - octaveShift);
    }
  }

  env.setLevels(ATTACK_LEVEL, DECAY_LEVEL, SUSTAIN_LEVEL, RELEASE_LEVEL);
  env.setTimes(ATTACK, DECAY, SUSTAIN, RELEASE);
}

void updateControl() {
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(100);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  dist = (duration / 2) / 29.1;

  buttonValue1 = mozziAnalogRead(photoResistorPin1);
  buttonValue2 = mozziAnalogRead(photoResistorPin2);
  buttonValue3 = mozziAnalogRead(photoResistorPin3);
  buttonValue4 = mozziAnalogRead(photoResistorPin4);

  env.update();

  // check if strings are pressed

  if (buttonValue1 < PHOTORES1_TOLERANCE && noteStopped) {
    env.noteOn();
    currentGain = 255;
    noteStopped = false;
  }

  if (buttonValue2 < PHOTORES2_TOLERANCE && noteStopped) {
    env.noteOn();
    currentGain = 255;
    noteStopped = false;
  }

  if (buttonValue3 < PHOTORES3_TOLERANCE && noteStopped) {
    env.noteOn();
    currentGain = 255;
    noteStopped = false;
  }

  if (buttonValue4 < PHOTORES4_TOLERANCE && noteStopped) {
    env.noteOn();
    currentGain = 255;
    noteStopped = false;
  }

  if (buttonValue1 > PHOTORES1_TOLERANCE &&
      buttonValue2 > PHOTORES2_TOLERANCE &&
      buttonValue3 > PHOTORES3_TOLERANCE &&
      buttonValue4 > PHOTORES4_TOLERANCE) {
    env.noteOff();
    noteStopped = true;
  }

  xAxis = mozziAnalogRead(joyXPin);
  yAxis = mozziAnalogRead(joyYPin);
  sw = digitalRead(joySWPin);

  dist = distAvg.next(dist);

  if (dist <= PITCH_LIMIT1 && (env.playing() || noteStopped)) {
    // ROOT A4
    aSin.setFreq(freqTable[4][0]);
    aSin2.setFreq(freqTable[4][1]);
    aSin3.setFreq(freqTable[4][2]);
    aSin4.setFreq(freqTable[4][3]);
  } else if (dist <= PITCH_LIMIT2 && (env.playing() || noteStopped)) {
    // ROOT B4
    aSin.setFreq(freqTable[3][0]);
    aSin2.setFreq(freqTable[3][1]);
    aSin3.setFreq(freqTable[3][2]);
    aSin4.setFreq(freqTable[3][3]);
  } else if (dist <= PITCH_LIMIT3 && (env.playing() || noteStopped)) {
    // ROOT C4
    aSin.setFreq(freqTable[2][0]);
    aSin2.setFreq(freqTable[2][1]);
    aSin3.setFreq(freqTable[2][2]);
    aSin4.setFreq(freqTable[2][3]);
  } else if (dist <= PITCH_LIMIT4 && (env.playing() || noteStopped)) {
    // ROOT D4
    aSin.setFreq(freqTable[1][0]);
    aSin2.setFreq(freqTable[1][1]);
    aSin3.setFreq(freqTable[1][2]);
    aSin4.setFreq(freqTable[1][3]);
  } else if (dist <= PITCH_LIMIT5 && (env.playing() || noteStopped)) {
    // ROOT E4
    aSin.setFreq(freqTable[0][0]);
    aSin2.setFreq(freqTable[0][1]);
    aSin3.setFreq(freqTable[0][2]);
    aSin4.setFreq(freqTable[0][3]);
  }
}

int updateAudio() {
  int currentSample = 0;

  // this should be polyphonic but 8bit and arduino processing is not good
  // enough
  if (buttonValue1 < PHOTORES1_TOLERANCE) {
    currentSample += aSin.next();
  } else if (buttonValue2 < PHOTORES2_TOLERANCE) {
    currentSample += aSin2.next();
  } else if (buttonValue3 < PHOTORES3_TOLERANCE) {
    currentSample += aSin3.next();
  } else if (buttonValue4 < PHOTORES4_TOLERANCE) {
    currentSample += aSin4.next();
  }

  currentSample *= env.next();

  // shift to the right so it fits in the 8 bit audio output
  return (((int)currentSample >> 6));
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
