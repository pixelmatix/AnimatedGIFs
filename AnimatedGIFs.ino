/*
 * Animated GIFs Display Code for SmartMatrix and 32x32 RGB LED Panels
 *
 * Uses SmartMatrix Library for Teensy 3.1 written by Louis Beaudoin at pixelmatix.com
 *
 * Written by: Craig A. Lindley
 *
 * Copyright (c) 2014 Craig A. Lindley
 * Refactoring by Louis Beaudoin (Pixelmatix)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

 /*
  * This example displays 32x32 GIF animations loaded from a SD Card connected to the Teensy 3.1
  * The GIFs can be up to 32 pixels in width and height.
  * This code has been tested with 32x32 pixel and 16x16 pixel GIFs, but is optimized for 32x32 pixel GIFs.
  *
  * Wiring is on the default Teensy 3.1 SPI pins, and chip select can be on any GPIO,
  * set by defining SD_CS in the code below
  * Function     | Pin
  * DOUT         |  11
  * DIN          |  12
  * CLK          |  13
  * CS (default) |  15
  *
  * This code first looks for .gif files in the /gifs/ directory
  * (customize below with the GIF_DIRECTORY definition) then plays random GIFs in the directory,
  * looping each GIF for DISPLAY_TIME_SECONDS
  *
  * This example is meant to give you an idea of how to add GIF playback to your own sketch.
  * For a project that adds GIF playback with other features, take a look at
  * Light Appliance and Aurora:
  * https://github.com/CraigLindley/LightAppliance
  * https://github.com/pixelmatix/aurora
  *
  * If you find any GIFs that won't play properly, please attach them to a new
  * Issue post in the GitHub repo here:
  * https://github.com/pixelmatix/AnimatedGIFs/issues
  */

#include <SmartMatrix3.h>
#include <SPI.h>
#include <SD.h>
#include "GIFDecoder.h"

#define DISPLAY_TIME_SECONDS 10

#define ENABLE_SCROLLING  1

// range 0-255
const int defaultBrightness = 255;

const rgb24 COLOR_BLACK = {
    0, 0, 0 };

/* SmartMatrix configuration and memory allocation */
#define COLOR_DEPTH 24                  // known working: 24, 48 - If the sketch uses type `rgb24` directly, COLOR_DEPTH must be 24
const uint8_t kMatrixWidth = 32;        // known working: 32, 64, 96, 128
const uint8_t kMatrixHeight = 32;       // known working: 16, 32, 48, 64
const uint8_t kRefreshDepth = 36;       // known working: 24, 36, 48
const uint8_t kDmaBufferRows = 2;       // known working: 2-4
const uint8_t kPanelType = SMARTMATRIX_HUB75_32ROW_MOD16SCAN; // use SMARTMATRIX_HUB75_16ROW_MOD8SCAN for common 16x32 panels
const uint8_t kMatrixOptions = (SMARTMATRIX_OPTIONS_NONE);    // see http://docs.pixelmatix.com/SmartMatrix for options
const uint8_t kBackgroundLayerOptions = (SM_BACKGROUND_OPTIONS_NONE);
const uint8_t kScrollingLayerOptions = (SM_SCROLLING_OPTIONS_NONE);

SMARTMATRIX_ALLOCATE_BUFFERS(matrix, kMatrixWidth, kMatrixHeight, kRefreshDepth, kDmaBufferRows, kPanelType, kMatrixOptions);
SMARTMATRIX_ALLOCATE_BACKGROUND_LAYER(backgroundLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kBackgroundLayerOptions);
#if ENABLE_SCROLLING == 1
SMARTMATRIX_ALLOCATE_SCROLLING_LAYER(scrollingLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kScrollingLayerOptions);
#endif

// Chip select for SD card on the SmartMatrix Shield
#define SD_CS 15

#define GIF_DIRECTORY "/gifs/"

int num_files;

