/*
 * Animated GIFs Display Code for 32x32 RGB LED Matrix
 *
 * This file contains code to enumerate and select animated GIF files by name
 *
 * Written by: Craig A. Lindley
 * Version: 1.1
 * Last Update: 06/18/2014
 */

#include <SdFat.h>
extern SdFat sd;

SdFile file;

int numberOfFiles;

// Enumerate and possibly display the animated GIF filenames in GIFS directory
int enumerateGIFFiles(boolean displayFilenames) {

    numberOfFiles = 0;

    // Set the current working directory
    if (! sd.chdir("GIFS", true)) {
        sd.errorHalt("Could not change to gifs directory");
    }
    sd.vwd()->rewind();

    char fn[13];
    while (file.openNext(sd.vwd(), O_READ)) {
        file.getFilename(fn);
        // If filename not deleted, count it
        if (fn[0] != '_') {
            numberOfFiles++;
            if (displayFilenames) {
                Serial.println(fn);
                delay(20);
            }
        }
        file.close();
    }
    // Set the current working directory
    if (! sd.chdir("/", true)) {
        sd.errorHalt("Could not change to root directory");
    }
    return numberOfFiles;
}

// Get the filename of the GIF filename with specified index
void getGIFFilenameByIndex(int index, char *fnBuffer) {

    // Make sure index is in range
    if ((index >= 0) && (index < numberOfFiles)) {

        // Set the current working directory
        if (! sd.chdir("GIFS", true)) {
            sd.errorHalt("Could not change to gifs directory");
        }
        file.close();
        sd.vwd()->rewind();

        while ((file.openNext(sd.vwd(), O_READ)) && (index >= 0)) {

            file.getFilename(fnBuffer);
            // If filename not deleted, count it
            if (fnBuffer[0] != '_') {
                index--;
            }
            file.close();
        }
        // Set the current working directory
        if (! sd.chdir("/", true)) {
            sd.errorHalt("Could not change to root directory");
        }
    }
}

// Return a random animated gif filename
void chooseRandomGIFFilename(char *fnBuffer) {

    int index = random(numberOfFiles);    
    getGIFFilenameByIndex(index, fnBuffer);
}






