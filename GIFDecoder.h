#ifndef _GIFDECODER_H_
#define _GIFDECODER_H_

#include <stdint.h>

typedef void (*callback)(void);
typedef void (*pixel_callback)(int16_t x, int16_t y, uint8_t red, uint8_t green, uint8_t blue);

void setScreenClearCallback(callback f);
void setUpdateScreenCallback(callback f);
void setDrawPixelCallback(pixel_callback f);

#endif
