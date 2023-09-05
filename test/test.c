#include <stdio.h>

#include "RunParameters.h"
#include "SoundStretch.h"
#include "WavFile.h"

int main(const int nParams, const char *const paramStr[]) {
    WavInFile *inFile;
    WavOutFile *outFile;
    RunParameters *params;
    void *soundTouch;

    // Parse command line parameters
    params = (RunParameters *)malloc(sizeof(RunParameters));
    InitRunParameters(params, nParams, paramStr);

    // Open input & output files
    openFiles(&inFile, &outFile, params);

    soundTouch = SoundTouch_init();
    // Setup the 'SoundTouch' object for processing the sound
    setup(soundTouch, inFile, params);

    // clock_t cs = clock();    // for benchmarking processing duration
    // Process the sound
    process(soundTouch, inFile, outFile);
    // clock_t ce = clock();    // for benchmarking processing duration
    // printf("duration: %lf\n", (double)(ce-cs)/CLOCKS_PER_SEC);

    // Close WAV file handles & dispose of the objects
    WavInFile_destroy(inFile);
    WavOutFile_destroy(outFile);
    free(params);
    params = NULL;

    SoundTouch_free(soundTouch);

    fprintf(stderr, "Done!\n\n");

    return 0;
}
