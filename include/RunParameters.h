
#ifndef RUNPARAMETERS_H
#define RUNPARAMETERS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "STTypes.h"

typedef struct {
    char *inFileName;
    char *outFileName;
    float tempoDelta;
    float pitchDelta;
    float rateDelta;
    int quick;
    int noAntiAlias;
    int speech;
} RunParameters;

void InitRunParameters(RunParameters *parameters, const int nParams, const char *const paramStr[]);

#endif
