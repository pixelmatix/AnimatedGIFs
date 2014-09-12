/*
 * Animated GIFs Display Code for 32x32 RGB LED Smart Pixel Matrix
 *
 * Uses SmartMatrix Library for Teensy 3.1 written by Louis Beaudoin at pixelmatix.com
 *
 * Written by: Craig A. Lindley
 *
 * Copyright (c) 2014 Craig A. Lindley
 * Minor modifications by Louis Beaudoin (Pixelmatix)
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

#include <math.h>
#include <stdlib.h>
#include <SdFat.h>
#include "SmartMatrix_32x32.h"
#include <FastLED.h>

#define DISPLAY_TIME_SECONDS 10

// Declared in FilenameFunctions.cpp
extern int numberOfFiles;
extern int enumerateGIFFiles(const char *directoryName, boolean displayFilenames);
extern void getGIFFilenameByIndex(const char *directoryName, int index, char *pnBuffer);
extern void chooseRandomGIFFilename(const char *directoryName, char *pnBuffer);

// Declared in GIFParseFunctions.cpp
extern int processGIFFile(const char * pathname);

const int defaultBrightness = 50;
const rgb24 COLOR_BLACK = {
    0, 0, 0};

const int WIDTH  = 32;
const int HEIGHT = 32;

// Smart Matrix instance
SmartMatrix matrix;

// Chip select for SD card on my hardware
#define SD_CS 15
// SD card instance
SdFat sd;

#define GIF_DIRECTORY "/gifs/"

// Setup method runs once, when the sketch starts
void setup() {

    // Seed the random number generator
    randomSeed(analogRead(14));

    Serial.begin(115200);

    // Wait for serial to settle
    delay(2000);

    // Initialize matrix
    matrix.begin();
    matrix.setBrightness(defaultBrightness);

    // Clear screen
    matrix.fillScreen(COLOR_BLACK);
    matrix.swapBuffers();

    // initialize the SD card at full speed
    if (! sd.begin(SD_CS, SPI_HALF_SPEED)) {
        sd.initErrorHalt();
    }

    // Determine how many animated GIF files exist
    enumerateGIFFiles(GIF_DIRECTORY, false);
}


void loop() {

    unsigned long futureTime;
    char pathname[30];

    int index = random(numberOfFiles);

    // Do forever
    while (true) {

        // Clear screen for new animation
        matrix.fillScreen(COLOR_BLACK);
        matrix.swapBuffers();

        delay(1000);

        getGIFFilenameByIndex(GIF_DIRECTORY, index++, pathname);
        if (index >= numberOfFiles) {
            index = 0;
        }

        // Calculate time in the future to terminate animation
        futureTime = millis() + (DISPLAY_TIME_SECONDS * 1000);

        while (futureTime > millis()) {
            processGIFFile(pathname);
        }
    }
}


























