// The MIT License (MIT)

// Copyright (c) 2016 Nathan Heskew

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "sfx.h"

SYSTEM_THREAD(ENABLED);

const int AMP_ENABLE_PIN = D0;
const int MOTION_PIN = D1;
const int MIC_PIN = A0;
const int SERVO_PIN = A5;
const int SPEAKER_PIN = DAC1;
const int LED_RED_PIN = WKP;
const int LED_GREEN_PIN = RX;
const int LED_BLUE_PIN = TX;
const int TRIGGER_PIN = D7;

volatile int8_t motionSensed = 0;
int motionActive = -1;
int soundActive = -1;
int triggerActive = 0;
int forcedActive = 0;

uint32_t triggerActiveDuration = 300000;
uint16_t ledAlarmDuration = 5000;

Timer motionTimeout(triggerActiveDuration, actOnNoMotion, true);
Timer soundTimeout(triggerActiveDuration, actOnNoSound, true);
Timer ledAlarmTimeout(ledAlarmDuration, ledNormal, true);

Servo headServo;
uint8_t pos = 90;

SFX* sfx = new SFX(AMP_ENABLE_PIN, SPEAKER_PIN);

void setup() {

  pub("thing/setup", "starting droid-sentry...");

  pub("thing/setup/firmware/version", System.version().c_str());

  sfx->init();

  // Pin configuration
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_BLUE_PIN, OUTPUT);
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(MOTION_PIN, INPUT);
  pinMode(MIC_PIN, INPUT);

  // Initialize the motion interrupt
  attachInterrupt(MOTION_PIN, actOnMotionISR, RISING);

  // Initialize the trigger
  digitalWrite(TRIGGER_PIN, LOW);

  // Initialize the servo
  headServo.attach(SERVO_PIN);
  headServo.write(pos);

  // Set up cloud functions
  Particle.function("turnon", turnOn);
  Particle.function("turnoff", turnOff);
  Particle.function("beep", makeBeeps);
  Particle.function("look", makeMovement);
  // ...and variables
  Particle.variable("motion", motionActive);
  Particle.variable("sound", soundActive);
  Particle.variable("trigger", triggerActive);
  Particle.variable("forced", forcedActive);

  initSequence();

  pub("thing/setup", "droid-sentry started!");
}

void loop() {

  // If not triggered and there's motion, trigger!
  if (!triggerIsActive() && motionIsActive()) {
    triggerOn();
  }

  // If active and not forced and there's no motion or sound, turn off the trigger
  if (!isForcedActive() && triggerIsActive() && !motionIsActive() && !soundIsActive()) {
    triggerOff();
  }

  motionCheck();
  soundCheck();
}

void initSequence() {

  ledInit();
  sfx->playBeeps();
  moveHead();
}

bool motionIsActive() {

  return motionActive == 1;
}

bool soundIsActive() {

  return soundActive == 1;
}

bool triggerIsActive() {

  return triggerActive == 1;
}

bool isForcedActive() {

  return forcedActive == 1;
}

void setMotionActive() {

  motionActive = 1;
}

void setMotionInactive() {

  motionActive = 0;
}

void setSoundActive() {

  soundActive = 1;
}

void setSoundInactive() {

  soundActive = 0;
}

void setTriggerActive() {

  triggerActive = 1;
}

void setTriggerInactive() {

  triggerActive = 0;
}

void setForcedActive() {

  forcedActive = 1;
}

void setForcedInactive() {

  forcedActive = 0;
}

int8_t turnOn(String arg) {

  pub("thing/remote/turnon");

  setForcedActive();
  triggerOn();

  return 1;
}

int8_t turnOff(String arg) {

  pub("thing/remote/turnoff");
  setForcedInactive();

  motionTimeout.stop();
  soundTimeout.stop();

  actOnNoMotion();
  actOnNoSound();

  triggerOff();

  return 1;
}

int8_t makeBeeps(String arg) {

  if (sfx->playBeeps() == true) {
    return 1;
  }

  return 0;
}

int8_t makeMovement(String arg) {

  moveHead();
  return 1;
}

