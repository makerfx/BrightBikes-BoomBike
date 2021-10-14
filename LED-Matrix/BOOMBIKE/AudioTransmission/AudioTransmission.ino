/* Stereo peak meter example, assumes Audio adapter but just uses terminal so no more parts required.

This example code is in the public domain
*/

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

const int myInput = AUDIO_INPUT_LINEIN;
// const int myInput = AUDIO_INPUT_MIC;

AudioInputI2S        audioInput;         // audio shield: mic or line-in
AudioAnalyzePeak     peak_L;
AudioAnalyzePeak     peak_R;
AudioOutputI2S       audioOutput;        // audio shield: headphones & line-out

AudioConnection c1(audioInput,0,peak_L,0);
AudioConnection c2(audioInput,1,peak_R,0);
AudioConnection c3(audioInput,0,audioOutput,0);
AudioConnection c4(audioInput,1,audioOutput,1);

AudioControlSGTL5000 audioShield;

const uint8_t pinBit2 = 1;
const uint8_t pinBit4 = 2;
const uint8_t pinBit8 = 3;
const uint8_t pinBit16 = 4;
const uint8_t pinBit32 = 16;
const uint8_t pinBit64 = 17;

void setup() {
  AudioMemory(6);
  audioShield.enable();
  audioShield.inputSelect(myInput);
  audioShield.volume(0.5);
  Serial.begin(115200);

  pinMode(pinBit2, OUTPUT);
  pinMode(pinBit4, OUTPUT);
  pinMode(pinBit8, OUTPUT);
  pinMode(pinBit16, OUTPUT);
  pinMode(pinBit32, OUTPUT);
  pinMode(pinBit64, OUTPUT);  
}

// for best effect make your terminal/monitor a minimum of 62 chars wide and as high as you can.

elapsedMillis fps;
uint8_t cnt=0;

void loop() {
  if(fps > 20) {
    if (peak_L.available() && peak_R.available()) {
      fps=0;
      uint8_t leftPeak=peak_L.read() * 63.0;
      uint8_t rightPeak=peak_R.read() * 255.0;
      uint8_t bit2 = LOW;
      uint8_t bit4 = LOW;
      uint8_t bit8 = LOW;
      uint8_t bit16 = LOW;
      uint8_t bit32 = LOW;
      uint8_t bit64 = LOW;
      leftPeak = leftPeak + 64;
      char str[7];
      itoa(leftPeak, str, 2);
      if ((int)str[1] == 49){
        bit64 = HIGH;
      }
      if ((int)str[2] == 49){
        bit32 = HIGH;
      }
      if ((int)str[3] == 49){
        bit16 = HIGH;
      }
      if ((int)str[4] == 49){
        bit8 = HIGH;
      }
      if ((int)str[5] == 49){
        bit4 = HIGH;
      }
      if ((int)str[6] == 49){
        bit2 = HIGH;
      }
      digitalWriteFast(pinBit2, bit2);
      digitalWriteFast(pinBit4, bit4);
      digitalWriteFast(pinBit8, bit8);
      digitalWriteFast(pinBit16, bit16);
      digitalWriteFast(pinBit32, bit32);
      digitalWriteFast(pinBit64, bit64);
      Serial.println(leftPeak-64);

      analogWrite(13, rightPeak);
    }
  }
}


/*
void loop() {
  if(fps > 10) {
    if (peak_L.available() && peak_R.available()) {
      fps=0;
      uint8_t leftPeak=peak_L.read() * 254.0;
      uint8_t rightPeak=peak_R.read() * 254.0;

      Serial4.println(leftPeak, HEX);
      
      analogWrite(13, rightPeak);
    }
  }
}
*/
