/*
    Animated GIFs Display Code for SmartMatrix and 32x32 RGB LED Panels

    This file contains code to parse animated GIF files

    Written by: Craig A. Lindley

    Copyright (c) 2014 Craig A. Lindley
    Minor modifications by Louis Beaudoin (pixelmatix)

    Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/

//#define GIFDEBUG 2

#include <Arduino.h>

#include "GifDecoder.h"

// Error codes
#define ERROR_NONE                      0
#define ERROR_DONE_PARSING              1
#define ERROR_WAITING                   2
#define ERROR_FILEOPEN                  -1
#define ERROR_FILENOTGIF                -2
#define ERROR_BADGIFFORMAT              -3
#define ERROR_UNKNOWNCONTROLEXT         -4
#define ERROR_GIF_TOO_WIDE              -5
#define ERROR_GIF_INVALID_PARAMETER     -6
#define ERROR_GIF_UNSUPPORTED_FEATURE   -7
#define ERROR_GIF_EARLY_EOF             -8
#define ERROR_GIF_EMPTY_FRAME           -9
#define ERROR_GIF_DECODE_ERROR          -10

template <int maxGifWidth, int maxGifHeight, int lzwMaxBits>
callback GifDecoder<maxGifWidth, maxGifHeight, lzwMaxBits>::screenClearCallback;
template <int maxGifWidth, int maxGifHeight, int lzwMaxBits>
callback GifDecoder<maxGifWidth, maxGifHeight, lzwMaxBits>::updateScreenCallback;
template <int maxGifWidth, int maxGifHeight, int lzwMaxBits>
pixel_callback GifDecoder<maxGifWidth, maxGifHeight, lzwMaxBits>::drawPixelCallback;
template <int maxGifWidth, int maxGifHeight, int lzwMaxBits>
line_callback GifDecoder<maxGifWidth, maxGifHeight, lzwMaxBits>::drawLineCallback;
template <int maxGifWidth, int maxGifHeight, int lzwMaxBits>
callback GifDecoder<maxGifWidth, maxGifHeight, lzwMaxBits>::startDrawingCallback;
template <int maxGifWidth, int maxGifHeight, int lzwMaxBits>
file_seek_callback GifDecoder<maxGifWidth, maxGifHeight, lzwMaxBits>::fileSeekCallback;
template <int maxGifWidth, int maxGifHeight, int lzwMaxBits>
file_position_callback GifDecoder<maxGifWidth, maxGifHeight, lzwMaxBits>::filePositionCallback;
template <int maxGifWidth, int maxGifHeight, int lzwMaxBits>
file_read_callback GifDecoder<maxGifWidth, maxGifHeight, lzwMaxBits>::fileReadCallback;
template <int maxGifWidth, int maxGifHeight, int lzwMaxBits>
file_read_block_callback GifDecoder<maxGifWidth, maxGifHeight, lzwMaxBits>::fileReadBlockCallback;
template <int maxGifWidth, int maxGifHeight, int lzwMaxBits>
file_size_callback GifDecoder<maxGifWidth, maxGifHeight, lzwMaxBits>::fileSizeCallback;


template <int maxGifWidth, int maxGifHeight, int lzwMaxBits>
void GifDecoder<maxGifWidth, maxGifHeight, lzwMaxBits>::setStartDrawingCallback(
    callback f) {
  startDrawingCallback = f;
}

template <int maxGifWidth, int maxGifHeight, int lzwMaxBits>
void GifDecoder<maxGifWidth, maxGifHeight, lzwMaxBits>::setUpdateScreenCallback(
    callback f) {
  updateScreenCallback = f;
}

template <int maxGifWidth, int maxGifHeight, int lzwMaxBits>
void GifDecoder<maxGifWidth, maxGifHeight, lzwMaxBits>::setDrawPixelCallback(
    pixel_callback f) {
  drawPixelCallback = f;
}

template <int maxGifWidth, int maxGifHeight, int lzwMaxBits>
void GifDecoder<maxGifWidth, maxGifHeight, lzwMaxBits>::setDrawLineCallback(
    line_callback f) {
  drawLineCallback = f;
}

template <int maxGifWidth, int maxGifHeight, int lzwMaxBits>
void GifDecoder<maxGifWidth, maxGifHeight, lzwMaxBits>::setScreenClearCallback(
    callback f) {
  screenClearCallback = f;
}

template <int maxGifWidth, int maxGifHeight, int lzwMaxBits>
void GifDecoder<maxGifWidth, maxGifHeight, lzwMaxBits>::setFileSeekCallback(
    file_seek_callback f) {
  fileSeekCallback = f;
}

template <int maxGifWidth, int maxGifHeight, int lzwMaxBits>
void GifDecoder<maxGifWidth, maxGifHeight, lzwMaxBits>::setFilePositionCallback(
    file_position_callback f) {
  filePositionCallback = f;
}

template <int maxGifWidth, int maxGifHeight, int lzwMaxBits>
void GifDecoder<maxGifWidth, maxGifHeight, lzwMaxBits>::setFileReadCallback(
    file_read_callback f) {
  fileReadCallback = f;
}

template <int maxGifWidth, int maxGifHeight, int lzwMaxBits>
void GifDecoder<maxGifWidth, maxGifHeight, lzwMaxBits>::setFileSizeCallback(
    file_size_callback f) {
  fileSizeCallback = f;
}

template <int maxGifWidth, int maxGifHeight, int lzwMaxBits>
void GifDecoder<maxGifWidth, maxGifHeight, lzwMaxBits>::setFileReadBlockCallback(file_read_block_callback f) {
  fileReadBlockCallback = f;
}

template <int maxGifWidth, int maxGifHeight, int lzwMaxBits>
void GifDecoder<maxGifWidth, maxGifHeight, lzwMaxBits>::DrawPixelRow(int startX, int y, int numPixels, rgb_24 * data) {
  for(int i=0; i<numPixels; i++)
  {
    if(drawPixelCallback)
        (*drawPixelCallback)(startX + i, y, data->red, data->green, data->blue);
    data++;
  }
}

template <int maxGifWidth, int maxGifHeight, int lzwMaxBits>
void GifDecoder<maxGifWidth, maxGifHeight, lzwMaxBits>::GIFDraw(GIFDRAW *pDraw) {
  int x_offset = 0;
  int y_offset = 0;

  uint8_t *s;
  //uint16_t *d, *usPalette, usTemp[320];
  int x, y, iWidth;
  rgb_24 *d, *usPalette, usTemp[320];

  iWidth = pDraw->iWidth;
  if (iWidth > DISPLAY_WIDTH)
    iWidth = DISPLAY_WIDTH;
  usPalette = (rgb_24*)pDraw->pPalette;

  y = pDraw->iY + pDraw->y; // current line
  
  s = pDraw->pPixels;
  if (pDraw->ucDisposalMethod == 2) // restore to background color
  {
    for (x=0; x<iWidth; x++)
    {
      if (s[x] == pDraw->ucTransparent)
        s[x] = pDraw->ucBackground;
    }
    pDraw->ucHasTransparency = 0;
  }
  // Apply the new pixels to the main image
  if (pDraw->ucHasTransparency) // if transparency used
  {
    uint8_t *pEnd, c, ucTransparent = pDraw->ucTransparent;
    int x, iCount;
    pEnd = s + iWidth;
    x = 0;
    iCount = 0; // count non-transparent pixels
    while(x < iWidth)
    {
      c = ucTransparent-1;
      d = usTemp;
      while (c != ucTransparent && s < pEnd)
      {
        c = *s++;
        if (c == ucTransparent) // done, stop
        {
          s--; // back up to treat it like transparent
        }
        else // opaque
        {
          *d++ = usPalette[c];
          iCount++;
        }
      } // while looking for opaque pixels
      if (iCount) // any opaque pixels?
      {
        DrawPixelRow(pDraw->iX+x+x_offset, y+y_offset, iCount, (rgb_24 *)usTemp);
        x += iCount;
        iCount = 0;
      }
      // no, look for a run of transparent pixels
      c = ucTransparent;
      while (c == ucTransparent && s < pEnd)
      {
        c = *s++;
        if (c == ucTransparent)
          iCount++;
        else
          s--; 
      }
      if (iCount)
      {
        x += iCount; // skip these
        iCount = 0;
      }
    }
  }
  else
  {
    s = pDraw->pPixels;
    // Translate the 8-bit pixels through the RGB565 palette (already byte reversed)
    for (x=0; x<iWidth; x++)
      usTemp[x] = usPalette[*s++];
      DrawPixelRow(pDraw->iX+x_offset, y+y_offset, iWidth, (rgb_24 *)usTemp);
  }
}

template <int maxGifWidth, int maxGifHeight, int lzwMaxBits>
void * GifDecoder<maxGifWidth, maxGifHeight, lzwMaxBits>::GIFOpenFile(const char *fname, int32_t *pSize) {
  // need to get file size, not part of current FilenameFunctions API
  if(fileSizeCallback)
    *pSize = fileSizeCallback();

  // we don't use GIFFile.fHandle, but AnimatedGIF wants to make sure the handle isn't NULL, return a non-zero number
  return (void*)1;
}

template <int maxGifWidth, int maxGifHeight, int lzwMaxBits>
void GifDecoder<maxGifWidth, maxGifHeight, lzwMaxBits>::GIFCloseFile(void *pHandle) {

}

template <int maxGifWidth, int maxGifHeight, int lzwMaxBits>
int32_t GifDecoder<maxGifWidth, maxGifHeight, lzwMaxBits>::GIFReadFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen) {
  int32_t iBytesRead;
  iBytesRead = iLen;
  //File *f = static_cast<File *>(pFile->fHandle);
  // Note: If you read a file all the way to the last byte, seek() stops working
  if ((pFile->iSize - pFile->iPos) < iLen)
    iBytesRead = pFile->iSize - pFile->iPos - 1; // <-- ugly work-around
  if (iBytesRead <= 0)
    return 0;
  //Serial.print("fileread: ");
  //Serial.print(iBytesRead);
  iBytesRead = (int32_t)fileReadBlockCallback(pBuf, iBytesRead);
  //Serial.print(",");
  //Serial.println(iBytesRead);

  pFile->iPos = (int32_t)filePositionCallback();
  return iBytesRead;
}

template <int maxGifWidth, int maxGifHeight, int lzwMaxBits>
int32_t GifDecoder<maxGifWidth, maxGifHeight, lzwMaxBits>::GIFSeekFile(GIFFILE *pFile, int32_t iPosition) {
  fileSeekCallback(iPosition);
  pFile->iPos = (int32_t)filePositionCallback();
  return pFile->iPos;
}

template <int maxGifWidth, int maxGifHeight, int lzwMaxBits>
int GifDecoder<maxGifWidth, maxGifHeight, lzwMaxBits>::startDecoding(void) {
  if(!beginCalled) {
    beginCalled = true;

    // using RGB888 = rgb24 palette instead of default RGB565
    gif.begin(BIG_ENDIAN_PIXELS, GIF_PALETTE_RGB888);
  }

  cycleNumber = 0;
  cycleTime = 0;
  frameStartTime = micros();

  screenClearCallback();

  // file is already open, and we don't know the name, send a 0-length string instead
  if (gif.open("", GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDraw))
  {
    Serial.printf("Successfully opened GIF; Canvas size = %d x %d\n", gif.getCanvasWidth(), gif.getCanvasHeight());
#if 0
    GIFINFO gi;
    if (gif.getInfo(&gi)) {
      Serial.printf("frame count: %d\n", gi.iFrameCount);
      Serial.printf("duration: %d ms\n", gi.iDuration);
      Serial.printf("max delay: %d ms\n", gi.iMaxDelay);
      Serial.printf("min delay: %d ms\n", gi.iMinDelay);
    }
#endif
    Serial.flush();
  } else {
    Serial.print("open failed: ");
    Serial.println(gif.getLastError());
    return translateGifErrorCode(gif.getLastError());    
  }

  return ERROR_NONE;
}

template <int maxGifWidth, int maxGifHeight, int lzwMaxBits>
int GifDecoder<maxGifWidth, maxGifHeight, lzwMaxBits>::translateGifErrorCode(int code) {
  switch (gif.getLastError()) {
    case GIF_SUCCESS:
      return ERROR_NONE;
    case  GIF_DECODE_ERROR:
      return ERROR_GIF_DECODE_ERROR;
    case  GIF_TOO_WIDE:
      return ERROR_GIF_TOO_WIDE;
    case  GIF_INVALID_PARAMETER:
      return ERROR_GIF_INVALID_PARAMETER;
    case  GIF_UNSUPPORTED_FEATURE:
      return ERROR_GIF_UNSUPPORTED_FEATURE;
    case  GIF_FILE_NOT_OPEN:
      return ERROR_FILEOPEN;
    case  GIF_EARLY_EOF:
      return ERROR_GIF_EARLY_EOF;
    case  GIF_EMPTY_FRAME:
      return ERROR_GIF_EMPTY_FRAME;
    case  GIF_BAD_FILE:
      return ERROR_BADGIFFORMAT;
    default:
      return -99;
  }
}

template <int maxGifWidth, int maxGifHeight, int lzwMaxBits>
int GifDecoder<maxGifWidth, maxGifHeight, lzwMaxBits>::decodeFrame(bool delayAfterDecode) {
  // Parse gif data
  int frameStatus;

  frameStatus = gif.playFrame(delayAfterDecode, &frameDelay_ms);

  if(updateScreenCallback)
      (*updateScreenCallback)();

  if(frameStatus < 0) {
    Serial.print("playFrame failed: ");
    Serial.println(gif.getLastError());

    return translateGifErrorCode(gif.getLastError());    
  }

  frameNumber++;
  // only track cycleTime on first frame
  if(!frameNumber)
    cycleTime += frameDelay_ms;

  // if done parsing
  if (frameStatus == 0) {
    frameCount = frameNumber;
    frameNumber = 0;
    cycleNumber++;
    frameStartTime = micros();

    fileSeekCallback(0);

    // file is already open, and we don't know the name, send a 0-length string instead
    gif.open("", GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDraw);

    return ERROR_DONE_PARSING;
  }

  return ERROR_NONE;
}
