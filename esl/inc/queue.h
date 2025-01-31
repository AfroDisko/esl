#ifndef QUEUE_H
#define QUEUE_H

#include "utils.h"

typedef enum
{
    EventNone,
    EventSwitchPressed,
    EventSwitchPressedContinuous,
    EventSwitchReleased,
    EventChangeColorRGB,
    EventChangeColorHSV
} EventType;

typedef union
{
    uint8_t  num;
    ColorRGB rgb;
    ColorHSV hsv;
} EventData;

typedef struct
{
    EventType type;
    EventData data;
} Event;

void queueEventEnqueue(Event event);

Event queueEventDequeue(void);

#endif
