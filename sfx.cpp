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

#include "application.h"
#include "sfx.h"

SFX::SFX(uint8_t ampEnablePin, uint8_t speakerPin) {
  
  _ampEnablePin = ampEnablePin;
  _speakerPin = speakerPin;
}

bool SFX::init() {
  
  // Pin configuration
  pinMode(_ampEnablePin, OUTPUT);
  pinMode(_speakerPin, OUTPUT);

  // Initialize the amp
  digitalWrite(_ampEnablePin, LOW);

  return true;
}

bool SFX::playBeeps() {

  return this->playSound(BEEPS_TIMING, BEEPS_LEN, BEEPS);
}

bool SFX::playAlert() {

  return this->playSound(ALERT_TIMING, ALERT_LEN, ALERT);
}

bool SFX::playSound(uint8_t sound_timing, uint16_t sound_len, const unsigned char sound[]) {

  uint32_t lastWrite = micros();
  uint32_t now, diff;
  uint16_t sound_i = 0;
  uint32_t value;

  digitalWrite(_ampEnablePin, HIGH);
  delayMicroseconds(50);

  while (sound_i < sound_len) {

      // Convert 8-bit to 12-bit
      value = map(sound[sound_i++], 0, 255, 0, 4095);

      now = micros();
      diff = (now - lastWrite);
      if (diff < sound_timing) {
          delayMicroseconds(sound_timing - diff);
      }

      analogWrite(_speakerPin, value);
      lastWrite = micros();
  }

  delayMicroseconds(50);
  digitalWrite(_ampEnablePin, LOW);

  return true;
}