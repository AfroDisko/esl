#ifndef FLASH_H
#define FLASH_H

#include <stdbool.h>

#include "utils.h"
#include "metadata.h"

typedef enum
{
    FlashRetCodeSuccess,
    FlashRetCodeBeyondPage,
    FlashRetCodeMetaNotFound
} FlashRetCode;

void flashSetup(bool force);

void flashSaveColorHSV(ColorHSV hsv);

void flashLoadColorHSV(ColorHSV* hsv);

void flashSaveColorRGBNamed(ColorRGB rgb, const char* name);

FlashRetCode flashLoadColorRGBNamed(ColorRGB* rgb, const char* mark);

FlashRetCode flashDeleteColorRGBNamed(const char* name);

uint32_t flashRecordCountMeta(uint8_t pageIdx, Metadata metaRef);

#endif
