#ifndef _GIFDECODER_H_
#define _GIFDECODER_H_

#include <stdint.h>

typedef void (*callback)(void);
typedef void (*pixel_callback)(int16_t x, int16_t y, uint8_t red, uint8_t green, uint8_t blue);
typedef byte (*get_byte_callback)(void);
typedef void* (*get_buffer_callback)(void);

void setScreenClearCallback(callback f);
void setUpdateScreenCallback(callback f);
void setDrawPixelCallback(pixel_callback f);

void setGetStackCallback(get_buffer_callback f);
void setGetSuffixCallback(get_buffer_callback f);
void setGetPrefixCallback(get_buffer_callback f);

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

void lzw_decode_init (int csize, get_byte_callback f);
int lzw_decode(byte *buf, int len);
void decompressAndDisplayFrame(unsigned long filePositionAfter);

#endif
