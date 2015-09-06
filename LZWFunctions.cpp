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

#include <Arduino.h>
#include "GIFDecoder.h"

// LZW constants
// NOTE: LZW_MAXBITS should be set to 10 or 11 for small displays, 12 for large displays
//   all 32x32-pixel GIFs tested work with 11, most work with 10
//   LZW_MAXBITS = 12 will support all GIFs, but takes 16kB RAM
#define LZW_MAXBITS    12
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
int bcnt;
byte *sp;
get_bytes_callback getBytesCallback;
byte * temp_buffer;

byte stack  [LZW_SIZTABLE];
byte suffix [LZW_SIZTABLE];
uint16_t prefix [LZW_SIZTABLE];

void lzw_setTempBuffer(byte * tempBuffer) {
    temp_buffer = tempBuffer;
}

// Initialize LZW decoder
//   csize initial code size in bits
//   buf input data
void lzw_decode_init (int csize, get_bytes_callback f) {

    getBytesCallback = f;

    // Initialize read buffer variables
    bbuf = 0;
    bbits = 0;
    bs = 0;
    bcnt = 0;

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
        if (bcnt == bs) {
            // get number of bytes in next block
            (*getBytesCallback)(temp_buffer, 1);
            bs = temp_buffer[0];
            (*getBytesCallback)(temp_buffer, bs);
            bcnt = 0;
        }
        bbuf |= temp_buffer[bcnt] << bbits;
        bbits += 8;
        bcnt++;
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

#if DEBUG == 1
    unsigned char debugMessagePrinted = 0;
#endif

    if (end_code < 0) {
        return 0;
    }
    l = len;

    for (;;) {
        while (sp > stack) {
            *buf++ = *(--sp);
            if ((--l) == 0) {
                return len;
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
                    if(!debugMessagePrinted) {
                        debugMessagePrinted = 1;
                        Serial.println("****** cursize >= MAXBITS *******");
                    }
#endif
                }

            }
        }
    }
    end_code = -1;
    return len - l;
}
