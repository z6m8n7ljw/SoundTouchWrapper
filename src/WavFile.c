
#include "WavFile.h"

static const char riffStr[] = "RIFF";
static const char waveStr[] = "WAVE";
static const char fmtStr[] = "fmt ";
static const char factStr[] = "fact";
static const char dataStr[] = "data";

//////////////////////////////////////////////////////////////////////////////
//
// Helper functions for swapping byte order to correctly read/write WAV files
// with big-endian CPU's: Define compile-time definition _BIG_ENDIAN_ to
// turn-on the conversion if it appears necessary.
//
// For example, Intel x86 is little-endian and doesn't require conversion,
// while PowerPC of Mac's and many other RISC cpu's are big-endian.

#ifdef BYTE_ORDER
// In gcc compiler detect the byte order automatically
#if BYTE_ORDER == BIG_ENDIAN
// big-endian platform.
#define _BIG_ENDIAN_
#endif
#endif

#ifdef _BIG_ENDIAN_
// big-endian CPU, swap bytes in 16 & 32 bit words

// helper-function to swap byte-order of 32bit integer
static int _swap32(int *dwData) {
    *dwData = ((*dwData >> 24) & 0x000000FF) | ((*dwData >> 8) & 0x0000FF00) | ((*dwData << 8) & 0x00FF0000) |
              ((*dwData << 24) & 0xFF000000);
    return *dwData;
}

// helper-function to swap byte-order of 16bit integer
static short _swap16(short *wData) {
    *wData = ((*wData >> 8) & 0x00FF) | ((*wData << 8) & 0xFF00);
    return *wData;
}

// helper-function to swap byte-order of buffer of 16bit integers
static void _swap16Buffer(short *pData, int numWords) {
    int i;

    for (i = 0; i < numWords; i++) {
        pData[i] = _swap16(&pData[i]);
    }
}

#else  // BIG_ENDIAN
// little-endian CPU, WAV file is ok as such

// dummy helper-function
static int _swap32(int *dwData) {
    // do nothing
    return *dwData;
}

// dummy helper-function
static short _swap16(short *wData) {
    // do nothing
    return *wData;
}

// dummy helper-function
static void _swap16Buffer(short *pData, int numBytes) {
    // do nothing
}

#endif  // BIG_ENDIAN

//////////////////////////////////////////////////////////////////////////////
//
// Class WavFileBase
//

WavFileBase *WavFileBase_create() {
    WavFileBase *fileBase = (WavFileBase *)malloc(sizeof(WavFileBase));
    if (fileBase != NULL) {
        fileBase->convBuff = NULL;
        fileBase->convBuffSize = 0;
    }
    return fileBase;
}

void WavFileBase_destroy(WavFileBase *fileBase) {
    if (fileBase != NULL) {
        free(fileBase->convBuff);
        free(fileBase);
    }
}

void *WavFileBase_getConvBuffer(WavFileBase *fileBase, int sizeBytes) {
    if (fileBase->convBuffSize < sizeBytes) {
        free(fileBase->convBuff);
        fileBase->convBuffSize = (sizeBytes + 15) & -8;  // round up to following 8-byte boundary
        fileBase->convBuff = (char *)malloc(fileBase->convBuffSize);
    }
    return fileBase->convBuff;
}

//////////////////////////////////////////////////////////////////////////////
//
// Class WavInFile
//

static void init(WavInFile *inFile) {
    int hdrsOk;

    // assume file stream is already open
    assert(inFile->fptr);

    // Read the file headers
    hdrsOk = WavInFile_readWavHeaders(inFile);
    if (hdrsOk != 0) {
        // Something didn't match in the wav file headers
        ST_THROW_RT_ERROR("Input file is corrupt or not a WAV file");
    }

    // sanity check for format parameters
    if ((inFile->header.format.channel_number < 1) || (inFile->header.format.channel_number > 9) ||
        (inFile->header.format.sample_rate < 4000) || (inFile->header.format.sample_rate > 192000) ||
        (inFile->header.format.byte_per_sample < 1) || (inFile->header.format.byte_per_sample > 320) ||
        (inFile->header.format.bits_per_sample < 8) || (inFile->header.format.bits_per_sample > 32)) {
        // Something didn't match in the wav file headers
        ST_THROW_RT_ERROR("Error: Illegal wav file header format parameters.");
    }

    inFile->dataRead = 0;
}

