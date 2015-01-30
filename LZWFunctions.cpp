/*
 * Animated GIFs Display Code for SmartMatrix and 32x32 RGB LED Panels
 *
 * This file contains code to decompress the LZW encoded animated GIF data
 *
 * Written by: Craig A. Lindley, Fabrice Bellard and Steven A. Bennett
 * See my book, "Practical Image Processing in C", John Wiley & Sons, Inc.
 *
 * Copyright (c) 2014 Craig A. Lindley
 * Minor modifications by Louis Beaudoin (pixelmatix)
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

#define DEBUG 0

#include "Arduino.h"
#include "GIFDecoder.h"

 typedef struct rgb24 {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} rgb24;


extern int lsdWidth;
extern int lsdHeight;
extern int lsdBackgroundIndex;

// Table based image attributes
extern int tbiImageX;
extern int tbiImageY;
extern int tbiWidth;
extern int tbiHeight;
extern boolean tbiInterlaced;

extern int frameDelay;
extern int transparentColorIndex;
extern int disposalMethod;

const int WIDTH  = 32;
const int HEIGHT = 32;

extern rgb24 palette[];

// LZW constants
// NOTE: LZW_MAXBITS set to 11 (initially 10) to support more GIFs with 6k RAM increase
#define LZW_MAXBITS    11
#define LZW_SIZTABLE  (1 << LZW_MAXBITS)

// Masks for 0 .. 16 bits
unsigned int mask[17] = {
    0x0000, 0x0001, 0x0003, 0x0007,
    0x000F, 0x001F, 0x003F, 0x007F,
    0x00FF, 0x01FF, 0x03FF, 0x07FF,
    0x0FFF, 0x1FFF, 0x3FFF, 0x7FFF,
    0xFFFF
};

// LZW variables
byte *pbuf;
int bbits;
int bbuf;
int cursize;                // The current code size
int curmask;
int codesize;
int clear_code;
int end_code;
int newcodes;               // First available code
int top_slot;               // Highest code for current size
int extra_slot;
int slot;                   // Last read code
int fc, oc;
int bs;                     // Current buffer size for GIF
byte *sp;
byte stack  [LZW_SIZTABLE];
byte suffix [LZW_SIZTABLE];
unsigned int prefix [LZW_SIZTABLE];

// Buffer image data is decoded into
byte imageData[1024];

// Backup image data buffer for saving portions of image disposal method == 3
byte imageDataBU[1024];

// Initialize LZW decoder
//   csize initial code size in bits
//   buf input data
void lzw_decode_init (int csize, byte *buf) {

    // Initialize read buffer variables
    pbuf = buf;
    bbuf = 0;
    bbits = 0;
    bs = 0;

    // Initialize decoder variables
    codesize = csize;
    cursize = codesize + 1;
    curmask = mask[cursize];
    top_slot = 1 << cursize;
    clear_code = 1 << codesize;
    end_code = clear_code + 1;
    slot = newcodes = clear_code + 2;
    oc = fc = -1;
    sp = stack;
}

//  Get one code of given number of bits from stream
int lzw_get_code() {

    while (bbits < cursize) {
        if (!bs) {
            bs = *pbuf++;
        }
        bbuf |= (*pbuf++) << bbits;
        bbits += 8;
        bs--;
    }
    int c = bbuf;
    bbuf >>= cursize;
    bbits -= cursize;
    return c & curmask;
}

// Decode given number of bytes
//   buf 8 bit output buffer
//   len number of pixels to decode
//   returns the number of bytes decoded
int lzw_decode(byte *buf, int len) {
    int l, c, code;

    if (end_code < 0) {
        return 0;
    }
    l = len;

    for (;;) {
        while (sp > stack) {
            *buf++ = *(--sp);
            if ((--l) == 0) {
                goto the_end;
            }
        }
        c = lzw_get_code();
        if (c == end_code) {
            break;

        }
        else if (c == clear_code) {
            cursize = codesize + 1;
            curmask = mask[cursize];
            slot = newcodes;
            top_slot = 1 << cursize;
            fc= oc= -1;

        }
        else	{

            code = c;
            if ((code == slot) && (fc >= 0)) {
                *sp++ = fc;
                code = oc;
            }
            else if (code >= slot) {
                break;
            }
            while (code >= newcodes) {
                *sp++ = suffix[code];
                code = prefix[code];
            }
            *sp++ = code;
            if ((slot < top_slot) && (oc >= 0)) {
                suffix[slot] = code;
                prefix[slot++] = oc;
            }
            fc = code;
            oc = c;
            if (slot >= top_slot) {
                if (cursize < LZW_MAXBITS) {
                    top_slot <<= 1;
                    curmask = mask[++cursize];
                } else {
#if DEBUG == 1
                    Serial.println("****** cursize >= MAXBITS *******");
#endif
                }

            }
        }
    }
    end_code = -1;
the_end:
    return len - l;
}

// Decompress LZW data and display animation frame
void decompressAndDisplayFrame() {

    // Each pixel of image is 8 bits and is an index into the palette

        // How the image is decoded depends upon whether it is interlaced or not
    // Decode the interlaced LZW data into the image buffer
    if (tbiInterlaced) {
        // Decode every 8th line starting at line 0
        for (int line = tbiImageY + 0; line < tbiHeight + tbiImageY; line += 8) {
            lzw_decode(imageData + (line * WIDTH) + tbiImageX, tbiWidth);
        }
        // Decode every 8th line starting at line 4
        for (int line = tbiImageY + 4; line < tbiHeight + tbiImageY; line += 8) {
            lzw_decode(imageData + (line * WIDTH) + tbiImageX, tbiWidth);
        }
        // Decode every 4th line starting at line 2
        for (int line = tbiImageY + 2; line < tbiHeight + tbiImageY; line += 4) {
            lzw_decode(imageData + (line * WIDTH) + tbiImageX, tbiWidth);
        }
        // Decode every 2nd line starting at line 1
        for (int line = tbiImageY + 1; line < tbiHeight + tbiImageY; line += 2) {
            lzw_decode(imageData + (line * WIDTH) + tbiImageX, tbiWidth);
        }
    }
    else	{
        // Decode the non interlaced LZW data into the image data buffer
        for (int line = tbiImageY; line < tbiHeight + tbiImageY; line++) {
            lzw_decode(imageData  + (line * WIDTH) + tbiImageX, tbiWidth);
        }
    }

    // Image data is decompressed, now display portion of image affected by frame

    rgb24 color;
    int yOffset, pixel;
    for (int y = tbiImageY; y < tbiHeight + tbiImageY; y++) {
        yOffset = y * WIDTH;
        for (int x = tbiImageX; x < tbiWidth + tbiImageX; x++) {
            // Get the next pixel
            pixel = imageData[yOffset + x];

            // Check pixel transparency
            if (pixel == transparentColorIndex) {
                continue;
            }

            // Pixel not transparent so get color from palette
            color.red   = palette[pixel].red;
            color.green = palette[pixel].green;
            color.blue  = palette[pixel].blue;

            // Draw the pixel
            if(drawPixelCallback)
                (*drawPixelCallback)(x, y, color.red, color.green, color.blue);
        }
    }
    // Make animation frame visible
    // swapBuffers() call can take up to 1/framerate seconds to return (it waits until a buffer copy is complete)
    // note the time before calling
    int nextFrameTime_ms = millis() + (10 * frameDelay);
    if(updateScreenCallback)
        (*updateScreenCallback)();

    // get the number of milliseconds to delay
    nextFrameTime_ms -= millis();

    // the space between frames isn't perfect as there is a variable amount of time to process the next frame, but this gets it close
    if(nextFrameTime_ms > 0)
        delay(nextFrameTime_ms);
}
