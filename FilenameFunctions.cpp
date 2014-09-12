/*
 * Animated GIFs Display Code for SmartMatrix and 32x32 RGB LED Panels
 *
 * This file contains code to enumerate and select animated GIF files by name
 *
 * Written by: Craig A. Lindley
 */

#include <SdFat.h>
extern SdFat sd;

SdFile file;

int numberOfFiles;

// Enumerate and possibly display the animated GIF filenames in GIFS directory
int enumerateGIFFiles(const char *directoryName, boolean displayFilenames) {

    numberOfFiles = 0;

    // Set the current working directory
    if (! sd.chdir(directoryName, true)) {
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

// Get the full path/filename of the GIF file with specified index
void getGIFFilenameByIndex(const char *directoryName, int index, char *pnBuffer) {

    char filename[13];

    // Make sure index is in range
    if ((index >= 0) && (index < numberOfFiles)) {

        // Set the current working directory
        if (! sd.chdir(directoryName, true)) {
            sd.errorHalt("Could not change to gifs directory");
        }

        // Make sure file is closed before starting
        file.close();

        // Rewind the directory to the beginning
        sd.vwd()->rewind();

        while ((file.openNext(sd.vwd(), O_READ)) && (index >= 0)) {

            file.getFilename(filename);

            // If filename is not marked as deleted, count it
            if ((filename[0] != '_') && (filename[0] != '~')) {
                index--;
            }
            file.close();
        }
        // Set the current working directory back to root
        if (! sd.chdir("/", true)) {
            sd.errorHalt("Could not change to root directory");
        }
        // Copy the directory name into the pathname buffer
        strcpy(pnBuffer, directoryName);

        // Append the filename to the pathname
        strcat(pnBuffer, filename);
    }
}

// Return a random animated gif path/filename from the specified directory
void chooseRandomGIFFilename(const char *directoryName, char *pnBuffer) {

    int index = random(numberOfFiles);
    getGIFFilenameByIndex(directoryName, index, pnBuffer);
}







