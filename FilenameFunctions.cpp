/*
 * Animated GIFs Display Code for SmartMatrix and 32x32 RGB LED Panels
 *
 * This file contains code to enumerate and select animated GIF files by name
 *
 * Written by: Craig A. Lindley
 */

// http://esp8266.github.io/Arduino/versions/2.3.0/doc/filesystem.html#uploading-files-to-file-system
// esp8266com/esp8266/libraries/SD/src/File.cpp
#include <FS.h>

File file;

int numberOfFiles;

bool fileSeekCallback(unsigned long position) {
    return file.seek(position);
}

unsigned long filePositionCallback(void) {
    return file.position();
}

int fileReadCallback(void) {
    yield();
    return file.read();
}

int fileReadBlockCallback(void * buffer, int numberOfBytes) {
    return file.read((uint8_t*)buffer, numberOfBytes);
}

bool isAnimationFile(String filenameString) {

#if defined(ESP32) || defined(ESP8266)
    // ESP32 filename includes the full path, so need to remove the path before looking at the filename
    int pathindex = filenameString.lastIndexOf("/");
    if(pathindex >= 0)
        filenameString.remove(0, pathindex + 1);
#endif

    if ((filenameString[0] == '_') || (filenameString[0] == '~') || (filenameString[0] == '.')) {
        Serial.println(" ignoring: leading _/~/. character");
        return false;
    }

    filenameString.toUpperCase();
    if (filenameString.endsWith(".GIF") != 1) {
        Serial.println(" ignoring: doesn't end of .GIF");
        return false;
    }

    return true;
}

// Enumerate and possibly display the animated GIF filenames in GIFS directory
int enumerateGIFFiles(const char *directoryName, boolean displayFilenames) {

    numberOfFiles = 0;

    Dir directory = SPIFFS.openDir(directoryName);

    Serial.print("Enumerate files in dir ");
    Serial.println(directoryName);
    while (directory.next()) {
        String filename = directory.fileName();
        if (isAnimationFile(filename)) {
            numberOfFiles++;
            if (displayFilenames) Serial.println(filename);
        }
    }
    return numberOfFiles;
}

// Get the full path/filename of the GIF file with specified index
void getGIFFilenameByIndex(const char *directoryName, int index, char *pnBuffer) {

    // Make sure index is in range
    if ((index < 0) || (index >= numberOfFiles))
        return;

    Dir directory = SPIFFS.openDir(directoryName);
    
    while (directory.next() && index >= 0) {
        String filename = directory.fileName();
        if (isAnimationFile(filename)) {
            index--;
            filename.toCharArray(pnBuffer, 29);
        }
    }
    Serial.print("Selected file ");
    Serial.println(pnBuffer);

    file.close();
}

int openGifFilenameByIndex(const char *directoryName, int index) {
    char pathname[30];

    getGIFFilenameByIndex(directoryName, index, pathname);
    
    // Pathname: /gifs/32anim_balls.gif
    //Serial.print("Pathname: ");
    //Serial.println(pathname);

    if(file)
        file.close();

    // Attempt to open the file for reading
    file = SPIFFS.open(pathname, "r");
    if (!file) {
        Serial.println("Error opening GIF file");
        return -1;
    }

    return 0;
}


// Return a random animated gif path/filename from the specified directory
void chooseRandomGIFFilename(const char *directoryName, char *pnBuffer) {

    int index = random(numberOfFiles);
    getGIFFilenameByIndex(directoryName, index, pnBuffer);
}
