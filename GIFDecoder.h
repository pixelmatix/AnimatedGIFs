#ifndef _GIFDECODER_H_
#define _GIFDECODER_H_

#include <stdint.h>

typedef void (*callback)(void);
typedef void (*pixel_callback)(int16_t x, int16_t y, uint8_t red, uint8_t green, uint8_t blue);

void setScreenClearCallback(callback f);
void setUpdateScreenCallback(callback f);
void setDrawPixelCallback(pixel_callback f);


// public
int enumerateGIFFiles(const char *directoryName, boolean displayFilenames);
void getGIFFilenameByIndex(const char *directoryName, int index, char *pnBuffer);
void chooseRandomGIFFilename(const char *directoryName, char *pnBuffer);
int processGIFFile(const char * pathname);

// private
#ifndef SmartMatrix_h
 typedef struct rgb24 {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} rgb24;
#endif

void lzw_decode_init (int csize, byte *buf);
void decompressAndDisplayFrame();
extern byte imageData[1024];
extern byte imageDataBU[1024];

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

extern rgb24 palette[];

#endif
