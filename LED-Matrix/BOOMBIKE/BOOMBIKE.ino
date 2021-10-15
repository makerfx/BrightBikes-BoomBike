/*
  THe Maker Effect Brightbikes Boombike
  HIGHLY adapted from the features demo of Smartmatrix by Louis Beaudoin (Pixelmatix)
  This project makes use of the following libraries:
    Adafruit GFX Layers Library
    SmartMatrix Library
    AnimatedGif
    GifDecoder (Smartmatrix left this one out of the list)
    MultiButton
  The SmartMatrix library is also modified to include a panel mapping for the 4x 64x64 display
  setup for the BoomBike.

    It is using a Teensy 4.1 and the LEDShield V5.
*/

#define USE_ADAFRUIT_GFX_LAYERS

// Design is using Teensy 4.1 and V5 Shield, see SmartMatrix Library for other hardware configurations
#include <MatrixHardware_Teensy4_ShieldV5.h>        // SmartLED Shield for Teensy 4 (V5)

// Smartmatrix Includes
#include <SmartMatrix.h>
#include "ColorConverterLib.h"

// GIF Playback Includes
#include <SD.h> // also used by audio
#include <GifDecoder.h>
#include <PinButton.h>
#include "FilenameFunctions.h"
#define DISPLAY_TIME_SECONDS 10
#define NUMBER_FULL_CYCLES   100

//    *************SmartMatrix Parameters******************
#define COLOR_DEPTH 24                  // Choose the color depth used for storing pixels in the layers: 24 or 48 (24 is good for most sketches - If the sketch uses type `rgb24` directly, COLOR_DEPTH must be 24)
const uint16_t kMatrixWidth = 256;       // Set to the width of your display, must be a multiple of 8
const uint16_t kMatrixHeight = 64;      // Set to the height of your display
const uint8_t kRefreshDepth = 24;       // Tradeoff of color quality vs refresh rate, max brightness, and RAM usage.  36 is typically good, drop down to 24 if you need to.  On Teensy, multiples of 3, up to 48: 3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45, 48.  On ESP32: 24, 36, 48
const uint8_t kDmaBufferRows = 48;       // known working: 2-4, use 2 to save RAM, more to keep from dropping frames and automatically lowering refresh rate.  (This isn't used on ESP32, leave as default)
const uint8_t kPanelType = SM_PANELTYPE_HUB75_64ROW_256COL_BRYANSCAN;   // Choose the configuration that matches your panels.  See more details in MatrixCommonHub75.h and the docs: https://github.com/pixelmatix/SmartMatrix/wiki
const uint32_t kMatrixOptions = (SM_HUB75_OPTIONS_NONE);        // see docs for options: https://github.com/pixelmatix/SmartMatrix/wiki
const uint8_t kBackgroundLayerOptions = (SM_BACKGROUND_OPTIONS_NONE);
const uint8_t kScrollingLayerOptions = (SM_SCROLLING_OPTIONS_NONE);
const uint8_t kIndexedLayerOptions = (SM_INDEXED_OPTIONS_NONE);

//    *************SmartMatrix Allocation******************
SMARTMATRIX_ALLOCATE_BUFFERS(matrix, kMatrixWidth, kMatrixHeight, kRefreshDepth, kDmaBufferRows, kPanelType, kMatrixOptions);
SMARTMATRIX_ALLOCATE_BACKGROUND_LAYER(backgroundLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kBackgroundLayerOptions);
#ifdef USE_ADAFRUIT_GFX_LAYERS
  // there's not enough allocated memory to hold the long strings used by this sketch by default, this increases the memory, but it may not be large enough
  SMARTMATRIX_ALLOCATE_GFX_MONO_LAYER(scrollingLayer, kMatrixWidth, kMatrixHeight, 24*1024, 1, COLOR_DEPTH, kScrollingLayerOptions);
#else
  SMARTMATRIX_ALLOCATE_SCROLLING_LAYER(scrollingLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kScrollingLayerOptions);
#endif
SMARTMATRIX_ALLOCATE_INDEXED_LAYER(indexedLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kIndexedLayerOptions);

#include "colorwheel.c"
#include "gimpbitmap.h"

const int defaultBrightness = (50*255)/100;        // full (100%) brightness
const int defaultScrollOffset = 6;
const rgb24 defaultBackgroundColor = {0x00, 0x00, 0x33};

const int ledPin = 13;
PinButton frtButton0(14);
PinButton frtButton1(15);
PinButton frtButton2(16);
PinButton frtButton3(17);
PinButton frtButton4(18);
PinButton frtButton5(19);
PinButton frtButton6(20);

GifDecoder<kMatrixWidth, kMatrixHeight, 12> decoder;
#define SD_CS BUILTIN_SDCARD