/* 
  advanced usage - use large buffers in backgroundLayer for LZW decoding, e.g. 
  with 64x64 or 32*128@24-bit backgroundLayer, drawingBuffer is 12kB, enough to hold the prefix and suffix buffer
  with 32*96@24-bit backgroundLayer, drawingBuffer is 9kB, enough to hold the prefix buffer
  with 32*64@24-bit backgroundLayer, drawingBuffer is 6kB, enough to hold the suffix buffer
  with smaller than 32x32@24-bit or 16x96@24-bit is too small to hold any buffers
  Setting USE_EXTERNAL_BUFFERS will enable the LZW decoding to trash these buffers for each frame, startDrawingCallback()
    takes care of copying data from the refreshBuffer to drawingBuffer before drawing the GIF on top
*/
#if USE_EXTERNAL_BUFFERS == 1
byte stackBuffer[4*1024];
// stack is 4kB at LZW_MAXBITS=12
void * getLZWStackBufferCallback(void) {
  return (void*)stackBuffer;
}

//byte suffixBuffer[4*1024];

// suffix is 4kB at LZW_MAXBITS=12
void * getLZWSuffixBufferCallback(void) {
  return (void*)((byte *)backgroundLayer.backBuffer() + 0*1024);
  //return (void*)suffixBuffer;
}

uint16_t prefixBuffer[4*1024];

// prefix is 8kB (4kB * sizeof(uint16_t)) at LZW_MAXBITS=12
void * getLZWPrefixBufferCallback(void) {
  return (void*)((byte *)backgroundLayer.backBuffer() + 4*1024);
  //return (void*)prefixBuffer;
}

void startDrawingCallback(void) {
  // copy refresh to draw
  backgroundLayer.copyRefreshToDrawing();
}
#endif

void screenClearCallback(void) {
  backgroundLayer.fillScreen({0,0,0});
}

void updateScreenCallback(void) {
  backgroundLayer.swapBuffers();
}

void drawPixelCallback(int16_t x, int16_t y, uint8_t red, uint8_t green, uint8_t blue) {
  backgroundLayer.drawPixel(x, y, {red, green, blue});
}

// Setup method runs once, when the sketch starts
void setup() {
    setScreenClearCallback(screenClearCallback);
    setUpdateScreenCallback(updateScreenCallback);
    setDrawPixelCallback(drawPixelCallback);

#if USE_EXTERNAL_BUFFERS == 1
    setStartDrawingCallback(startDrawingCallback);
    setGetStackCallback(getLZWStackBufferCallback);
    setGetSuffixCallback(getLZWSuffixBufferCallback);
    setGetPrefixCallback(getLZWPrefixBufferCallback);
#endif

    // Seed the random number generator
    randomSeed(analogRead(14));

    Serial.begin(115200);

    // Initialize matrix
    matrix.addLayer(&backgroundLayer); 
#if ENABLE_SCROLLING == 1
    matrix.addLayer(&scrollingLayer); 
#endif

    matrix.begin();

    // for large panels, set the refresh lower to give more CPU time to decoding GIFs
    matrix.setRefreshRate(90);

    // Clear screen
    backgroundLayer.fillScreen(COLOR_BLACK);
    backgroundLayer.swapBuffers();

    // initialize the SD card at full speed
    pinMode(SD_CS, OUTPUT);
    if (!SD.begin(SD_CS)) {
#if ENABLE_SCROLLING == 1
        scrollingLayer.start("No SD card", -1);
#endif
        Serial.println("No SD card");
        while(1);
    }

    // Determine how many animated GIF files exist
    num_files = enumerateGIFFiles(GIF_DIRECTORY, false);

    if(num_files < 0) {
#if ENABLE_SCROLLING == 1
        scrollingLayer.start("No gifs directory", -1);
#endif
        Serial.println("No gifs directory");
        while(1);
    }

    if(!num_files) {
#if ENABLE_SCROLLING == 1
        scrollingLayer.start("Empty gifs directory", -1);
#endif
        Serial.println("Empty gifs directory");
        while(1);
    }
}


void loop() {
    unsigned long futureTime;
    char pathname[30];

    int index = random(num_files);

    // Do forever
    while (true) {
        // Can clear screen for new animation here, but this might cause flicker with short animations
        // matrix.fillScreen(COLOR_BLACK);
        // matrix.swapBuffers();

        getGIFFilenameByIndex(GIF_DIRECTORY, index++, pathname);
        if (index >= num_files) {
            index = 0;
        }

        // Calculate time in the future to terminate animation
        futureTime = millis() + (DISPLAY_TIME_SECONDS * 1000);

        while (futureTime > millis()) {
            processGIFFile(pathname);
        }
    }
}
