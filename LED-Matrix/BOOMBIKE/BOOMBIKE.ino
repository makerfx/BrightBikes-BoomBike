/*
  THe Maker Effect Brightbikes Boombike
  HIGHLY adapted from the features demo of Smartmatrix by Louis Beaudoin (Pixelmatix)
  This project makes use of the following libraries:
    Adafruit GFX Layers Library
    SmartMatrix Library
    AnimatedGif
    GifDecoder (Smartmatrix left this one out of the list)
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
#include <SD.h>
#include <GifDecoder.h>
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

const int defaultBrightness = (100*255)/100;        // full (100%) brightness
const int defaultScrollOffset = 6;
const rgb24 defaultBackgroundColor = {0x00, 0x00, 0x33};
const int ledPin = 13;

GifDecoder<kMatrixWidth, kMatrixHeight, 12> decoder;
#define SD_CS BUILTIN_SDCARD
#define GIF_DIRECTORY "/gifs/"

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





// the setup() method runs once, when the sketch starts
void setup() {
  // initialize the digital pin as an output.
  pinMode(ledPin, OUTPUT);

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

  // Determine how many animated GIF files exist
  num_files = enumerateGIFFiles(GIF_DIRECTORY, true);
  if(num_files < 0) {
    Serial.println("No gifs directory");
    while(1);
  }

  // If GIF folder is empty
  if(!num_files) {
    Serial.println("Empty gifs directory");
    while(1);
  }
}

// Set various shows / effects to play here. they will run in order if enabled.
// First variable is enable / Disable, second variable is runtime for show.
#define ENAB_RECTANGLE_BRYAN    1
#define TIME_RECTANGLE_BRYAN    10000

#define ENAB_MOVINGBARS_BRYAN   1
#define TIME_MOVINGBARS_BRYAN   20000

#define ENAB_GIF_PLAYBACK       1
#define TIME_GIF_PLAYBACK       40000

#define ENAB_EMPTY              0
#define TIME_EMPTY              0


// the loop() method runs over and over again,
// as long as the board has power
void loop() {
    int i, j;
    unsigned long currentMillis;

    // clear screen
    backgroundLayer.fillScreen(defaultBackgroundColor);
    backgroundLayer.swapBuffers();



//*********************************************************************************************************************
//*************************************** SHOWS / EFFECTS *************************************************************
//*********************************************************************************************************************
//*********************************************************************************************************************



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

#if (ENAB_MOVINGBARS_BRYAN == 1)
  {

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
        int ShowRunTime = millis() + TIME_MOVINGBARS_BRYAN;

        for (i = 0; i < barcount; i++) {
          bar_start_height[i] = 0;
          bar_draw_height[i] = random(0, matrix.getScreenHeight());
          bar_start_time[i] = millis();
          bar_draw_time[i] = millis() + random(randlo, randhi);
        }
        scrollingLayer.setCursor(0, 0);
        scrollingLayer.setTextColor(0xFFFF);
        scrollingLayer.setTextSize(4);
        scrollingLayer.start("BRIGHTBIKES BOOMBIKE", 1);
        
        while (millis() < ShowRunTime) {
          for (i = 0; i < barcount; i++) {
            //color.red = (cos(.01*PI*(colorcount+100+(i*10)+((i%2)*100)))*90)+90;
            color.red = 0;
            color.green = (cos(.01*PI*(colorcount+133+(i*10)))*90)+90;
            color.blue = (cos(.01*PI*(colorcount+133+(i*10)))*40)+215;
            int bar_travel_to = map(millis(), bar_start_time[i], bar_draw_time[i], bar_start_height[i], bar_draw_height[i]); 
            backgroundLayer.fillRectangle((matrix.getScreenWidth() / barcount) * i, bar_travel_to, (matrix.getScreenWidth() / barcount) * (i + 1), 0, color);
         
            if (millis() >= bar_draw_time[i]) {
              bar_start_height[i] = bar_travel_to;
              bar_draw_height[i] = random(0, matrix.getScreenHeight());
              bar_start_time[i] = millis();
              bar_draw_time[i] = millis() + random(randlo, randhi);
            }
          }

          backgroundLayer.swapBuffers();
          backgroundLayer.fillScreen(defaultBackgroundColor);
          colorcount += 1;

          delay(drawrate);
        }
        
  }
#endif


    // "Drawing Rectangles"
#if (ENAB_GIF_PLAYBACK == 1)
  {
    static unsigned long displayStartTime_millis;
    static int nextGIF = 1;     // we haven't loaded a GIF yet on first pass through, make sure we do that

    unsigned long now = millis();

    static int index = 0;

    int ShowRunTime = millis() + TIME_GIF_PLAYBACK;

    while (millis() < ShowRunTime) {
      Serial.println("Loop");


      // default behavior is to play the gif for DISPLAY_TIME_SECONDS or for NUMBER_FULL_CYCLES, whichever comes first
      if(decoder.getCycleNumber() > 0) { 
          nextGIF = 1;
      }


      if(nextGIF) {
        nextGIF = 0;

        if (openGifFilenameByIndex(GIF_DIRECTORY, index) >= 0) {
            // Can clear screen for new animation here, but this might cause flicker with short animations
            // matrix.fillScreen(COLOR_BLACK);
            // matrix.swapBuffers();

            // start decoding, skipping to the next GIF if there's an error
            if(decoder.startDecoding() < 0) {
                nextGIF = 1;
                return;
            }

            // Calculate time in the future to terminate animation
            displayStartTime_millis = now;
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
  }
#endif
    // "Drawing Round Rectangles"
#if (ENAB_EMPTY == 1)
    {
      //Just an empty statement for future expansion
    }
#endif
}
