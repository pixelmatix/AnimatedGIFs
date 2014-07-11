/*
 * Animated GIFs Display Code for 32x32 RGB LED Smart Pixel Matrix
 *
 * Uses Teensy 3.1 driver written by Louis Beaudoin at pixelmatrix.com
 *
 * Written by: Craig A. Lindley
 * Version: 1.2
 * Last Update: 07/04/2014
 */

#include <math.h>
#include <stdlib.h>
#include <SdFat.h>
#include "SmartMatrix_32x32.h"

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
    matrix.setColorCorrection(cc24);

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


