WavInFile *WavInFile_create(const char *fileName) {
    WavInFile *inFile = (WavInFile *)malloc(sizeof(WavInFile));
    if (inFile == NULL) {
        ST_THROW_RT_ERROR("Error: Unable to allocate memory for WavInFile");
    }

    // Try to open the file for reading
    inFile->fptr = fopen(fileName, "rb");
    if (inFile->fptr == NULL) {
        // didn't succeed
        char msg[256];
        snprintf(msg, sizeof(msg), "Error : Unable to open file \"%s\" for reading.", fileName);
        ST_THROW_RT_ERROR(msg);
    }
    inFile->base = WavFileBase_create();

    init(inFile);

    return inFile;
}

WavInFile *WavInFile_createWithFile(FILE *file) {
    WavInFile *inFile = (WavInFile *)malloc(sizeof(WavInFile));
    if (inFile == NULL) {
        ST_THROW_RT_ERROR("Error: Unable to allocate memory for WavInFile");
    }

    // Try to open the file for reading
    inFile->fptr = file;
    if (!file) {
        // didn't succeed
        ST_THROW_RT_ERROR("Error : Unable to access input stream for reading");
    }
    inFile->base = WavFileBase_create();

    init(inFile);

    return inFile;
}

void WavInFile_destroy(WavInFile *inFile) {
    if (inFile) {
        if (inFile->fptr) {
            fclose(inFile->fptr);
            inFile->fptr = NULL;
        }
        WavFileBase_destroy(inFile->base);
        free(inFile);
    }
}

void WavInFile_rewind(WavInFile *inFile) {
    int hdrsOk;

    fseek(inFile->fptr, 0, SEEK_SET);
    hdrsOk = WavInFile_readWavHeaders(inFile);
    assert(hdrsOk == 0);
    inFile->dataRead = 0;
}

int WavInFile_checkCharTags(const WavInFile *inFile) {
    // header.format.fmt should equal to 'fmt '
    if (memcmp(fmtStr, inFile->header.format.fmt, 4) != 0) return -1;
    // header.data.data_field should equal to 'data'
    if (memcmp(dataStr, inFile->header.data.data_field, 4) != 0) return -1;

    return 0;
}

