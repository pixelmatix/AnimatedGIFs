/*
 * Animated GIFs Display Code for SmartMatrix and 32x32 RGB LED Panels
 *
 * This file contains code to parse animated GIF files
 *
 * Written by: Craig A. Lindley
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

#define GIFDEBUG 0

#if GIFDEBUG == 1
#define DEBUG_SCREEN_DESCRIPTOR                             1
#define DEBUG_GLOBAL_COLOR_TABLE                            1
#define DEBUG_PROCESSING_PLAIN_TEXT_EXT                     1
#define DEBUG_PROCESSING_GRAPHIC_CONTROL_EXT                1
#define DEBUG_PROCESSING_APP_EXT                            1
#define DEBUG_PROCESSING_COMMENT_EXT                        1
#define DEBUG_PROCESSING_FILE_TERM                          1
#define DEBUG_PROCESSING_TABLE_IMAGE_DESC                   1
#define DEBUG_PROCESSING_TBI_DESC_START                     1
#define DEBUG_PROCESSING_TBI_DESC_INTERLACED                1
#define DEBUG_PROCESSING_TBI_DESC_LOCAL_COLOR_TABLE         1
#define DEBUG_PROCESSING_TBI_DESC_LZWCODESIZE               1
#define DEBUG_PROCESSING_TBI_DESC_DATABLOCKSIZE             1
#define DEBUG_PROCESSING_TBI_DESC_LZWIMAGEDATA_OVERFLOW     1
#define DEBUG_PROCESSING_TBI_DESC_LZWIMAGEDATA_SIZE         1
#define DEBUG_PARSING_DATA                                  1
#define DEBUG_DECOMPRESS_AND_DISPLAY                        1

#define DEBUG_WAIT_FOR_KEY_PRESS                            0

#endif

#include <Arduino.h>
#include <SD.h>
#include "GIFDecoder.h"

File file;

const int WIDTH  = 32;
const int HEIGHT = 32;
// maximum bounds of the decoded GIF (dimensions of imageData buffer)

// Error codes
#define ERROR_NONE		           0
#define ERROR_FILEOPEN		       -1
#define ERROR_FILENOTGIF	       -2
#define ERROR_BADGIFFORMAT         -3
#define ERROR_UNKNOWNCONTROLEXT	   -4

#define GIFHDRTAGNORM   "GIF87a"  // tag in valid GIF file
#define GIFHDRTAGNORM1  "GIF89a"  // tag in valid GIF file
#define GIFHDRSIZE 6

// Global GIF specific definitions
#define COLORTBLFLAG    0x80
#define INTERLACEFLAG   0x40
#define TRANSPARENTFLAG 0x01

#define NO_TRANSPARENT_INDEX -1

// Disposal methods
#define DISPOSAL_NONE       0
#define DISPOSAL_LEAVE      1
#define DISPOSAL_BACKGROUND 2
#define DISPOSAL_RESTORE    3

// Logical screen descriptor attributes
int lsdWidth;
int lsdHeight;
int lsdPackedField;
int lsdAspectRatio;
int lsdBackgroundIndex;

// Table based image attributes
int tbiImageX;
int tbiImageY;
int tbiWidth;
int tbiHeight;
int tbiPackedBits;
boolean tbiInterlaced;

int frameDelay;
int transparentColorIndex;
int prevBackgroundIndex;
int prevDisposalMethod;
int disposalMethod;
int lzwCodeSize;
boolean keyFrame;
int rectX;
int rectY;
int rectWidth;
int rectHeight;

unsigned long nextFrameTime_ms;

int colorCount;
rgb_24 palette[256];

char tempBuffer[260];

// Buffer image data is decoded into
byte imageData[WIDTH * HEIGHT];

// Backup image data buffer for saving portions of image disposal method == 3
byte imageDataBU[WIDTH * HEIGHT];

callback screenClearCallback;
callback updateScreenCallback;
pixel_callback drawPixelCallback;
callback startDrawingCallback;

void GifDecoder::setStartDrawingCallback(callback f) {
    startDrawingCallback = f;
}

void GifDecoder::setUpdateScreenCallback(callback f) {
    updateScreenCallback = f;
}

void GifDecoder::setDrawPixelCallback(pixel_callback f) {
    drawPixelCallback = f;
}

void GifDecoder::setScreenClearCallback(callback f) {
    screenClearCallback = f;
}

// Backup the read stream by n bytes
void GifDecoder::backUpStream(int n) {
    file.seek(file.position() - n);
}

// Read a file byte
int GifDecoder::readByte() {

    int b = file.read();
    if (b == -1) {
        Serial.println("Read error or EOF occurred");
    }
    return b;
}

// Read a file word
int GifDecoder::readWord() {

    int b0 = readByte();
    int b1 = readByte();
    return (b1 << 8) | b0;
}

// Read the specified number of bytes into the specified buffer
int GifDecoder::readIntoBuffer(void *buffer, int numberOfBytes) {

    int result = file.read(buffer, numberOfBytes);
    if (result == -1) {
        Serial.println("Read error or EOF occurred");
    }
    return result;
}

// Fill a portion of imageData buffer with a color index
void GifDecoder::fillImageDataRect(byte colorIndex, int x, int y, int width, int height) {

    int yOffset;

    for (int yy = y; yy < height + y; yy++) {
        yOffset = yy * WIDTH;
        for (int xx = x; xx < width + x; xx++) {
            imageData[yOffset + xx] = colorIndex;
        }
    }
}

// Fill entire imageData buffer with a color index
void GifDecoder::fillImageData(byte colorIndex) {

    memset(imageData, colorIndex, sizeof(imageData));
}

// Copy image data in rect from a src to a dst
void GifDecoder::copyImageDataRect(byte *dst, byte *src, int x, int y, int width, int height) {

    int yOffset, offset;

    for (int yy = y; yy < height + y; yy++) {
        yOffset = yy * WIDTH;
        for (int xx = x; xx < width + x; xx++) {
            offset = yOffset + xx;
            dst[offset] = src[offset];
        }
    }
}

// Make sure the file is a Gif file
bool GifDecoder::parseGifHeader() {

    char buffer[10];

    readIntoBuffer(buffer, GIFHDRSIZE);
    if ((strncmp(buffer, GIFHDRTAGNORM,  GIFHDRSIZE) != 0) &&
        (strncmp(buffer, GIFHDRTAGNORM1, GIFHDRSIZE) != 0))  {
        return false;
    }
    else    {
        return true;
    }
}

// Parse the logical screen descriptor
void GifDecoder::parseLogicalScreenDescriptor() {

    lsdWidth = readWord();
    lsdHeight = readWord();
    lsdPackedField = readByte();
    lsdBackgroundIndex = readByte();
    lsdAspectRatio = readByte();

#if GIFDEBUG == 1 && DEBUG_SCREEN_DESCRIPTOR == 1
    Serial.print("lsdWidth: ");
    Serial.println(lsdWidth);
    Serial.print("lsdHeight: ");
    Serial.println(lsdHeight);
    Serial.print("lsdPackedField: ");
    Serial.println(lsdPackedField, HEX);
    Serial.print("lsdBackgroundIndex: ");
    Serial.println(lsdBackgroundIndex);
    Serial.print("lsdAspectRatio: ");
    Serial.println(lsdAspectRatio);
#endif
}

// Parse the global color table
void GifDecoder::parseGlobalColorTable() {

    // Does a global color table exist?
    if (lsdPackedField & COLORTBLFLAG) {

        // A GCT was present determine how many colors it contains
        colorCount = 1 << ((lsdPackedField & 7) + 1);

#if GIFDEBUG == 1 && DEBUG_GLOBAL_COLOR_TABLE == 1
        Serial.print("Global color table with ");
        Serial.print(colorCount);
        Serial.println(" colors present");
#endif
        // Read color values into the palette array
        int colorTableBytes = sizeof(rgb_24) * colorCount;
        readIntoBuffer(palette, colorTableBytes);
    }
}

// Parse plain text extension and dispose of it
void GifDecoder::parsePlainTextExtension() {

#if GIFDEBUG == 1 && DEBUG_PROCESSING_PLAIN_TEXT_EXT == 1
    Serial.println("\nProcessing Plain Text Extension");
#endif
    // Read plain text header length
    byte len = readByte();

    // Consume plain text header data
    readIntoBuffer(tempBuffer, len);

    // Consume the plain text data in blocks
    len = readByte();
    while (len != 0) {
        readIntoBuffer(tempBuffer, len);
        len = readByte();
    }
}

// Parse a graphic control extension
void GifDecoder::parseGraphicControlExtension() {

#if GIFDEBUG == 1 && DEBUG_PROCESSING_GRAPHIC_CONTROL_EXT == 1
    Serial.println("\nProcessing Graphic Control Extension");
#endif
    int len = readByte();	// Check length
    if (len != 4) {
        Serial.println("Bad graphic control extension");
    }

    int packedBits = readByte();
    frameDelay = readWord();
    transparentColorIndex = readByte();

    if ((packedBits & TRANSPARENTFLAG) == 0) {
        // Indicate no transparent index
        transparentColorIndex = NO_TRANSPARENT_INDEX;
    }
    disposalMethod = (packedBits >> 2) & 7;
    if (disposalMethod > 3) {
        disposalMethod = 0;
        Serial.println("Invalid disposal value");
    }

    readByte();	// Toss block end

#if GIFDEBUG == 1 && DEBUG_PROCESSING_GRAPHIC_CONTROL_EXT == 1
    Serial.print("PacketBits: ");
    Serial.println(packedBits, HEX);
    Serial.print("Frame delay: ");
    Serial.println(frameDelay);
    Serial.print("transparentColorIndex: ");
    Serial.println(transparentColorIndex);
    Serial.print("disposalMethod: ");
    Serial.println(disposalMethod);
#endif
}

// Parse application extension
void GifDecoder::parseApplicationExtension() {

    memset(tempBuffer, 0, sizeof(tempBuffer));

#if GIFDEBUG == 1 && DEBUG_PROCESSING_APP_EXT == 1
    Serial.println("\nProcessing Application Extension");
#endif

    // Read block length
    byte len = readByte();

    // Read app data
    readIntoBuffer(tempBuffer, len);

#if GIFDEBUG == 1 && DEBUG_PROCESSING_APP_EXT == 1
    // Conditionally display the application extension string
    if (strlen(tempBuffer) != 0) {
        Serial.print("Application Extension: ");
        Serial.println(tempBuffer);
    }
#endif

    // Consume any additional app data
    len = readByte();
    while (len != 0) {
        readIntoBuffer(tempBuffer, len);
        len = readByte();
    }
}

// Parse comment extension
void GifDecoder::parseCommentExtension() {

#if GIFDEBUG == 1 && DEBUG_PROCESSING_COMMENT_EXT == 1
    Serial.println("\nProcessing Comment Extension");
#endif

    // Read block length
    byte len = readByte();
    while (len != 0) {
        // Clear buffer
        memset(tempBuffer, 0, sizeof(tempBuffer));

        // Read len bytes into buffer
        readIntoBuffer(tempBuffer, len);

#if GIFDEBUG == 1 && DEBUG_PROCESSING_COMMENT_EXT == 1
        // Display the comment extension string
        if (strlen(tempBuffer) != 0) {
            Serial.print("Comment Extension: ");
            Serial.println(tempBuffer);
        }
#endif
        // Read the new block length
        len = readByte();
    }
}

// Parse file terminator
int GifDecoder::parseGIFFileTerminator() {

#if GIFDEBUG == 1 && DEBUG_PROCESSING_FILE_TERM == 1
    Serial.println("\nProcessing file terminator");
#endif

    byte b = readByte();
    if (b != 0x3B) {

#if GIFDEBUG == 1 && DEBUG_PROCESSING_FILE_TERM == 1
        Serial.print("Terminator byte: ");
        Serial.println(b, HEX);
#endif
        Serial.println("Bad GIF file format - Bad terminator");
        return ERROR_BADGIFFORMAT;
    }
    else	{
        return ERROR_NONE;
    }
}

// Parse table based image data
void GifDecoder::parseTableBasedImage() {

#if GIFDEBUG == 1 && DEBUG_PROCESSING_TBI_DESC_START == 1
    Serial.println("\nProcessing Table Based Image Descriptor");
#endif

#if GIFDEBUG == 1 && DEBUG_PARSING_DATA == 1
    Serial.println("File Position: ");
    Serial.println(file.position());
    Serial.println("File Size: ");
    Serial.println(file.size());
#endif

    // Parse image descriptor
    tbiImageX = readWord();
    tbiImageY = readWord();
    tbiWidth = readWord();
    tbiHeight = readWord();
    tbiPackedBits = readByte();

#if GIFDEBUG == 1
    Serial.print("tbiImageX: ");
    Serial.println(tbiImageX);
    Serial.print("tbiImageY: ");
    Serial.println(tbiImageY);
    Serial.print("tbiWidth: ");
    Serial.println(tbiWidth);
    Serial.print("tbiHeight: ");
    Serial.println(tbiHeight);
    Serial.print("PackedBits: ");
    Serial.println(tbiPackedBits, HEX);
#endif

    // Is this image interlaced ?
    tbiInterlaced = ((tbiPackedBits & INTERLACEFLAG) != 0);

#if GIFDEBUG == 1 && DEBUG_PROCESSING_TBI_DESC_INTERLACED == 1
    Serial.print("Image interlaced: ");
    Serial.println((tbiInterlaced != 0) ? "Yes" : "No");
#endif

    // Does this image have a local color table ?
    boolean localColorTable =  ((tbiPackedBits & COLORTBLFLAG) != 0);

    if (localColorTable) {
        int colorBits = ((tbiPackedBits & 7) + 1);
        colorCount = 1 << colorBits;

#if GIFDEBUG == 1 && DEBUG_PROCESSING_TBI_DESC_LOCAL_COLOR_TABLE == 1
        Serial.print("Local color table with ");
        Serial.print(colorCount);
        Serial.println(" colors present");
#endif
        // Read colors into palette
        int colorTableBytes = sizeof(rgb_24) * colorCount;
        readIntoBuffer(palette, colorTableBytes);
    }

    // One time initialization of imageData before first frame
    if (keyFrame) {
        if (transparentColorIndex == NO_TRANSPARENT_INDEX) {
            fillImageData(lsdBackgroundIndex);
        }
        else    {
            fillImageData(transparentColorIndex);
        }
        keyFrame = false;

        rectX = 0;
        rectY = 0;
        rectWidth = WIDTH;
        rectHeight = HEIGHT;
    }
    // Don't clear matrix screen for these disposal methods
    if ((prevDisposalMethod != DISPOSAL_NONE) && (prevDisposalMethod != DISPOSAL_LEAVE)) {
        if(screenClearCallback)
            (*screenClearCallback)();
    }

    // Process previous disposal method
    if (prevDisposalMethod == DISPOSAL_BACKGROUND) {
        // Fill portion of imageData with previous background color
        fillImageDataRect(prevBackgroundIndex, rectX, rectY, rectWidth, rectHeight);
    }
    else if (prevDisposalMethod == DISPOSAL_RESTORE) {
        copyImageDataRect(imageData, imageDataBU, rectX, rectY, rectWidth, rectHeight);
    }

    // Save disposal method for this frame for next time
    prevDisposalMethod = disposalMethod;

    if (disposalMethod != DISPOSAL_NONE) {
        // Save dimensions of this frame
        rectX = tbiImageX;
        rectY = tbiImageY;
        rectWidth = tbiWidth;
        rectHeight = tbiHeight;

        // limit rectangle to the bounds of WIDTH*HEIGHT
        if(rectX + rectWidth > WIDTH)
            rectWidth = WIDTH-rectX;
        if(rectY + rectHeight > HEIGHT)
            rectHeight = HEIGHT-rectY;
        if(rectX >= WIDTH || rectY >= HEIGHT) {
            rectX = rectY = rectWidth = rectHeight = 0;
        }

        if (disposalMethod == DISPOSAL_BACKGROUND) {
            if (transparentColorIndex != NO_TRANSPARENT_INDEX) {
                prevBackgroundIndex = transparentColorIndex;
            }
            else    {
                prevBackgroundIndex = lsdBackgroundIndex;
            }
        }
        else if (disposalMethod == DISPOSAL_RESTORE) {
            copyImageDataRect(imageDataBU, imageData, rectX, rectY, rectWidth, rectHeight);
        }
    }

    // Read the min LZW code size
    lzwCodeSize = readByte();

#if GIFDEBUG == 1 && DEBUG_PROCESSING_TBI_DESC_LZWCODESIZE == 1
    Serial.print("LzwCodeSize: ");
    Serial.println(lzwCodeSize);
    Serial.println("File Position Before: ");
    Serial.println(file.position());
#endif

    unsigned long filePositionBefore = file.position();

    // Gather the lzw image data
    // NOTE: the dataBlockSize byte is left in the data as the lzw decoder needs it
    int offset = 0;
    int dataBlockSize = readByte();
    while (dataBlockSize != 0) {
#if GIFDEBUG == 1 && DEBUG_PROCESSING_TBI_DESC_DATABLOCKSIZE == 1
    Serial.print("dataBlockSize: ");
    Serial.println(dataBlockSize);
#endif
        backUpStream(1);
        dataBlockSize++;
        file.seek(file.position() + dataBlockSize);

        offset += dataBlockSize;
        dataBlockSize = readByte();
    }

#if GIFDEBUG == 1 && DEBUG_PROCESSING_TBI_DESC_LZWIMAGEDATA_SIZE == 1
    Serial.print("total lzwImageData Size: ");
    Serial.println(offset);
    Serial.println("File Position Test: ");
    Serial.println(file.position());
#endif

    // this is the position where GIF decoding needs to pick up after decompressing frame
    unsigned long filePositionAfter = file.position();

    file.seek(filePositionBefore);

    // Process the animation frame for display

    // Initialize the LZW decoder for this frame
    lzw_decode_init(lzwCodeSize, readIntoBuffer);
    lzw_setTempBuffer((byte*)tempBuffer);

    // Make sure there is at least some delay between frames
    if (frameDelay < 1) {
        frameDelay = 1;
    }

    // Decompress LZW data and display the frame
    decompressAndDisplayFrame(filePositionAfter);

    // Graphic control extension is for a single frame
    transparentColorIndex = NO_TRANSPARENT_INDEX;
    disposalMethod = DISPOSAL_NONE;
}

// Parse gif data
int GifDecoder::parseData() {

#if GIFDEBUG == 1 && DEBUG_PARSING_DATA == 1
    Serial.println("\nParsing Data Block");
#endif

    boolean done = false;
    while (! done) {

#if GIFDEBUG == 1 && DEBUG_WAIT_FOR_KEY_PRESS == 1
    Serial.println("\nPress Key For Next");
    while(Serial.read() <= 0);
#endif

        // Determine what kind of data to process
        byte b = readByte();

        if (b == 0x2c) {
            // Parse table based image
#if GIFDEBUG == 1 && DEBUG_PARSING_DATA == 1
    Serial.println("\nParsing Table Based");
#endif
            parseTableBasedImage();

        }
        else if (b == 0x21) {
            // Parse extension
            b = readByte();

#if GIFDEBUG == 1 && DEBUG_PARSING_DATA == 1
    Serial.println("\nParsing Extension");
#endif

            // Determine which kind of extension to parse
            switch (b) {
            case 0x01:
                // Plain test extension
                parsePlainTextExtension();
                break;
            case 0xf9:
                // Graphic control extension
                parseGraphicControlExtension();
                break;
            case 0xfe:
                // Comment extension
                parseCommentExtension();
                break;
            case 0xff:
                // Application extension
                parseApplicationExtension();
                break;
            default:
                Serial.print("Unknown control extension: ");
                Serial.println(b, HEX);
                return ERROR_UNKNOWNCONTROLEXT;
            }
        }
        else    {
#if GIFDEBUG == 1 && DEBUG_PARSING_DATA == 1
    Serial.println("\nParsing Done");
#endif
            done = true;

            // Push unprocessed byte back into the stream for later processing
            backUpStream(1);
        }
    }
    return ERROR_NONE;
}

// Attempt to parse the gif file
int GifDecoder::processGIFFile(const char *pathname) {

    // Initialize variables
    keyFrame = true;
    prevDisposalMethod = DISPOSAL_NONE;
    transparentColorIndex = NO_TRANSPARENT_INDEX;

    Serial.print("Pathname: ");
    Serial.println(pathname);

    if(file)
        file.close();

    // Attempt to open the file for reading
    file = SD.open(pathname);
    if (!file) {
        Serial.println("Error opening GIF file");
        return ERROR_FILEOPEN;
    }
    // Validate the header
    if (! parseGifHeader()) {
        Serial.println("Not a GIF file");
        file.close();
        return ERROR_FILENOTGIF;
    }
    // If we get here we have a gif file to process

    // Parse the logical screen descriptor
    parseLogicalScreenDescriptor();

    // Parse the global color table
    parseGlobalColorTable();

    // Parse gif data
    int result = parseData();
    if (result != ERROR_NONE) {
        Serial.println("Error: ");
        Serial.println(result);
        Serial.println(" occurred during parsing of data");
        file.close();
        return result;
    }
    // Parse the gif file terminator
    result = parseGIFFileTerminator();
    file.close();

    Serial.println("Success");
    return result;
}

// Decompress LZW data and display animation frame
void GifDecoder::decompressAndDisplayFrame(unsigned long filePositionAfter) {

    // Each pixel of image is 8 bits and is an index into the palette

        // How the image is decoded depends upon whether it is interlaced or not
    // Decode the interlaced LZW data into the image buffer
    if (tbiInterlaced) {
        // Decode every 8th line starting at line 0
        for (int line = tbiImageY + 0; line < tbiHeight + tbiImageY; line += 8) {
            lzw_decode(imageData + (line * WIDTH) + tbiImageX, tbiWidth, min(imageData + (line * WIDTH) + WIDTH, imageData + sizeof(imageData)));
        }
        // Decode every 8th line starting at line 4
        for (int line = tbiImageY + 4; line < tbiHeight + tbiImageY; line += 8) {
            lzw_decode(imageData + (line * WIDTH) + tbiImageX, tbiWidth, min(imageData + (line * WIDTH) + WIDTH, imageData + sizeof(imageData)));
        }
        // Decode every 4th line starting at line 2
        for (int line = tbiImageY + 2; line < tbiHeight + tbiImageY; line += 4) {
            lzw_decode(imageData + (line * WIDTH) + tbiImageX, tbiWidth, min(imageData + (line * WIDTH) + WIDTH, imageData + sizeof(imageData)));
        }
        // Decode every 2nd line starting at line 1
        for (int line = tbiImageY + 1; line < tbiHeight + tbiImageY; line += 2) {
            lzw_decode(imageData + (line * WIDTH) + tbiImageX, tbiWidth, min(imageData + (line * WIDTH) + WIDTH, imageData + sizeof(imageData)));
        }
    }
    else    {
        // Decode the non interlaced LZW data into the image data buffer
        for (int line = tbiImageY; line < tbiHeight + tbiImageY; line++) {
            lzw_decode(imageData  + (line * WIDTH) + tbiImageX, tbiWidth, imageData + sizeof(imageData));
        }
    }

#if GIFDEBUG == 1 && DEBUG_DECOMPRESS_AND_DISPLAY == 1
    Serial.println("File Position After: ");
    Serial.println(file.position());
#endif

#if GIFDEBUG == 1 && DEBUG_WAIT_FOR_KEY_PRESS == 1
    Serial.println("\nPress Key For Next");
    while(Serial.read() <= 0);
#endif

    // LZW doesn't parse through all the data, manually set position
    file.seek(filePositionAfter);

    // Optional callback can be used to get drawing routines ready
    if(startDrawingCallback)
        (*startDrawingCallback)();

    // Image data is decompressed, now display portion of image affected by frame
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

            // Pixel not transparent so get color from palette and draw the pixel
            if(drawPixelCallback)
                (*drawPixelCallback)(x, y, palette[pixel].red, palette[pixel].green, palette[pixel].blue);
        }
    }
    // Make animation frame visible
    // swapBuffers() call can take up to 1/framerate seconds to return (it waits until a buffer copy is complete)
    // note the time before calling

    // wait until time to display next frame
    while(nextFrameTime_ms > millis());

    // calculate time to display next frame
    nextFrameTime_ms = millis() + (10 * frameDelay);
    if(updateScreenCallback)
        (*updateScreenCallback)();
}
