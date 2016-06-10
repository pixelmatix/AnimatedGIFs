#ifndef _GIFDECODER_H_
#define _GIFDECODER_H_

#include <stdint.h>

typedef void (*callback)(void);
typedef void (*pixel_callback)(int16_t x, int16_t y, uint8_t red, uint8_t green, uint8_t blue);
typedef int (*get_bytes_callback)(void * buffer, int numberOfBytes);
typedef void* (*get_buffer_callback)(void);

typedef struct rgb_24 {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} rgb_24;

class GifDecoder {
public:
    int processGIFFile(const char * pathname);
    
    void setScreenClearCallback(callback f);
    void setUpdateScreenCallback(callback f);
    void setDrawPixelCallback(pixel_callback f);
    void setStartDrawingCallback(callback f);

private:
    void parseTableBasedImage(void);
    void decompressAndDisplayFrame(unsigned long filePositionAfter);
    int parseData(void);
    int parseGIFFileTerminator(void);
    void parseCommentExtension(void);
    void parseApplicationExtension(void);
    void parseGraphicControlExtension(void);
    void parsePlainTextExtension(void);
    void parseGlobalColorTable(void);
    void parseLogicalScreenDescriptor(void);
    bool parseGifHeader(void);
    void copyImageDataRect(byte *dst, byte *src, int x, int y, int width, int height);
    void fillImageData(byte colorIndex);
    void fillImageDataRect(byte colorIndex, int x, int y, int width, int height);
    static int readIntoBuffer(void *buffer, int numberOfBytes);
    int readWord(void);
    void backUpStream(int n);
    int readByte(void);

    void lzw_decode_init(int csize, get_bytes_callback f);
    int lzw_decode(byte *buf, int len, byte *bufend);
    void lzw_setTempBuffer(byte * tempBuffer);
};

#endif