int WavInFile_read(WavInFile *inFile, float *buffer, int maxElems) {
    unsigned int afterDataRead;
    int numBytes;
    int numElems;
    int bytesPerSample;

    assert(buffer);

    bytesPerSample = inFile->header.format.bits_per_sample / 8;
    if ((bytesPerSample < 1) || (bytesPerSample > 4)) {
        char errorMsg[256];
        snprintf(errorMsg, sizeof(errorMsg),
                 "\nOnly 8/16/24/32 bit sample WAV files supported. Can't open WAV file with %d bit sample format. ",
                 inFile->header.format.bits_per_sample);
        ST_THROW_RT_ERROR(errorMsg);
    }

    numBytes = maxElems * bytesPerSample;
    afterDataRead = inFile->dataRead + numBytes;
    if (afterDataRead > inFile->header.data.data_len) {
        // Don't read more samples than are marked available in header
        numBytes = (int)inFile->header.data.data_len - (int)inFile->dataRead;
        assert(numBytes >= 0);
    }

    // read raw data into temporary buffer
    char *temp = (char *)WavFileBase_getConvBuffer(inFile->base, numBytes);
    numBytes = (int)fread(temp, 1, numBytes, inFile->fptr);
    inFile->dataRead += numBytes;

    numElems = numBytes / bytesPerSample;

    // swap byte ordert & convert to float, depending on sample format
    switch (bytesPerSample) {
        case 1: {
            unsigned char *temp2 = (unsigned char *)temp;
            double conv = 1.0 / 128.0;
            for (int i = 0; i < numElems; i++) {
                buffer[i] = (float)(temp2[i] * conv - 1.0);
            }
            break;
        }

        case 2: {
            short *temp2 = (short *)temp;
            double conv = 1.0 / 32768.0;
            for (int i = 0; i < numElems; i++) {
                short value = temp2[i];
                buffer[i] = (float)(_swap16(&value) * conv);
            }
            break;
        }

        case 3: {
            char *temp2 = (char *)temp;
            double conv = 1.0 / 8388608.0;
            for (int i = 0; i < numElems; i++) {
                int value = *((int *)temp2);
                value = _swap32(&value) & 0x00ffffff;            // take 24 bits
                value |= (value & 0x00800000) ? 0xff000000 : 0;  // extend minus sign bits
                buffer[i] = (float)(value * conv);
                temp2 += 3;
            }
            break;
        }

        case 4: {
            int *temp2 = (int *)temp;
            double conv = 1.0 / 2147483648.0;
            assert(sizeof(int) == 4);
            for (int i = 0; i < numElems; i++) {
                int value = temp2[i];
                buffer[i] = (float)(_swap32(&value) * conv);
            }
            break;
        }
    }

    return numElems;
}

int WavInFile_eof(const WavInFile *wavInFile) {
    // return true if all data has been read or file eof has reached
    return (wavInFile->dataRead == wavInFile->header.data.data_len || feof(wavInFile->fptr));
}

// test if character code is between a white space ' ' and little 'z'
static int isAlpha(char c) { return (c >= ' ' && c <= 'z') ? 1 : 0; }

// test if all characters are between a white space ' ' and little 'z'
static int isAlphaStr(const char *str) {
    char c = str[0];

    while (c) {
        if (isAlpha(c) == 0) return 0;
        str++;
        c = str[0];
    }

    return 1;
}

int WavInFile_readRIFFBlock(WavInFile *wavInFile) {
    if (fread(&(wavInFile->header.riff), sizeof(WavRiff), 1, wavInFile->fptr) != 1) return -1;

    // swap 32bit data byte order if necessary
    _swap32((int *)&(wavInFile->header.riff.package_len));

    // header.riff.riff_char should equal to 'RIFF');
    if (memcmp(riffStr, wavInFile->header.riff.riff_char, 4) != 0) return -1;
    // header.riff.wave should equal to 'WAVE'
    if (memcmp(waveStr, wavInFile->header.riff.wave, 4) != 0) return -1;

    return 0;
}

