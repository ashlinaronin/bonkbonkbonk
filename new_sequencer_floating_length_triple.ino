// repeats what you do
// ATtiny85 at 8 MHz
//https://bitbucket.org/mrgrok/mrgroks-arduino-sketches/src/b2d52c93640c9f582ac0cd0e867c2a67107274f4/recordAndPlaybackAtRate/recordAndPlaybackAtRate.ino?fileviewer=file-view-default
#include <Wire.h>
#include <Adafruit_MotorShield.h>
#include "utility/Adafruit_MS_PWMServoDriver.h"

// pins
const byte modeBtn = 5;
const byte morseBtn1 = 4;
const byte morseBtn2 = 6;

Adafruit_MotorShield AFMS = Adafruit_MotorShield();
Adafruit_DCMotor *solenoid1 = AFMS.getMotor(3);
Adafruit_DCMotor *solenoid2 = AFMS.getMotor(4);
Adafruit_DCMotor *solenoid3 = AFMS.getMotor(2);
Adafruit_DCMotor *pump = AFMS.getMotor(1);

const int sampleLength = 20;  // ms
const int maxLength = 300;
int seqLength = 300;

boolean inRecordMode = false;
boolean wasInRecordMode = false;

// seq recording buffers
int seq1[maxLength];
int seq2[maxLength];

short idxPlayback = 0;
short idxRecord = 0;

int getPrevIndex(int currentIndex) {
  return (currentIndex - 1 + seqLength) % seqLength;
}

int getNextIndex(int currentIndex) {
  if (seq1[currentIndex + 1] == 2) {
    return 0;
  }
  return (currentIndex + 1) % seqLength;
}


void printArray(int arr[], int length, String name) {
  Serial.print(name);
  Serial.print("--");
  for (int i = 0; i < length; i++) {
    Serial.print(arr[i]);
    Serial.print(", ");
  }
}


void setup() {
  pinMode(modeBtn, INPUT);
  pinMode(morseBtn1, INPUT);
  pinMode(morseBtn2, INPUT);
  AFMS.begin();
  solenoid1->setSpeed(255);
  solenoid2->setSpeed(255);
  solenoid3->setSpeed(255);
  pump->setSpeed(255);
  Serial.begin(9600);
}

void resetForRecording() {
  memset(seq1, 0, sizeof(seq1));
  memset(seq2, 0, sizeof(seq2));
  idxRecord = 0;  // reset record idx just to make playback start point obvious
  idxPlayback = 0;
}

void loop() {
  inRecordMode = digitalRead(modeBtn);
  if (inRecordMode == true) {
    if (!wasInRecordMode) {
      resetForRecording();
    }
    recordLoop();
  } else {
    // continue playing loop
    if (wasInRecordMode) {
      seqLength = idxRecord;
      idxPlayback = 0;
      wasInRecordMode = false;  // record prev state for next iteration so we know whether to reset the record arr index
    }
    playbackLoop();
  }

  wasInRecordMode = inRecordMode;  // record prev state for next iteration so we know whether to reset the record arr index
}

void recordLoop() {
  boolean state1 = digitalRead(morseBtn1);
  boolean state2 = digitalRead(morseBtn2);
  solenoid1->run(state1 ? FORWARD : RELEASE);  // give feedback to person recording the loop
  solenoid2->run(state2 ? FORWARD : RELEASE);
  solenoid3->run(state2 ? FORWARD : RELEASE);
  pump->run(state1 ? FORWARD : RELEASE);

  seq1[idxRecord] = state1;
  seq2[idxRecord] = state2;

  idxRecord = (idxRecord + 1) % maxLength;

  delay(sampleLength);  // slow the loop to a time constant so we can reproduce the timelyness of the recording
}


void playbackLoop() {
  int prevIndex = getPrevIndex(idxPlayback);

  // only call motor driver if values have changed
  if (seq1[idxPlayback] != seq1[prevIndex]) {
    solenoid1->run(seq1[idxPlayback] ? FORWARD : RELEASE);
    pump->run(seq1[idxPlayback] ? FORWARD : RELEASE);
  }

  // only call motor driver if values have changed
  if (seq2[idxPlayback] != seq2[prevIndex]) {
    solenoid2->run(seq2[idxPlayback] ? FORWARD : RELEASE);
    solenoid3->run(seq2[idxPlayback] ? FORWARD : RELEASE);
  }

  idxPlayback = getNextIndex(idxPlayback);

  delay(sampleLength);  // keep time same as we had for record loop (approximately)
}
