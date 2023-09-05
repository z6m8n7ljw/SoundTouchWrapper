////////////////////////////////////////////////////////////////////////////////
///
/// SoundStretch main routine.
///
/// Author        : Copyright (c) Olli Parviainen
/// Author e-mail : oparviai 'at' iki.fi
/// SoundTouch WWW: http://www.surina.net/soundtouch
///
////////////////////////////////////////////////////////////////////////////////
//
// License :
//
//  SoundTouch audio processing library
//  Copyright (c) Olli Parviainen
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
////////////////////////////////////////////////////////////////////////////////

#include "SoundStretch.h"

// Processing chunk size (size chosen to be divisible by 2, 4, 6, 8, 10, 12, 14, 16 channels ...)
#define BUFF_SIZE 2048

#if _WIN32
#include <fcntl.h>
#include <io.h>

// Macro for Win32 standard input/output stream support: Sets a file stream into binary mode
#define SET_STREAM_TO_BIN_MODE(f) (_setmode(_fileno(f), _O_BINARY))
#else
// Not needed for GNU environment...
#define SET_STREAM_TO_BIN_MODE(f) \
    {}
#endif

void openFiles(WavInFile **inFile, WavOutFile **outFile, const RunParameters *params) {
    int bits, samplerate, channels;

    if (strcmp(params->inFileName, "stdin") == 0) {
        // used 'stdin' as input file
        SET_STREAM_TO_BIN_MODE(stdin);
        *inFile = WavInFile_createWithFile(stdin);
    } else {
        // open input file...
        *inFile = WavInFile_create(params->inFileName);
    }

    // ... open output file with same sound parameters
    bits = (int)WavInFile_getNumBits(*inFile);
    samplerate = (int)WavInFile_getSampleRate(*inFile);
    channels = (int)WavInFile_getNumChannels(*inFile);

    if (params->outFileName) {
        if (strcmp(params->outFileName, "stdout") == 0) {
            SET_STREAM_TO_BIN_MODE(stdout);
            *outFile = WavOutFile_createWithFile(stdout, samplerate, bits, channels);
        } else {
            *outFile = WavOutFile_create(params->outFileName, samplerate, bits, channels);
        }
    } else {
        *outFile = NULL;
    }
}

// Sets the 'SoundTouch' object up according to input file sound format &
// command line parameters
void setup(void *pSoundTouch, const WavInFile *inFile, const RunParameters *params) {
    int sampleRate;
    int channels;

    sampleRate = (int)WavInFile_getSampleRate(inFile);
    channels = (int)WavInFile_getNumChannels(inFile);
    SoundTouch_setSampleRate(pSoundTouch, sampleRate);
    SoundTouch_setChannels(pSoundTouch, channels);

    SoundTouch_setPitchSemiTones(pSoundTouch, params->pitchDelta);

    // print processing information
    if (params->outFileName) {
#ifdef SOUNDTOUCH_INTEGER_SAMPLES
        fprintf(stderr, "Uses 16bit integer sample type.\n");
#else
#ifndef SOUNDTOUCH_FLOAT_SAMPLES
#error "Sampletype not defined"
#endif
        fprintf(stderr, "Uses 32bit floating point sample type.\n");
#endif
        // print processing information only if outFileName given i.e. some processing will happen
        fprintf(stderr, "Tempo change = %+g %%\n", params->tempoDelta);
        fprintf(stderr, "Pitch change = %+g semitones\n", params->pitchDelta);
        fprintf(stderr, "Rate change  = %+g %%\n", params->rateDelta);
    } else {
        // outFileName not given
        fprintf(stderr, "Warning: output file name missing, won't output anything.\n");
    }

    fflush(stderr);
}

// Processes the sound
void process(void *pSoundTouch, WavInFile *inFile, WavOutFile *outFile) {
    int nSamples;
    int nChannels;
    int buffSizeSamples;
    SAMPLETYPE sampleBuffer[BUFF_SIZE];

    if ((inFile == NULL) || (outFile == NULL)) return;  // nothing to do.

    nChannels = (int)WavInFile_getNumChannels(inFile);
    assert(nChannels > 0);
    buffSizeSamples = BUFF_SIZE / nChannels;

    // Process samples read from the input file
    while (WavInFile_eof(inFile) == 0) {
        int num;

        // Read a chunk of samples from the input file
#ifdef SOUNDTOUCH_INTEGER_SAMPLES
        num = WavInFile_readInt(inFile, sampleBuffer, BUFF_SIZE);
#else
        num = WavInFile_readFloat(inFile, sampleBuffer, BUFF_SIZE);
#endif

        nSamples = num / (int)WavInFile_getNumChannels(inFile);

        // Feed the samples into SoundTouch processor
        SoundTouch_putSamples(pSoundTouch, sampleBuffer, nSamples);

        // Read ready samples from SoundTouch processor & write them output file.
        // NOTES:
        // - 'receiveSamples' doesn't necessarily return any samples at all
        //   during some rounds!
        // - On the other hand, during some round 'receiveSamples' may have more
        //   ready samples than would fit into 'sampleBuffer', and for this reason
        //   the 'receiveSamples' call is iterated for as many times as it
        //   outputs samples.
        do {
            nSamples = SoundTouch_receiveSamples(pSoundTouch, sampleBuffer, buffSizeSamples);
#ifdef SOUNDTOUCH_INTEGER_SAMPLES
            WavOutFile_writeInt(outFile, sampleBuffer, nSamples * nChannels);
#else
            WavOutFile_writeFloat(outFile, sampleBuffer, nSamples * nChannels);
#endif

        } while (nSamples != 0);
    }

    // Now the input file is processed, yet 'flush' few last samples that are
    // hiding in the SoundTouch's internal processing pipeline.
    SoundTouch_flush(pSoundTouch);
    do {
        nSamples = SoundTouch_receiveSamples(pSoundTouch, sampleBuffer, buffSizeSamples);
#ifdef SOUNDTOUCH_INTEGER_SAMPLES
        WavOutFile_writeInt(outFile, sampleBuffer, nSamples * nChannels);
#else
        WavOutFile_writeFloat(outFile, sampleBuffer, nSamples * nChannels);
#endif
    } while (nSamples != 0);
}