const char gifDirs[8][7] = { "/gif0/", "/gif1/", "/gif2/", "/gif3/", "/gif4/", "/gif5/", "/gif6/" };

int num_files;

void screenClearCallback(void) {
  backgroundLayer.fillScreen({0,0,0});
}
void updateScreenCallback(void) {
  backgroundLayer.swapBuffers();
}
void drawPixelCallback(int16_t x, int16_t y, uint8_t red, uint8_t green, uint8_t blue) {
    backgroundLayer.drawPixel(x, y, {red, green, blue});
}

//    *************BoomBike Variables******************     

static int frtButtonStatus = 0;
uint8_t audVol = 0;
const uint8_t audPin2 = 0;
const uint8_t audPin4 = 1;
const uint8_t audPin8 = 8;
const uint8_t audPin16 = 21;
const uint8_t audPin32 = 22;
const uint8_t audPin64 = 23;


// the setup() method runs once, when the sketch starts
void setup() {
  // initialize the digital pin as an output.
  pinMode(ledPin, OUTPUT);
  
  pinMode(audPin2, INPUT);
  pinMode(audPin4, INPUT);
  pinMode(audPin8, INPUT);
  pinMode(audPin16, INPUT);
  pinMode(audPin32, INPUT);
  pinMode(audPin64, INPUT);
  
  decoder.setScreenClearCallback(screenClearCallback);
  decoder.setUpdateScreenCallback(updateScreenCallback);
  decoder.setDrawPixelCallback(drawPixelCallback);
  decoder.setFileSeekCallback(fileSeekCallback);
  decoder.setFilePositionCallback(filePositionCallback);
  decoder.setFileReadCallback(fileReadCallback);
  decoder.setFileReadBlockCallback(fileReadBlockCallback);
  // NOTE: new callback function required after we moved to using the external AnimatedGIF library to decode GIFs
  decoder.setFileSizeCallback(fileSizeCallback);

  Serial.begin(115200);

  matrix.addLayer(&backgroundLayer); 
  matrix.addLayer(&scrollingLayer); 
  matrix.addLayer(&indexedLayer); 
  matrix.begin();

  matrix.setBrightness(defaultBrightness);
  scrollingLayer.setOffsetFromTop(defaultScrollOffset);
  backgroundLayer.enableColorCorrection(true);

  // Confirm SD Card
  if(initFileSystem(SD_CS) < 0) {
    Serial.println("No SD card");
    while(1);
  }
}

// Function to check status of all buttons, and return most recent button pressed.
int CheckButtonsChange() {
  frtButton0.update();
  if (frtButton0.isClick()) {
    frtButtonStatus = 0;
  }
  frtButton1.update();
  if (frtButton1.isClick()) {
    frtButtonStatus = 1;
  }
  frtButton2.update();
  if (frtButton2.isClick()) {
    frtButtonStatus = 2;
  }
  frtButton3.update();
  if (frtButton3.isClick()) {
    frtButtonStatus = 3;
  }
  frtButton4.update();
  if (frtButton4.isClick()) {
    frtButtonStatus = 4;
  }
  frtButton5.update();
  if (frtButton5.isClick()) {
    frtButtonStatus = 5;
  }
  frtButton6.update();
  if (frtButton6.isClick()) {
    frtButtonStatus = 6;
  }
  return frtButtonStatus;
}

// runs a particular show, usually called from CheckButtonsChange()
void RunShow(int showNum) {
  switch(showNum) {
    case 0:
      MainShow();
      break;
    case 1:  // remaining cases all use Gif playback
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
      GifShow(showNum);
      break;
    default:
      frtButtonStatus = 0;
      MainShow();
  }
}

