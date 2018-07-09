#ifndef config_h
#define config_h
#include <Adafruit_GFX.h>
#include <FastLED_NeoMatrix.h>
#include <FastLED.h>


//---------------------------------------------------------------------------- 
//

uint8_t matrix_brightness = 32;
float matrix_gamma = 3.0; // higher number is darker

#define M16BY16T4
//#define M32BY8T3P


#ifdef M16BY16T4
#define OFFSETX 0
#define OFFSETY 0

#ifdef ESP8255
#define NEOPIN D1
#else
#define NEOPIN 13
#endif

// Used by LEDMatrix
#define MATRIX_TILE_WIDTH   16 // width of EACH NEOPIXEL MATRIX (not total display)
#define MATRIX_TILE_HEIGHT  16 // height of each matrix
#define MATRIX_TILE_H       2  // number of matrices arranged horizontally
#define MATRIX_TILE_V       2  // number of matrices arranged vertically

// Used by NeoMatrix
#define mw (MATRIX_TILE_WIDTH *  MATRIX_TILE_H)
#define mh (MATRIX_TILE_HEIGHT * MATRIX_TILE_V)
#define NUMMATRIX (mw*mh)

// Compat for some other demos
#define NUM_LEDS NUMMATRIX 
#define MATRIX_HEIGHT mh
#define MATRIX_WIDTH mw

CRGB matrixleds[NUMMATRIX];

// MATRIX DECLARATION:
// Parameter 1 = width of EACH NEOPIXEL MATRIX (not total display)
// Parameter 2 = height of each matrix
// Parameter 3 = number of matrices arranged horizontally
// Parameter 4 = number of matrices arranged vertically
// Parameter 5 = pin number (most are valid)
// Parameter 6 = matrix layout flags, add together as needed:
//   NEO_MATRIX_TOP, NEO_MATRIX_BOTTOM, NEO_MATRIX_LEFT, NEO_MATRIX_RIGHT:
//     Position of the FIRST LED in the FIRST MATRIX; pick two, e.g.
//     NEO_MATRIX_TOP + NEO_MATRIX_LEFT for the top-left corner.
//   NEO_MATRIX_ROWS, NEO_MATRIX_COLUMNS: LEDs WITHIN EACH MATRIX are
//     arranged in horizontal rows or in vertical columns, respectively;
//     pick one or the other.
//   NEO_MATRIX_PROGRESSIVE, NEO_MATRIX_ZIGZAG: all rows/columns WITHIN
//     EACH MATRIX proceed in the same order, or alternate lines reverse
//     direction; pick one.
//   NEO_TILE_TOP, NEO_TILE_BOTTOM, NEO_TILE_LEFT, NEO_TILE_RIGHT:
//     Position of the FIRST MATRIX (tile) in the OVERALL DISPLAY; pick
//     two, e.g. NEO_TILE_TOP + NEO_TILE_LEFT for the top-left corner.
//   NEO_TILE_ROWS, NEO_TILE_COLUMNS: the matrices in the OVERALL DISPLAY
//     are arranged in horizontal rows or in vertical columns, respectively;
//     pick one or the other.
//   NEO_TILE_PROGRESSIVE, NEO_TILE_ZIGZAG: the ROWS/COLUMS OF MATRICES
//     (tiles) in the OVERALL DISPLAY proceed in the same order for every
//     line, or alternate lines reverse direction; pick one.  When using
//     zig-zag order, the orientation of the matrices in alternate rows
//     will be rotated 180 degrees (this is normal -- simplifies wiring).
//   See example below for these values in action.
FastLED_NeoMatrix *matrix = new FastLED_NeoMatrix(matrixleds, MATRIX_TILE_WIDTH, MATRIX_TILE_HEIGHT, MATRIX_TILE_H, MATRIX_TILE_V, 
  NEO_MATRIX_BOTTOM     + NEO_MATRIX_RIGHT +
    NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG + 
    NEO_TILE_TOP + NEO_TILE_RIGHT +  NEO_TILE_PROGRESSIVE);

