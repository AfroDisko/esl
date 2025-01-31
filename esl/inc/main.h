#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>

#include "utils.h"

typedef enum
{
    InputModeNone,
    InputModeModifyH,
    InputModeModifyS,
    InputModeModifyV
} InputMode;

typedef struct 
{
    InputMode mode;
    ColorHSV  color;
    uint8_t*  ptrColorParam;
} Context;

#endif