int WavInFile_readHeaderBlock(FILE *fptr, WavHeader *header) {
    char label[5];
    char fmtStr[5] = "fmt ";
    char factStr[5] = "fact";
    char dataStr[5] = "data";

    // lead label string
    if (fread(label, 1, 4, fptr) != 4) return -1;
    label[4] = 0;

    if (isAlphaStr(label) == 0) return -1;  // not a valid label

    // Decode blocks according to their label
    if (strcmp(label, fmtStr) == 0) {
        int nLen, nDump;

        // 'fmt ' block
        memcpy(header->format.fmt, fmtStr, 4);

        // read length of the format field
        if (fread(&nLen, sizeof(int), 1, fptr) != 1) return -1;
        // swap byte order if necessary
        _swap32(&nLen);

        // calculate how much length differs from expected
        nDump = nLen - ((int)sizeof(header->format) - 8);

        // verify that header length isn't smaller than expected structure
        if ((nLen < 0) || (nDump < 0)) return -1;

        header->format.format_len = nLen;

        // if format_len is larger than expected, read only as much data as we've space for
        if (nDump > 0) {
            nLen = sizeof(header->format) - 8;
        }

        // read data
        if (fread(&(header->format.fixed), nLen, 1, fptr) != 1) return -1;

        // swap byte order if necessary
        _swap16((short *)&(header->format.fixed));            // short int fixed;
        _swap16((short *)&(header->format.channel_number));   // short int channel_number;
        _swap32((int *)&(header->format.sample_rate));        // int sample_rate;
        _swap32((int *)&(header->format.byte_rate));          // int byte_rate;
        _swap16((short *)&(header->format.byte_per_sample));  // short int byte_per_sample;
        _swap16((short *)&(header->format.bits_per_sample));  // short int bits_per_sample;

        // if format_len is larger than expected, skip the extra data
        if (nDump > 0) {
            fseek(fptr, nDump, SEEK_CUR);
        }

        return 0;
    } else if (strcmp(label, factStr) == 0) {
        int nLen, nDump;

        // 'fact' block
        memcpy(header->fact.fact_field, factStr, 4);

        // read length of the fact field
        if (fread(&nLen, sizeof(int), 1, fptr) != 1) return -1;
        // swap byte order if necessary
        _swap32(&nLen);

        // calculate how much length differs from expected
        nDump = nLen - ((int)sizeof(header->fact) - 8);

        // verify that fact length isn't smaller than expected structure
        if ((nLen < 0) || (nDump < 0)) return -1;

        header->fact.fact_len = nLen;

        // if format_len is larger than expected, read only as much data as we've space for
        if (nDump > 0) {
            nLen = sizeof(header->fact) - 8;
        }

        // read data
        if (fread(&(header->fact.fact_sample_len), nLen, 1, fptr) != 1) return -1;

        // swap byte order if necessary
        _swap32((int *)&(header->fact.fact_sample_len));  // int sample_length;

        // if fact_len is larger than expected, skip the extra data
        if (nDump > 0) {
            fseek(fptr, nDump, SEEK_CUR);
        }

        return 0;
    } else if (strcmp(label, dataStr) == 0) {
        // 'data' block
        memcpy(header->data.data_field, dataStr, 4);
        if (fread(&(header->data.data_len), sizeof(uint), 1, fptr) != 1) return -1;

        // swap byte order if necessary
        _swap32((int *)&(header->data.data_len));

        return 1;
    } else {
        uint len, i;
        uint temp;
        // unknown block

        // read length
        if (fread(&len, sizeof(len), 1, fptr) != 1) return -1;
        // scan through the block
        for (i = 0; i < len; i++) {
            if (fread(&temp, 1, 1, fptr) != 1) return -1;
            if (feof(fptr)) return -1;  // unexpected eof
        }
    }
    return 0;
}

int WavInFile_readWavHeaders(WavInFile *wavInFile) {
    int res;

    memset(&(wavInFile->header), 0, sizeof(wavInFile->header));

    res = WavInFile_readRIFFBlock(wavInFile);
    if (res) return 1;
    // read header blocks until data block is found
    do {
        // read header blocks
        res = WavInFile_readHeaderBlock(wavInFile->fptr, &wavInFile->header);
        if (res < 0) return 1;  // error in file structure
    } while (res == 0);
    // check that all required tags are legal
    return WavInFile_checkCharTags(wavInFile);
}

uint WavInFile_getNumChannels(const WavInFile *wavInFile) { return wavInFile->header.format.channel_number; }

uint WavInFile_getNumBits(const WavInFile *wavInFile) { return wavInFile->header.format.bits_per_sample; }

uint WavInFile_getBytesPerSample(const WavInFile *wavInFile) {
    return WavInFile_getNumChannels(wavInFile) * WavInFile_getNumBits(wavInFile) / 8;
}

uint WavInFile_getSampleRate(const WavInFile *wavInFile) { return wavInFile->header.format.sample_rate; }

