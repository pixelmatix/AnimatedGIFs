/*
 * Animated GIFs Display Code for 32x32 RGB LED Matrix
 *
 * This file contains code to parse animated GIF files
 *
 * Written by: Craig A. Lindley
 * Version: 1.1
 * Last Update: 06/18/2014
 */

// NOTE: setting this to 1 will cause parsing to fail at the present time
#define DEBUG 0

#include <SdFat.h>
extern SdFat sd;
extern SdFile file;

// Defined in LZWFunctions.cpp
extern void lzw_decode_init (int csize, byte *buf);
extern void decompressAndDisplayFrame();

// Error codes
#define ERROR_NONE		    0
#define ERROR_FILEOPEN		   -1
#define ERROR_FILENOTGIF	   -2
#define ERROR_BADGIFFORMAT         -3
#define ERROR_UNKNOWNCONTROLEXT	   -4

#define GIFHDRTAGNORM   "GIF87a"  // tag in valid GIF file
#define GIFHDRTAGNORM1  "GIF89a"  // tag in valid GIF file
#define GIFHDRSIZE 6

// Global GIF specific definitions
#define GLOBALCOLORTBL     0x80
#define LOCALCOLORTBL      0x80
#define LOCALINTERLACEFLAG 0x40

#define NO_TRANSPARENT_INDEX 8192

// RGB data structure
typedef struct {
    byte Red;
    byte Green;
    byte Blue;
} 
RGB;

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

int frameCount;
int frameDelay;
int transparentColorIndex;
int disposalMethod;
int lzwCodeSize;

int colorCount;
RGB palette[256];

byte lzwImageData[1024];
char tempBuffer[260];

// Backup the read stream by n bytes
void backUpStream(int n) {
    file.seekCur(-n);
}

// Read a file byte
int readByte() {

    int b = file.read();
    if (b == -1) {
        Serial.println("Read error or EOF occurred");
    }
    return b;
}

// Read a file word
int readWord() {

    int b0 = readByte();
    int b1 = readByte();    
    return (b1 << 8) | b0;
}

int readIntoBuffer(void *buffer, int numberOfBytes) {

    int result = file.read(buffer, numberOfBytes);
    if (result == -1) {
        Serial.println("Read error or EOF occurred");
    }
    return result;
}

