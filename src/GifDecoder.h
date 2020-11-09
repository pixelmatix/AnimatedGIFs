#ifndef _GIFDECODER_H_
#define _GIFDECODER_H_

#include <stdint.h>
#include <AnimatedGIF.h>

#define DISPLAY_WIDTH maxGifWidth
#define DISPLAY_HEIGHT maxGifHeight

typedef void (*callback)(void);
typedef void (*pixel_callback)(int16_t x, int16_t y, uint8_t red, uint8_t green,
                               uint8_t blue);
typedef void (*line_callback)(int16_t x, int16_t y, uint8_t *buf, int16_t wid,
                              uint16_t *palette565, int16_t skip);
typedef void *(*get_buffer_callback)(void);

typedef bool (*file_seek_callback)(unsigned long position);
typedef unsigned long (*file_position_callback)(void);
typedef int (*file_read_callback)(void);
typedef int (*file_read_block_callback)(void *buffer, int numberOfBytes);
typedef int (*file_size_callback)(void);

typedef struct rgb_24 {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
} rgb_24;

template <int maxGifWidth, int maxGifHeight, int lzwMaxBits> class GifDecoder {
public:
  int startDecoding(void);
  int decodeFrame(bool delayAfterDecode = true);
  int getCycleTime(void) { return cycleTime; }  // only valid when cycleNumber > 0, ideal number of ms to play one cycle of GIF
  int getFrameNumber(void) { return frameNumber; } // only valid when cycleNumber > 0, number of frames in one cycle of GIF
  int getCycleNumber(void) { return cycleNumber; } // number indicates number of cycles the GIF has gone through, 0 on first pass,
  int getFrameCount(void) { return frameCount; } // count of the current frame decoded, resets on each cycle
  unsigned int getFrameDelay_ms(void) { return frameDelay_ms; } // delay of the last frame decoded
  void getSize(uint16_t *w, uint16_t *h) {
    *w = gif.getCanvasWidth();
    *h = gif.getCanvasHeight();
  }

  void setScreenClearCallback(callback f);
  void setUpdateScreenCallback(callback f);
  void setDrawPixelCallback(pixel_callback f);
  void setDrawLineCallback(line_callback f);
  void setStartDrawingCallback(callback f); // note this is not called when NO_IMAGEDATA == 2, and has not been tested recently

  void setFileSeekCallback(file_seek_callback f);
  void setFilePositionCallback(file_position_callback f);
  void setFileReadCallback(file_read_callback f);
  void setFileReadBlockCallback(file_read_block_callback f);
  void setFileSizeCallback(file_size_callback f);

private:
  AnimatedGIF gif;

  bool beginCalled;

  int cycleNumber;
  int cycleTime;
  unsigned long frameNumber;
  int frameCount;
  int frameDelay_ms;

  uint32_t frameStartTime;

  static callback screenClearCallback;
  static callback updateScreenCallback;
  static pixel_callback drawPixelCallback;
  static line_callback drawLineCallback;
  static callback startDrawingCallback;
  static file_seek_callback fileSeekCallback;
  static file_position_callback filePositionCallback;
  static file_read_callback fileReadCallback;
  static file_read_block_callback fileReadBlockCallback;
  static file_size_callback fileSizeCallback;

  static void GIFDraw(GIFDRAW *pDraw);
  static void * GIFOpenFile(const char *fname, int32_t *pSize);
  static void GIFCloseFile(void *pHandle);
  static int32_t GIFReadFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen);
  static int32_t GIFSeekFile(GIFFILE *pFile, int32_t iPosition);
  static void DrawPixelRow(int startX, int y, int numPixels, rgb_24 * data);
  int translateGifErrorCode(int code);
};

#include "GifDecoder_Impl.h"

#endif
