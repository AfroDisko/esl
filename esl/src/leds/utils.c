#include "utils.h"

ColorRGB hsv2rgb(ColorHSV hsv)
{
    ColorRGB rgb;

    if(hsv.s == 0)
    {
        rgb.r = hsv.v;
        rgb.g = hsv.v;
        rgb.b = hsv.v;
        return rgb;
    }

    uint8_t reg = hsv.h / 43;
    uint8_t res = (hsv.h - (reg * 43)) * 6; 

    uint8_t p = (hsv.v * (UINT8_MAX - hsv.s)) >> 8;
    uint8_t q = (hsv.v * (UINT8_MAX - ((hsv.s * res) >> 8))) >> 8;
    uint8_t t = (hsv.v * (UINT8_MAX - ((hsv.s * (UINT8_MAX - res)) >> 8))) >> 8;

    switch(reg)
    {
    case 0:
        rgb.r = hsv.v;
        rgb.g = t;
        rgb.b = p;
        break;

    case 1:
        rgb.r = q;
        rgb.g = hsv.v;
        rgb.b = p;
        break;

    case 2:
        rgb.r = p;
        rgb.g = hsv.v;
        rgb.b = t;
        break;

    case 3:
        rgb.r = p;
        rgb.g = q;
        rgb.b = hsv.v;
        break;

    case 4:
        rgb.r = t;
        rgb.g = p;
        rgb.b = hsv.v;
        break;

    default:
        rgb.r = hsv.v;
        rgb.g = p;
        rgb.b = q;
        break;
    }

    return rgb;
}

ColorHSV rgb2hsv(ColorRGB rgb)
{
    ColorHSV hsv;
    
    uint8_t rgbMin, rgbMax;

    rgbMin = rgb.r < rgb.g ? (rgb.r < rgb.b ? rgb.r : rgb.b) : (rgb.g < rgb.b ? rgb.g : rgb.b);
    rgbMax = rgb.r > rgb.g ? (rgb.r > rgb.b ? rgb.r : rgb.b) : (rgb.g > rgb.b ? rgb.g : rgb.b);

    hsv.v = rgbMax;
    if(hsv.v == 0)
    {
        hsv.h = 0;
        hsv.s = 0;
        return hsv;
    }

    hsv.s = UINT8_MAX * (rgbMax - rgbMin) / hsv.v;
    if(hsv.s == 0)
    {
        hsv.h = 0;
        return hsv;
    }

    if(rgbMax == rgb.r)
        hsv.h = 43 * (rgb.g - rgb.b) / (rgbMax - rgbMin);
    else if(rgbMax == rgb.g)
        hsv.h = 85 + 43 * (rgb.b - rgb.r) / (rgbMax - rgbMin);
    else
        hsv.h = 171 + 43 * (rgb.r - rgb.g) / (rgbMax - rgbMin);

    return hsv;
}
