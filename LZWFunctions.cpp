/*
 * Animated GIFs Display Code for 32x32 RGB LED Matrix
 *
 * This file contains code to decompress the LZW encoded animated GIF data
 *
 * Written by: Craig A. Lindley, Fabrice Bellard and Steven A. Bennett
 * See my book, "Practical Image Processing in C", John Wiley & Sons, Inc.
 *
 * Version: 1.1
 * Last Update: 06/18/2014
 */

#include "SmartMatrix.h"

const int WIDTH  = 32;
const int HEIGHT = 32;

extern SmartMatrix matrix;

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

// RGB data structure
typedef struct {
    byte Red;
    byte Green;
    byte Blue;
} 
RGB;

extern RGB palette[];

#define NO_TRANSPARENT_INDEX 8192

// LZW constants
// NOTE: LZW_MAXBITS set to 10 to save memory
#define LZW_MAXBITS    10
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
        for (int line = 0; line < tbiHeight; line += 8) {				
            lzw_decode(imageData + (line * WIDTH), tbiWidth);
        }			
        // Decode every 8th line starting at line 4
        for (int line = 4; line < tbiHeight; line += 8) {				
            lzw_decode(imageData + (line * WIDTH), tbiWidth);
        }			
        // Decode every 4th line starting at line 2
        for (int line = 2; line < tbiHeight; line += 4) {				
            lzw_decode(imageData + (line * WIDTH), tbiWidth);
        }			
        // Decode every 2nd line starting at line 1
        for (int line = 1; line < tbiHeight; line += 2) {				
            lzw_decode(imageData + (line * WIDTH), tbiWidth);
        }
    }	
    else	{			
        // Decode the non interlaced LZW data into the image data buffer
        for (int line = tbiImageY; line < tbiHeight + tbiImageY; line++) {				
            lzw_decode(imageData  + (line * WIDTH) + tbiImageX, tbiWidth);
        }
    }
    // Image data is decompressed, now display it
    // GIF data stored left to right and top to bottom
    rgb24 color;
    byte pixel;
    int pixelIndex = 0;
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            // Get the next pixel
            pixel = imageData[pixelIndex++];

            // If pixel is transparent, then skip processing it
            if (pixel == transparentColorIndex) {
                continue;
            }

            color.red   = palette[pixel].Red;
            color.green = palette[pixel].Green;
            color.blue  = palette[pixel].Blue;

            // Draw the pixel
            matrix.drawPixel(x, y, color);
        }
    }
    // Make animation frame visible
    matrix.swapBuffers();

    if (disposalMethod == 2) {
        memset(imageData, lsdBackgroundIndex, sizeof(imageData));
    }
}











