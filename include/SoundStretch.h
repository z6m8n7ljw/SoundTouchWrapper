#ifndef SOUNDSTRETCH_H
#define SOUNDSTRETCH_H

#include <stdio.h>
#include <string.h>

#include "RunParameters.h"
#include "STTypes.h"
#include "SoundTouch_Wrapper.h"
#include "WavFile.h"

void openFiles(WavInFile **inFile, WavOutFile **outFile, const RunParameters *params);
void setup(void *pSoundTouch, const WavInFile *inFile, const RunParameters *params);
void process(void *pSoundTouch, WavInFile *inFile, WavOutFile *outFile);

#endif  // SOUNDSTRETCH_H
