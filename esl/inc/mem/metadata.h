#ifndef METADATA_H
#define METADATA_H

#include <stdint.h>
#include <stdbool.h>

#define METADATA_TYPE_PAGE_INFO       0x00
#define METADATA_TYPE_COLOR_RGB       0x10
#define METADATA_TYPE_COLOR_HSV       0x20
#define METADATA_TYPE_COLOR_RGB_NAMED 0x30
#define METADATA_TYPE_COLOR_HSV_NAMED 0x40
#define METADATA_TYPE_NONE            0xf0

#define METADATA_STATE_DELETED        0x00
#define METADATA_STATE_ACTIVE         0x01
#define METADATA_STATE_NONE           0x0f

#define METADATA_MASK_TYPE            0xf0
#define METADATA_MASK_STATE           0x0f

typedef struct
{
    uint8_t type;
    uint8_t state;
    uint8_t length;
} Metadata;

bool metadataIsEqual(const Metadata* m1, const Metadata* m2);

bool metadataIsCommon(const Metadata* m1, const Metadata* m2);

#endif