void MainShow() {

        int drawrate = 20;
        int barcount = 128;
        int randhi = 700;
        int randlo = 300;
        int textReplay = 15000;


        rgb24 color;
        int colorcount = 0;
        int bar_start_height[barcount];
        int bar_draw_height[barcount];
        int bar_start_time[barcount];
        int bar_draw_time[barcount];
        int showLength = 30000;
        int showRestart = millis() - 1;

        for (int i = 0; i < barcount; i++) {
          bar_start_height[i] = 0;
          bar_draw_height[i] = random(0, matrix.getScreenHeight());
          bar_start_time[i] = millis();
          bar_draw_time[i] = millis() + random(randlo, randhi);
        }

        while (CheckButtonsChange() == 0) {
          if (showRestart < millis()){
            showRestart = millis() + showLength;
            
            scrollingLayer.setCursor(0, 0);
            scrollingLayer.setMode(wrapForward);
            scrollingLayer.setTextColor(0xFFFF);
            scrollingLayer.setTextSize(4);
            scrollingLayer.start("BRIGHTBIKES BOOMBIKE", 1);
          }
          
          for (int i = 0; i < barcount; i++) {
            //color.red = (cos(.01*PI*(colorcount+100+(i*10)+((i%2)*100)))*90)+90;
            color.red = 0;
            color.green = (cos(.01*PI*(colorcount+133+(i*10)))*90)+90;
            color.blue = (cos(.01*PI*(colorcount+133+(i*10)))*40)+215;
            int bar_travel_to = map(millis(), bar_start_time[i], bar_draw_time[i], bar_start_height[i], bar_draw_height[i]); 
            backgroundLayer.fillRectangle((matrix.getScreenWidth() / barcount) * i, bar_travel_to, (matrix.getScreenWidth() / barcount) * (i + 1), 64, color);
         
            if (millis() >= bar_draw_time[i]) {
              bar_start_height[i] = bar_travel_to;
              bar_draw_height[i] = random(0, matrix.getScreenHeight());
              bar_start_time[i] = millis();
              bar_draw_time[i] = millis() + random(randlo, randhi);
            }
          }

          backgroundLayer.swapBuffers();
          
          uint8_t audBit2 = 0;
          uint8_t audBit4 = 0;
          uint8_t audBit8 = 0;
          uint8_t audBit16 = 0;
          uint8_t audBit32 = 0;
          uint8_t audBit64 = 0;
          audVol = 0;

          audBit2 = digitalReadFast(audPin2);
          audBit4 = digitalReadFast(audPin4);
          audBit8 = digitalReadFast(audPin8);
          audBit16 = digitalReadFast(audPin16);
          audBit32 = digitalReadFast(audPin32);
          audBit64 = digitalReadFast(audPin64);

          Serial.print(audBit64);
          Serial.print(",");
          Serial.print(audBit32);
          Serial.print(",");
          Serial.print(audBit16);
          Serial.print(",");
          Serial.print(audBit8);
          Serial.print(",");
          Serial.print(audBit4);
          Serial.print(",");
          Serial.println(audBit2);

          audVol += audBit2;
          audVol += 2*audBit4;
          audVol += 4*audBit8;
          audVol += 8*audBit16;
          audVol += 16*audBit32;
          audVol += 32*audBit64;           
          
          rgb24 bgcolor;
          bgcolor.red = audVol*4;
          bgcolor.green = 0;
          bgcolor.blue = 0;
          Serial.print("Audio:");
          Serial.println(audVol);
          backgroundLayer.fillScreen(bgcolor);
          colorcount += 1;

          delay(drawrate);
        }
        scrollingLayer.stop();

}

void GifShow(int showNum) {

    static int index = 0;
    int nextGIF = 1;

    int ShowRunTime = millis() + 120000; // Length of the show
    num_files = enumerateGIFFiles(gifDirs[showNum], true);
    

    while (millis() < ShowRunTime && CheckButtonsChange() == showNum ) {
      // programming set up to allow multiple Gifs in a folder to be played back during a "show"
      if(decoder.getCycleNumber() > 0) { 
          nextGIF = 1;
      }

      if(nextGIF) {
        nextGIF = 0;
        // gifDirs added for multiple option / button folders response
        if (openGifFilenameByIndex(gifDirs[showNum], index) >= 0) {


            // start decoding, skipping to the next GIF if there's an error
            if(decoder.startDecoding() < 0) {
                nextGIF = 1;
                return;
            }

        }

        // get the index for the next pass through
        if (++index >= num_files) {
            index = 0;
        }

      }

      if(decoder.decodeFrame() < 0) {
        // There's an error with this GIF, go to the next one
        nextGIF = 1;
      }
    }
    if (millis() >= ShowRunTime) {
      frtButtonStatus = 0;
    }
}

// the loop() method runs over and over again,
// as long as the board has power

void loop() {

    // clear screen
    backgroundLayer.fillScreen(defaultBackgroundColor);
    backgroundLayer.swapBuffers();

    // Run the show!
    RunShow(CheckButtonsChange());    

}

//*********************************************************************************************************************
//*************************************** OLD STUFF *******************************************************************
//*********************************************************************************************************************
//*********************************************************************************************************************


/*
#if (ENAB_RECTANGLE_BRYAN == 1)
  {

        currentMillis = millis();
        int ShowRunTime = currentMillis + TIME_RECTANGLE_BRYAN; 
        rgb24 color;
        color.red = 35;
        color.green = 115;
        color.blue = 185;
        int countup = 0;
        while (millis() < ShowRunTime) {
          for (i=0;i<=129;i++){
            backgroundLayer.fillScreen(defaultBackgroundColor);
            backgroundLayer.fillRectangle(countup, 32, countup + 1, 33, color);
            backgroundLayer.swapBuffers();
            countup++;
            delay(50);
          }
          countup = 0;
          backgroundLayer.fillScreen(defaultBackgroundColor);
         
        }

  }
#endif
*/
