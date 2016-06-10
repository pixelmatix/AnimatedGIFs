#ifndef FILENAME_FUNCTIONS_H
#define FILENAME_FUNCTIONS_H

int enumerateGIFFiles(const char *directoryName, boolean displayFilenames);
void getGIFFilenameByIndex(const char *directoryName, int index, char *pnBuffer);
void chooseRandomGIFFilename(const char *directoryName, char *pnBuffer);

#endif
