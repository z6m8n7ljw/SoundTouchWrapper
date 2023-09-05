////////////////////////////////////////////////////////////////////////////////
///
/// Classes for easy reading & writing of WAV sound files.
///
/// For big-endian CPU, define BIG_ENDIAN during compile-time to correctly
/// parse the WAV files with such processors.
///
/// Admittingly, more complete WAV reader routines may exist in public domain, but
/// the reason for 'yet another' one is that those generic WAV reader libraries are
/// exhaustingly large and cumbersome! Wanted to have something simpler here, i.e.
/// something that's not already larger than rest of the SoundTouch/SoundStretch program...
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

#ifndef WAVFILE_H
#define WAVFILE_H

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "STTypes.h"

#ifndef uint
typedef unsigned int uint;
#endif

// WAV audio file 'riff' section header
typedef struct {
    char riff_char[4];
    uint package_len;
    char wave[4];
} WavRiff;

// WAV audio file 'format' section header
typedef struct {
    char fmt[4];
    unsigned int format_len;
    unsigned short fixed;
    unsigned short channel_number;
    unsigned int sample_rate;
    unsigned int byte_rate;
    unsigned short byte_per_sample;
    unsigned short bits_per_sample;
} WavFormat;

// WAV audio file 'fact' section header
typedef struct {
    char fact_field[4];
    uint fact_len;
    uint fact_sample_len;
} WavFact;

// WAV audio file 'data' section header
typedef struct {
    char data_field[4];
    uint data_len;
} WavData;

// WAV audio file header
typedef struct {
    WavRiff riff;
    WavFormat format;
    WavFact fact;
    WavData data;
} WavHeader;

// Base class for processing WAV audio files.
typedef struct {
    char *convBuff;
    int convBuffSize;
} WavFileBase;

WavFileBase *WavFileBase_create();
void WavFileBase_destroy(WavFileBase *wavFileBase);
void *WavFileBase_getConvBuffer(WavFileBase *wavFileBase, int sizeBytes);

// Class for reading WAV audio files.
typedef struct {
    FILE *fptr;
    WavFileBase *base;
    long position;     // Position within the audio stream
    long dataRead;     // Counter of how many bytes of sample data have been read from the file
    WavHeader header;  // WAV header information
} WavInFile;

WavInFile *WavInFile_create(const char *fileName);
WavInFile *WavInFile_createWithFile(FILE *file);
void WavInFile_destroy(WavInFile *wavFile);
void WavInFile_rewind(WavInFile *wavFile);
int WavInFile_checkCharTags(const WavInFile *wavFile);
int WavInFile_read(WavInFile *inFile, unsigned char *buffer, int maxElems);
int WavInFile_readInt(WavInFile *inFile, short *buffer, int maxElems);
int WavInFile_readFloat(WavInFile *wavFile, float *buffer, int maxElems);
int WavInFile_eof(const WavInFile *wavInFile);
int WavInFile_readRIFFBlock(WavInFile *wavInFile);
int WavInFile_readHeaderBlock(FILE *fptr, WavHeader *header);
int WavInFile_readWavHeaders(WavInFile *wavInFile);
uint WavInFile_getNumChannels(const WavInFile *wavInFile);
uint WavInFile_getNumBits(const WavInFile *wavInFile);
uint WavInFile_getBytesPerSample(const WavInFile *wavInFile);
uint WavInFile_getSampleRate(const WavInFile *wavInFile);
uint WavInFile_getDataSizeInBytes(const WavInFile *wavInFile);
uint WavInFile_getNumSamples(const WavInFile *wavInFile);
uint WavInFile_getLengthMS(const WavInFile *wavInFile);

// Class for writing WAV audio files.
typedef struct {
    FILE *fptr;
    WavFileBase *base;
    int bytesWritten;  // Counter of how many bytes have been written to the file so far
    WavHeader header;  // WAV file header data
} WavOutFile;

WavOutFile *WavOutFile_create(const char *fileName, int sampleRate, int bits, int channels);
WavOutFile *WavOutFile_createWithFile(FILE *file, int sampleRate, int bits, int channels);
void WavOutFile_destroy(WavOutFile *wavOutFile);
void WavOutFile_fillInHeader(WavOutFile *wavOutFile, unsigned int sampleRate, unsigned int bits, unsigned int channels);
void WavOutFile_finishHeader(WavOutFile *wavOutFile);
void WavOutFile_writeHeader(WavOutFile *wavOutFile);
void WavOutFile_write(WavOutFile *outFile, const unsigned char *buffer, int numElems);
void WavOutFile_writeInt(WavOutFile *outFile, const short *buffer, int numElems);
void WavOutFile_writeFloat(WavOutFile *wavOutFile, const float *buffer, int numElems);

#endif