uint WavInFile_getDataSizeInBytes(const WavInFile *wavInFile) { return wavInFile->header.data.data_len; }

uint WavInFile_getNumSamples(const WavInFile *wavInFile) {
    if (wavInFile->header.format.byte_per_sample == 0) return 0;
    if (wavInFile->header.format.fixed > 1) return wavInFile->header.fact.fact_sample_len;
    return wavInFile->header.data.data_len / (unsigned short)wavInFile->header.format.byte_per_sample;
}

uint WavInFile_getLengthMS(const WavInFile *wavInFile) {
    double numSamples;
    double sampleRate;

    numSamples = (double)WavInFile_getNumSamples(wavInFile);
    sampleRate = (double)WavInFile_getSampleRate(wavInFile);

    return (uint)(1000.0 * numSamples / sampleRate + 0.5);
}

//////////////////////////////////////////////////////////////////////////////
//
// Class WavOutFile
//

WavOutFile *WavOutFile_create(const char *fileName, int sampleRate, int bits, int channels) {
    WavOutFile *outFile = (WavOutFile *)malloc(sizeof(WavOutFile));
    if (outFile == NULL) {
        ST_THROW_RT_ERROR("Error: Unable to allocate memory for WavOutFile");
    }
    outFile->bytesWritten = 0;
    outFile->fptr = fopen(fileName, "wb");
    if (outFile->fptr == NULL) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Error: Unable to open file \"%s\" for writing.", fileName);
        assert(0 && msg);
    }
    outFile->base = WavFileBase_create();

    WavOutFile_fillInHeader(outFile, sampleRate, bits, channels);
    WavOutFile_writeHeader(outFile);

    return outFile;
}

WavOutFile *WavOutFile_createWithFile(FILE *file, int sampleRate, int bits, int channels) {
    WavOutFile *outFile = (WavOutFile *)malloc(sizeof(WavOutFile));
    if (outFile == NULL) {
        ST_THROW_RT_ERROR("Error: Unable to allocate memory for WavOutFile");
    }
    outFile->bytesWritten = 0;
    outFile->fptr = file;
    if (outFile->fptr == NULL) {
        char msg[] = "Error: Unable to access output file stream.";
        assert(0 && msg);
    }
    outFile->base = WavFileBase_create();

    WavOutFile_fillInHeader(outFile, sampleRate, bits, channels);
    WavOutFile_writeHeader(outFile);

    return outFile;
}

void WavOutFile_destroy(WavOutFile *outFile) {
    WavOutFile_finishHeader(outFile);
    if (outFile->fptr) {
        fclose(outFile->fptr);
        outFile->fptr = NULL;
    }
    WavFileBase_destroy(outFile->base);
    free(outFile);
}

void WavOutFile_fillInHeader(WavOutFile *outFile, unsigned int sampleRate, unsigned int bits, unsigned int channels) {
    char riffStr[] = "RIFF";
    char waveStr[] = "WAVE";
    char fmtStr[] = "fmt ";
    char factStr[] = "fact";
    char dataStr[] = "data";

    memcpy(&(outFile->header.riff.riff_char), riffStr, 4);
    outFile->header.riff.package_len = 0;
    memcpy(&(outFile->header.riff.wave), waveStr, 4);

    memcpy(&(outFile->header.format.fmt), fmtStr, 4);
    outFile->header.format.format_len = 0x10;
    outFile->header.format.fixed = 1;
    outFile->header.format.channel_number = (short)channels;
    outFile->header.format.sample_rate = (int)sampleRate;
    outFile->header.format.bits_per_sample = (short)bits;
    outFile->header.format.byte_per_sample = (short)(bits * channels / 8);
    outFile->header.format.byte_rate = outFile->header.format.byte_per_sample * (int)sampleRate;
    outFile->header.format.sample_rate = (int)sampleRate;

    memcpy(&(outFile->header.fact.fact_field), factStr, 4);
    outFile->header.fact.fact_len = 4;
    outFile->header.fact.fact_sample_len = 0;

    memcpy(&(outFile->header.data.data_field), dataStr, 4);
    outFile->header.data.data_len = 0;
}