void ledInit() {

  analogWrite(LED_RED_PIN, 120);
  analogWrite(LED_GREEN_PIN, 150);
  analogWrite(LED_BLUE_PIN, 100);

  delay(1000);

  ledAlarmTimeout.reset();
}

void ledNormal() {

  analogWrite(LED_RED_PIN, 255);
  analogWrite(LED_GREEN_PIN, 150);
  analogWrite(LED_BLUE_PIN, 255);
}

void ledAlarm() {

  analogWrite(LED_RED_PIN, 120);
  analogWrite(LED_GREEN_PIN, 255);
  analogWrite(LED_BLUE_PIN, 255);
  ledAlarmTimeout.reset();
}

void triggerOn() {

  setTriggerActive();
  digitalWrite(TRIGGER_PIN, HIGH);

  sfx->playAlert();
  moveHead();

  pub("thing/trigger/on");
}

void triggerOff() {

  setTriggerInactive();
  digitalWrite(TRIGGER_PIN, LOW);

  ledNormal();

  pub("thing/trigger/off");
}

void moveHead() {

  for (pos = 90; pos > 70; pos -= 2) {
    headServo.write(pos);
    delay(15);
  }

  for (; pos < 110; pos += 2) {
    headServo.write(pos);
    delay(15);
  }

  delay(200);
  
  for (; pos > 90; pos -= 2) {
    headServo.write(pos);
    delay(15);
  }
}

void actOnMotionISR() {

  motionSensed = 1;
}

void actOnMotion() {

  ledAlarm();

  if (motionIsActive() == true) {
    motionTimeout.reset();
    pub("thing/motion/more");
    return;
  }

  setMotionActive();
  motionTimeout.reset();

  pub("thing/motion/first");
}

void actOnNoMotion() {

  pub("thing/debug/actOnNoMotion");

  if (motionIsActive() == false) {
    return;
  }

  pub("thing/debug/callSetMotionInactive");

  setMotionInactive();
  pub("thing/motion/none");
}

void actOnSound() {

  // sound only keeps the trigger active
  if (motionIsActive() == false) {
    pub("thing/sound/ignored");
    return;
  }

  ledAlarm();

  if (soundIsActive() == true) {
    soundTimeout.reset();
    pub("thing/sound/more");
    return;
  }

  setSoundActive();
  soundTimeout.reset();

  pub("thing/sound/first");
}

void actOnNoSound() {

  if (soundIsActive() == false) {
    return;
  }

  setSoundInactive();
  pub("thing/sound/none");
}

void pub(const char* eventName, const char* data) {

  Particle.publish(eventName, data, 604800, PRIVATE);
}

void pub(const char* eventName) {

  pub(eventName, "1");
}

void motionCheck() {

  if (motionSensed == 1) {

    motionSensed = 0;
    actOnMotion();
  }
}

void soundCheck() {

  const int8_t sampleWindow = 50; // Sample window width in mS (50 mS = 20Hz)
  uint8_t sounds = 0;

  uint32_t startMillis = millis();
  uint16_t peakToPeak = 0;

  uint16_t signalMax = 0;
  uint16_t signalMin = 4095;

  uint16_t sample;

  // collect data for the sample window
  while (millis() - startMillis < sampleWindow) {

    sample = analogRead(MIC_PIN);
    if (sample < 4095) {

      if (sample > signalMax) {
        signalMax = sample;
      }
      else if (sample < signalMin) {
        signalMin = sample;
      }
    }
  }

  peakToPeak = signalMax - signalMin;  // max - min = peak-peak amplitude
  uint16_t mV = peakToPeak * 3300 / 4095;  // convert to mV

  if (signalMin > 1400 && signalMax < 3968 && mV > 500) {

    if (++sounds > 1) {

      char outString[20];
      sprintf(outString, "%u - %u: %u", (unsigned)signalMin, (unsigned)signalMax, (unsigned)mV);
      pub("thing/debug/sound", outString);

      actOnSound();
    }
  }
  else {
    sounds = 0;
  }
}
