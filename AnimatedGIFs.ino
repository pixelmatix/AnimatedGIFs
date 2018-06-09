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

#include "config.h"

// http://esp8266.github.io/Arduino/versions/2.3.0/doc/filesystem.html#uploading-files-to-file-system
// esp8266com/esp8266/libraries/SD/src/File.cpp
#include <FS.h>

#include "GifDecoder.h"
#include "FilenameFunctions.h"

#ifdef ESP8266
extern "C" {
#include "user_interface.h"
}
#endif

#define DISPLAY_TIME_SECONDS 10

// range 0-255
const int defaultBrightness = 255;

#define rgb24 CRGB
const rgb24 COLOR_BLACK = {
    0, 0, 0 };

/* SmartMatrix configuration and memory allocation */
#define COLOR_DEPTH 24                  // known working: 24, 48 - If the sketch uses type `rgb24` directly, COLOR_DEPTH must be 24
const uint8_t kMatrixWidth = 32;        // known working: 32, 64, 96, 128
const uint8_t kMatrixHeight = 32;       // known working: 16, 32, 48, 64


/* template parameters are maxGifWidth, maxGifHeight, lzwMaxBits
 * 
 * The lzwMaxBits value of 12 supports all GIFs, but uses 16kB RAM
 * lzwMaxBits can be set to 10 or 11 for small displays, 12 for large displays
 * All 32x32-pixel GIFs tested work with 11, most work with 10
 */
GifDecoder<kMatrixWidth, kMatrixHeight, 12> decoder;


#define GIF_DIRECTORY "/gifs/"

int num_files;

void screenClearCallback(void) {
  matrix_clear();
}

void updateScreenCallback(void) {
  matrix_show();
}

void drawPixelCallback(int16_t x, int16_t y, uint8_t red, uint8_t green, uint8_t blue) {
  matrixleds[XY(x,y)] = red*65536 + green*256 + blue;
}

// Setup method runs once, when the sketch starts
void setup() {
    Serial.begin(115200);

#ifdef ESP8266
    Serial.println();
    Serial.print( F("Heap: ") ); Serial.println(system_get_free_heap_size());
    Serial.print( F("Boot Vers: ") ); Serial.println(system_get_boot_version());
    Serial.print( F("CPU: ") ); Serial.println(system_get_cpu_freq());
    Serial.print( F("SDK: ") ); Serial.println(system_get_sdk_version());
    Serial.print( F("Chip ID: ") ); Serial.println(system_get_chip_id());
    Serial.print( F("Flash ID: ") ); Serial.println(spi_flash_get_id());
    Serial.print( F("Flash Size: ") ); Serial.println(ESP.getFlashChipRealSize());
    Serial.print( F("Vcc: ") ); Serial.println(ESP.getVcc());
    Serial.println();
#endif

    SPIFFS.begin();
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {
	String fileName = dir.fileName();
	size_t fileSize = dir.fileSize();
	Serial.printf("FS File: %s, size: %s\n", fileName.c_str(), String(fileSize).c_str());
    }
    Serial.printf("\n");

    FastLED.addLeds<WS2811_PORTA,3>(matrixleds, NUMMATRIX/3).setCorrection(TypicalLEDStrip);
    matrix->begin();

    matrix->setBrightness(matrix_brightness);
    decoder.setScreenClearCallback(screenClearCallback);
    decoder.setUpdateScreenCallback(updateScreenCallback);
    decoder.setDrawPixelCallback(drawPixelCallback);

    decoder.setFileSeekCallback(fileSeekCallback);
    decoder.setFilePositionCallback(filePositionCallback);
    decoder.setFileReadCallback(fileReadCallback);
    decoder.setFileReadBlockCallback(fileReadBlockCallback);

    // Seed the random number generator
    randomSeed(analogRead(14));


    // Determine how many animated GIF files exist
    num_files = enumerateGIFFiles(GIF_DIRECTORY, true);

    if(num_files < 0) {
        Serial.println("No gifs directory");
        delay(1000000);
    }

    if(!num_files) {
        Serial.println("Empty gifs directory");
        delay(1000000);
    }
}


void loop() {
    static unsigned long futureTime;

    //int index = random(num_files);
    static int index = -1;

    if(futureTime < millis()) {
        if (++index >= num_files) {
            index = 0;
        }
        Serial.print("Fetching file index #");
        Serial.println(index);

        if (openGifFilenameByIndex(GIF_DIRECTORY, index) >= 0) {
            // Can clear screen for new animation here, but this might cause flicker with short animations
	    // matrix_clear();

            decoder.startDecoding();

            // Calculate time in the future to terminate animation
            futureTime = millis() + (DISPLAY_TIME_SECONDS * 1000);
        }
    }

    decoder.decodeFrame();
}

// vim:sts=4:sw=4
