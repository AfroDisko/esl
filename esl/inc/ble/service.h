#ifndef SERVICE_H
#define SERVICE_H

#include <stdint.h>

#include "ble.h"

#include "utils.h"

#define UUID_BLE_SERVICE_BASE {0x2E, 0x4B, 0x06, 0xCC, 0xD0, 0x44, 0x46, 0x0F, 0xA4, 0xA1, 0x6D, 0x70, 0xC0, 0x27, 0x77, 0x70}
#define UUID_BLE_SERVICE_SHRT 0x0000

typedef struct
{
    ble_uuid_t uuid;
    uint16_t   hserv;
    uint16_t   hconn;
} BLEService;

typedef struct
{
    ble_uuid_t               uuid;
    ble_gatts_attr_md_t      attrmd;
    ble_gatts_char_md_t      charmd;
    ble_gatts_char_handles_t handles;
} BLEAttr;

ret_code_t bleServiceSetup(void);

ret_code_t bleServiceAttrHSVSetup(ColorHSV* ptr);
ret_code_t bleServiceAttrHSVNotify(void);
uint32_t   bleServiceAttrHSVGetHandle(void);

ret_code_t bleServiceAttrInputSetup(ColorHSV* ptr);

#endif
