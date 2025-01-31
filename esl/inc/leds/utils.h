#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

typedef struct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} ColorRGB;

typedef struct
{
    uint8_t h;
    uint8_t s;
    uint8_t v;
} ColorHSV;

ColorRGB hsv2rgb(ColorHSV hsv);

ColorHSV rgb2hsv(ColorRGB rgb);

#endif