void WavOutFile_finishHeader(WavOutFile *outFile) {
    outFile->header.riff.package_len = outFile->bytesWritten + sizeof(WavHeader) - sizeof(WavRiff) + 4;
    outFile->header.data.data_len = outFile->bytesWritten;
    outFile->header.fact.fact_sample_len = outFile->bytesWritten / outFile->header.format.byte_per_sample;

    WavOutFile_writeHeader(outFile);
}

void WavOutFile_writeHeader(WavOutFile *outFile) {
    WavHeader hdrTemp;
    int res;

    hdrTemp = outFile->header;
    _swap32((int *)&hdrTemp.riff.package_len);
    _swap32((int *)&hdrTemp.format.format_len);
    _swap16((short *)&hdrTemp.format.fixed);
    _swap16((short *)&hdrTemp.format.channel_number);
    _swap32((int *)&hdrTemp.format.sample_rate);
    _swap32((int *)&hdrTemp.format.byte_rate);
    _swap16((short *)&hdrTemp.format.byte_per_sample);
    _swap16((short *)&hdrTemp.format.bits_per_sample);
    _swap32((int *)&hdrTemp.data.data_len);
    _swap32((int *)&hdrTemp.fact.fact_len);
    _swap32((int *)&hdrTemp.fact.fact_sample_len);

    fseek(outFile->fptr, 0, SEEK_SET);
    res = (int)fwrite(&hdrTemp, sizeof(hdrTemp), 1, outFile->fptr);
    if (res != 1) {
        ST_THROW_RT_ERROR("Error while writing to a wav file.");
    }

    fseek(outFile->fptr, 0, SEEK_END);
}

static int saturate(float fvalue, float minval, float maxval) {
    if (fvalue > maxval) {
        fvalue = maxval;
    } else if (fvalue < minval) {
        fvalue = minval;
    }
    return (int)fvalue;
}

void WavOutFile_write(WavOutFile *outFile, const float *buffer, int numElems) {
    int numBytes;
    int bytesPerSample;

    if (numElems == 0) return;

    bytesPerSample = outFile->header.format.bits_per_sample / 8;
    numBytes = numElems * bytesPerSample;
    void *temp = WavFileBase_getConvBuffer(outFile->base, numBytes + 7);

    switch (bytesPerSample) {
        case 1: {
            unsigned char *temp2 = (unsigned char *)temp;
            for (int i = 0; i < numElems; i++) {
                temp2[i] = (unsigned char)saturate(buffer[i] * 128.0f + 128.0f, 0.0f, 255.0f);
            }
            break;
        }

        case 2: {
            short *temp2 = (short *)temp;
            for (int i = 0; i < numElems; i++) {
                short value = (short)saturate(buffer[i] * 32768.0f, -32768.0f, 32767.0f);
                temp2[i] = _swap16(&value);
            }
            break;
        }

        case 3: {
            char *temp2 = (char *)temp;
            for (int i = 0; i < numElems; i++) {
                int value = saturate(buffer[i] * 8388608.0f, -8388608.0f, 8388607.0f);
                *((int *)temp2) = _swap32(&value);
                temp2 += 3;
            }
            break;
        }

        case 4: {
            int *temp2 = (int *)temp;
            for (int i = 0; i < numElems; i++) {
                int value = saturate(buffer[i] * 2147483648.0f, -2147483648.0f, 2147483647.0f);
                temp2[i] = _swap32(&value);
            }
            break;
        }

        default:
            assert(0);
    }

    int res = (int)fwrite(temp, 1, numBytes, outFile->fptr);

    if (res != numBytes) {
        ST_THROW_RT_ERROR("Error while writing to a wav file.");
    }
    outFile->bytesWritten += numBytes;
}
