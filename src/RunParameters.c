
#include "RunParameters.h"

// Program usage instructions
static const char whatText[] =
    "This application processes WAV audio files by modifying the sound tempo,\n"
    "pitch and playback rate properties independently from each other.\n"
    "\n";

static const char usage[] =
    "Usage :\n"
    "    soundstretch infilename outfilename [switches]\n"
    "\n"
    "To use standard input/output pipes, give 'stdin' and 'stdout' as filenames.\n"
    "\n"
    "Available switches are:\n"
    "  -tempo=n : Change sound tempo by n percents  (n=-95..+5000 %)\n"
    "  -pitch=n : Change sound pitch by n semitones (n=-60..+60 semitones)\n"
    "  -rate=n  : Change sound rate by n percents   (n=-95..+5000 %)\n"
    "  -quick   : Use quicker tempo change algorithm (gain speed, lose quality)\n"
    "  -naa     : Don't use anti-alias filtering (gain speed, lose quality)\n"
    "  -speech  : Tune algorithm for speech processing (default is for music)\n"
    "  -license : Display the program license text (LGPL)\n";

// Converts a char into lower case
static int _toLowerCase(int c) {
    if (c >= 'A' && c <= 'Z') {
        c += 'a' - 'A';
    }
    return c;
}

// Checks parameter limits
static void checkLimits(RunParameters *parameters) {
    if (parameters->tempoDelta < -95.0f) {
        parameters->tempoDelta = -95.0f;
    } else if (parameters->tempoDelta > 5000.0f) {
        parameters->tempoDelta = 5000.0f;
    }

    if (parameters->pitchDelta < -60.0f) {
        parameters->pitchDelta = -60.0f;
    } else if (parameters->pitchDelta > 60.0f) {
        parameters->pitchDelta = 60.0f;
    }

    if (parameters->rateDelta < -95.0f) {
        parameters->rateDelta = -95.0f;
    } else if (parameters->rateDelta > 5000.0f) {
        parameters->rateDelta = 5000.0f;
    }
}

// Unknown switch parameter -- throws an exception with an error message
void throwIllegalParamExp(const char *str) {
    char msg[256];
    strcpy(msg, "ERROR : Illegal parameter \"");
    strcat(msg, str);
    strcat(msg, "\".\n\n");
    strcat(msg, usage);
    ST_THROW_RT_ERROR(msg);
}

// Unknown switch parameter -- print an error message
float parseSwitchValue(const char *str) {
    int pos;

    pos = (int)(strchr(str, '=') - str);
    if (pos < 0) {
        // '=' missing
        throwIllegalParamExp(str);
    }

    // Read numerical parameter value after '='
    return (float)atof(str + pos + 1);
}

// Interprets a single switch parameter string of format "-switch=xx"
// Valid switches are "-tempo=xx", "-pitch=xx" and "-rate=xx". Stores
// switch values into 'params' structure.
static void parseSwitchParam(RunParameters *parameters, const char *str) {
    int upS;

    if (str[0] != '-') {
        // leading hyphen missing => not a valid parameter
        throwIllegalParamExp(str);
    }

    // Take the first character of switch name & change to lower case
    upS = _toLowerCase(str[1]);

    // interpret the switch name & operate accordingly
    switch (upS) {
        case 't':
            // switch '-tempo=xx'
            parameters->tempoDelta = parseSwitchValue(str);
            break;

        case 'p':
            // switch '-pitch=xx'
            parameters->pitchDelta = parseSwitchValue(str);
            break;

        case 'r':
            // switch '-rate=xx'
            parameters->rateDelta = parseSwitchValue(str);
            break;

        case 'q':
            // switch '-quick'
            parameters->quick = 1;
            break;

        case 'n':
            // switch '-naa'
            parameters->noAntiAlias = 1;
            break;

        case 's':
            // switch '-speech'
            parameters->speech = 1;
            break;

        default:
            // unknown switch
            throwIllegalParamExp(str);
    }
}

// Constructor
void InitRunParameters(RunParameters *parameters, const int nParams, const char *const paramStr[]) {
    int i;
    int nFirstParam;

    if (nParams < 3) {
        // Too few parameters
        char msg[256];
        strcpy(msg, whatText);
        strcat(msg, usage);
        ST_THROW_RT_ERROR(msg);
    }

    parameters->inFileName = NULL;
    parameters->outFileName = NULL;
    parameters->tempoDelta = 0;
    parameters->pitchDelta = 0;
    parameters->rateDelta = 0;
    parameters->quick = 0;
    parameters->noAntiAlias = 0;
    parameters->speech = 0;

    // Get input & output file names
    parameters->inFileName = (char *)paramStr[1];
    parameters->outFileName = (char *)paramStr[2];

    if (parameters->outFileName[0] == '-') {
        // no outputfile name was given but parameters
        parameters->outFileName = NULL;
        nFirstParam = 2;
    } else {
        nFirstParam = 3;
    }

    // parse switch parameters
    for (i = nFirstParam; i < nParams; i++) {
        parseSwitchParam(parameters, paramStr[i]);
    }

    checkLimits(parameters);
}