// Make sure the file is a Gif file
boolean parseGifHeader() {

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
void parseLogicalScreenDescriptor() {

    lsdWidth = readWord();
    lsdHeight = readWord();
    lsdPackedField = readByte();
    lsdBackgroundIndex = readByte();
    lsdAspectRatio = readByte();

#if DEBUG == 1
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
void parseGlobalColorTable() {

    // Does a global color table exist?
    if (lsdPackedField & GLOBALCOLORTBL) {

        // A GCT was present determine how many colors it contains
        colorCount = 1 << ((lsdPackedField & 7) + 1);

#if DEBUG == 1
        Serial.print("Global color table with ");
        Serial.print(colorCount);
        Serial.println(" colors present");
#endif
        // Read color values into the palette array
        int colorTableBytes = sizeof(RGB) * colorCount;
        readIntoBuffer(palette, colorTableBytes);

#if DEBUG == 1
        // Show the palette values
        for (int i = 0; i < colorCount; i++) {
            delay(10);
            Serial.print(palette[i].Red);
            Serial.print(" ");
            Serial.print(palette[i].Green);
            Serial.print(" ");
            Serial.print(palette[i].Blue);
            Serial.println();
        }
        Serial.println();
#endif
    }
}

// Parse plain text extension and dispose of it
void parsePlainTextExtension() {

#if DEBUG == 1
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
void parseGraphicControlExtension() {

#if DEBUG == 1
    Serial.println("\nProcessing Graphic Control Extension");
#endif
    readByte();	// Toss length

    int packedBits = readByte();
    frameDelay = readWord();
    transparentColorIndex = readByte();

    if ((packedBits & 0x01) == 0) {
        // Indicate no transparent index
        transparentColorIndex = NO_TRANSPARENT_INDEX;
    }
    disposalMethod = (packedBits >> 2) & 7;

    readByte();	// Toss block end

#if DEBUG == 1
    Serial.print("Frame delay: ");
    Serial.println(frameDelay);
    Serial.print("transparentColorIndex: ");
    Serial.println(transparentColorIndex);
    Serial.print("disposalMethod: ");
    Serial.println(disposalMethod);
#endif
}

// Parse application extension
void parseApplicationExtension() {

    memset(tempBuffer, 0, sizeof(tempBuffer));

#if DEBUG == 1
    Serial.println("\nProcessing Application Extension");
#endif

    // Read block length
    byte len = readByte();

    // Read app data
    readIntoBuffer(tempBuffer, len);

#if DEBUG == 1
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
void parseCommentExtension() {

#if DEBUG == 1
    Serial.println("\nProcessing Comment Extension");
#endif

    // Read block length
    byte len = readByte();
    while (len != 0) {
        // Clear buffer
        memset(tempBuffer, 0, sizeof(tempBuffer));

        // Read len bytes into buffer
        readIntoBuffer(tempBuffer, len);		

#if DEBUG == 1
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
int parseGIFFileTerminator() {

#if DEBUG == 1
    Serial.println("\nProcessing file terminator");
#endif

    byte b = readByte();	
    if (b != 0x3B) {

#if DEBUG == 1
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
void parseTableBasedImage() {

#if DEBUG == 1
    Serial.println("\nProcessing Table Based Image Descriptor");
#endif

    frameCount++;

    // Parse image descriptor
    tbiImageX = readWord();
    tbiImageY = readWord();
    tbiWidth = readWord();
    tbiHeight = readWord();
    tbiPackedBits = readByte();

#if DEBUG == 1
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
    tbiInterlaced = ((tbiPackedBits & LOCALINTERLACEFLAG) != 0);

#if DEBUG == 1
    Serial.print("Image interlaced: ");
    Serial.println((tbiInterlaced != 0) ? "Yes" : "No");     
#endif

    // Does this image have a local color table ?
    boolean localColorTable =  ((tbiPackedBits & LOCALCOLORTBL) != 0);

    if (localColorTable) {
        int colorBits = ((tbiPackedBits & 7) + 1);
        colorCount = 1 << colorBits;

#if DEBUG == 1
        Serial.print("Local color table with ");
        Serial.print(colorCount);
        Serial.println(" colors present");
#endif
        // Read colors into palette
        int colorTableBytes = sizeof(RGB) * colorCount;
        readIntoBuffer(palette, colorTableBytes);
    }

    // Read the min LZW code size
    lzwCodeSize = readByte();

#if DEBUG == 1
    Serial.print("LzwCodeSize: ");
    Serial.println(lzwCodeSize);
#endif

    // Gather the lzw image data
    // NOTE: the dataBlockSize byte is left in the data as the lzw decoder needs it
    int offset = 0;
    int dataBlockSize = readByte();
    while (dataBlockSize != 0) {
        backUpStream(1);
        dataBlockSize++;
        readIntoBuffer(lzwImageData + offset, dataBlockSize);
        offset += dataBlockSize;
        dataBlockSize = readByte();
    }
    // Process the animation frame for display

    // Initialize the LZW decoder for this frame
    lzw_decode_init(lzwCodeSize, lzwImageData);

    // Decompress LZW data and display the frame
    decompressAndDisplayFrame();

    // Delay for specified duration between frames
    delay(frameDelay * 10);
}

// Parse gif data
int parseData() {

#if DEBUG == 1
    Serial.println("\nParsing Data Block");
#endif

    boolean done = false;	
    while (! done) {

        // Determine what kind of data to process
        byte b = readByte();

        if (b == 0x2c) {
            // Parse table based image
            parseTableBasedImage();

        }	
        else if (b == 0x21) {
            // Parse extension
            b = readByte();

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
        else	{
            done = true;

            // Push unprocessed byte back into the stream for later processing
            backUpStream(1);
        }
    }
    return ERROR_NONE;
}

// Attempt to parse the gif file
int processGIFFile(char *pathname) {

    char pn[30];

    // Initialize variables
    frameCount = 0;

    memset(pn, 0, sizeof(pn));
    strcpy(pn, pathname);

    Serial.print("Pathname: ");
    Serial.println(pn);
    
    file.close();

    // Attempt to open the file for reading
    if (! file.open(pn)) {
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

#if DEBUG == 1
    Serial.print("Frame count: ");
    Serial.println(frameCount);
#endif
    Serial.println("Success");
    return result;
}