#elif M32BY8T3P

// If the matrix is not 32x32, adjust how the image is displayed
#define OFFSETX -4
#define OFFSETY 0

// Used by LEDMatrix
#define MATRIX_TILE_WIDTH   8 // width of EACH NEOPIXEL MATRIX (not total display)
#define MATRIX_TILE_HEIGHT  32 // height of each matrix
#define MATRIX_TILE_H       3  // number of matrices arranged horizontally
#define MATRIX_TILE_V       1  // number of matrices arranged vertically

// Used by NeoMatrix
#define mw (MATRIX_TILE_WIDTH *  MATRIX_TILE_H)
#define mh (MATRIX_TILE_HEIGHT * MATRIX_TILE_V)
#define NUMMATRIX (mw*mh)

// Compat for some other demos
#define NUM_LEDS NUMMATRIX 
#define MATRIX_HEIGHT mh
#define MATRIX_WIDTH mw

CRGB matrixleds[NUMMATRIX];

FastLED_NeoMatrix *matrix = new FastLED_NeoMatrix(matrixleds, MATRIX_TILE_WIDTH, MATRIX_TILE_HEIGHT, MATRIX_TILE_H, MATRIX_TILE_V, 
  NEO_MATRIX_TOP     + NEO_MATRIX_RIGHT +
    NEO_MATRIX_ROWS + NEO_MATRIX_ZIGZAG + 
    NEO_TILE_TOP + NEO_TILE_LEFT +  NEO_TILE_PROGRESSIVE);

#else
#error please define the neomatrix array config
#endif

uint16_t XY( uint8_t x, uint8_t y) {
    return matrix->XY(x,y);
}

void matrix_clear() {
    //FastLED[1].clearLedData();
    // clear does not work properly with multiple matrices connected via parallel inputs
#ifdef M32BY8T3P
    memset(matrixleds, 0, NUMMATRIX*MATRIX_TILE_H);
#else
    matrix->clear();
#endif
}

void matrix_show() {
#if 0
#ifdef ESP8266
// Disable watchdog interrupt so that it does not trigger in the middle of
// updates. and break timing of pixels, causing random corruption on interval
// https://github.com/esp8266/Arduino/issues/34
// Note that with https://github.com/FastLED/FastLED/pull/596 interrupts, even
// in parallel mode, should not affect output. That said, reducing their amount
// is still good.
// Well, that sure didn't work, it actually made things worse in a demo during
// fade, so I'm turning it off again.
    //ESP.wdtDisable();
#endif
    FastLED[0].showLeds(matrix_brightness);
#ifdef ESP8266
    //ESP.wdtEnable(1000);
#endif
#endif
    matrix->show();
}

void matrix_setup() {
#ifdef M16BY16T4
    FastLED.addLeds<NEOPIXEL,NEOPIN>(matrixleds, NUMMATRIX).setCorrection(TypicalLEDStrip);
    Serial.print("Neomatrix total LEDs: ");
    Serial.println(NUMMATRIX);
#elif M32BY8T3P
// See  https://github.com/FastLED/FastLED/wiki/Parallel-Output for pin definitions
    #ifdef ESP8266
    FastLED.addLeds<WS2811_PORTA,3>(matrixleds, NUMMATRIX/MATRIX_TILE_H).setCorrection(TypicalLEDStrip);
    #else
    FastLED.addLeds<WS2811_PORTD,3>(matrixleds, NUMMATRIX/MATRIX_TILE_H).setCorrection(TypicalLEDStrip);
    #endif
    Serial.print("Neomatrix parallel output, total LEDs: ");
    Serial.println(NUMMATRIX);
#endif
    matrix->begin();

    matrix->setBrightness(matrix_brightness);
    matrix->precal_gamma(matrix_gamma);
}

#endif // config_h
