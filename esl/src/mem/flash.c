#include <string.h>

#include "nrf_bootloader_info.h"
#include "nrf_dfu_types.h"

#include "nrf_fstorage.h"
#include "nrf_fstorage_sd.h"

#include "nrf_log.h"

#include "metadata.h"
#include "flash.h"

#ifdef  BOOTLOADER_START_ADDR
#undef  BOOTLOADER_START_ADDR
#define BOOTLOADER_START_ADDR 0xE0000
#endif

#define APP_DATA_START_ADDR_P0 (BOOTLOADER_START_ADDR - NRF_DFU_APP_DATA_AREA_SIZE)
#define APP_DATA_START_ADDR_P1 (APP_DATA_START_ADDR_P0 + 1 * CODE_PAGE_SIZE)
#define APP_DATA_START_ADDR_P2 (APP_DATA_START_ADDR_P0 + 2 * CODE_PAGE_SIZE)
#define APP_DATA_BEYOND        (APP_DATA_START_ADDR_P0 + 3 * CODE_PAGE_SIZE)

#define APP_DATA_PAGES_NUM 3

#define DATA_OFFSET      4
#define DATA_BUFFER_SIZE UINT8_MAX

static const uint32_t gAppDataStartAddr[] =
{
    APP_DATA_START_ADDR_P0,
    APP_DATA_START_ADDR_P1,
    APP_DATA_START_ADDR_P2
};

static const Metadata gMetadataNone =
{
    .type   = METADATA_TYPE_NONE,
    .state  = METADATA_STATE_NONE,
    .length = UINT8_MAX
};

static const Metadata gMetadataPage =
{
    .type   = METADATA_TYPE_PAGE_INFO,
    .state  = METADATA_STATE_ACTIVE,
    .length = 0
};

NRF_FSTORAGE_DEF(nrf_fstorage_t gStorage) =
{
    .start_addr = APP_DATA_START_ADDR_P0,
    .end_addr   = APP_DATA_BEYOND
};

static uint8_t flashAlignLength(uint8_t len)
{
    if(len % 4 != 0)
        len = len / 4 + 4;
    return len;
}

static void flashAwait(void)
{
    while(nrf_fstorage_is_busy(&gStorage)){}
}

static void flashMetadataWrite(uint32_t addr, Metadata meta)
{
    uint8_t bytes[2];
    bytes[0] = (meta.type & METADATA_MASK_TYPE) | (meta.state & METADATA_MASK_STATE);
    bytes[1] = meta.length;

    nrf_fstorage_write(&gStorage, addr, bytes, DATA_OFFSET, NULL);
    flashAwait();
}

static Metadata flashMetadataRead(uint32_t addr)
{
    Metadata meta =
    {
        .type   = *(uint8_t*)addr & METADATA_MASK_TYPE,
        .state  = *(uint8_t*)addr & METADATA_MASK_STATE,
        .length = *(uint8_t*)(addr + 1)
    };
    return meta;
}

static uint32_t flashGetNextAddr(uint32_t addrCurr)
{
    Metadata meta = flashMetadataRead(addrCurr);
    return addrCurr + DATA_OFFSET + meta.length;
}

static void flashPageErase(uint8_t pageIdx)
{
    nrf_fstorage_erase(&gStorage, gAppDataStartAddr[pageIdx], 1, NULL);
    flashMetadataWrite(gAppDataStartAddr[pageIdx], gMetadataPage);
}

void flashSetup(bool force)
{
    nrf_fstorage_init(&gStorage, &nrf_fstorage_sd, NULL);

    for(uint8_t pageIdx = 0; pageIdx < APP_DATA_PAGES_NUM; ++pageIdx)
    {
        Metadata meta = flashMetadataRead(gAppDataStartAddr[pageIdx]);
        if(!metadataIsEqual(&meta, &gMetadataPage) || force)
            flashPageErase(pageIdx);
    }
}

static FlashRetCode flashRecordWrite(uint8_t pageIdx, uint32_t addr, Metadata meta, const uint8_t* data)
{
    if(DATA_OFFSET + meta.length > gAppDataStartAddr[pageIdx] + CODE_PAGE_SIZE - addr)
        return FlashRetCodeBeyondPage;

    flashMetadataWrite(addr, meta);
    nrf_fstorage_write(&gStorage, addr + DATA_OFFSET, data, meta.length, NULL);
    flashAwait();

    return FlashRetCodeSuccess;
}

static FlashRetCode flashRecordFindLast(uint8_t pageIdx, uint32_t* addr)
{
    uint32_t addrCurr = gAppDataStartAddr[pageIdx];
    uint32_t addrNext = flashGetNextAddr(addrCurr);

    Metadata metaNext = flashMetadataRead(addrNext);

    while(!metadataIsEqual(&metaNext, &gMetadataNone))
    {
        addrCurr = addrNext;
        addrNext = flashGetNextAddr(addrCurr);

        metaNext = flashMetadataRead(addrNext);
    }

    *addr = addrCurr;
    return FlashRetCodeSuccess;
}

static FlashRetCode flashRecordFindLastMeta(uint8_t pageIdx, uint32_t* addr, Metadata metaRef)
{
    uint32_t addrCurr = gAppDataStartAddr[pageIdx];
    uint32_t addrNext = flashGetNextAddr(addrCurr);

    Metadata metaCurr = flashMetadataRead(addrCurr);
    Metadata metaNext = flashMetadataRead(addrNext);

    *addr = gAppDataStartAddr[pageIdx];

    while(!metadataIsEqual(&metaNext, &gMetadataNone))
    {
        addrCurr = addrNext;
        addrNext = flashGetNextAddr(addrCurr);

        metaCurr = flashMetadataRead(addrCurr);
        metaNext = flashMetadataRead(addrNext);

        if(metadataIsEqual(&metaCurr, &metaRef))
            *addr = addrCurr;
    }

    if(*addr > gAppDataStartAddr[pageIdx])
        return FlashRetCodeSuccess;
    else
        return FlashRetCodeMetaNotFound;
}

static FlashRetCode flashRecordFindFree(uint8_t pageIdx, uint32_t* addr)
{
    uint32_t addrCurr = gAppDataStartAddr[pageIdx];
    uint32_t addrNext = flashGetNextAddr(addrCurr);

    Metadata metaNext = flashMetadataRead(addrNext);

    while(!metadataIsEqual(&metaNext, &gMetadataNone))
    {
        addrCurr = addrNext;
        addrNext = flashGetNextAddr(addrCurr);

        metaNext = flashMetadataRead(addrNext);
    }

    *addr = addrNext;
    return FlashRetCodeSuccess;
}

uint32_t flashRecordCountMeta(uint8_t pageIdx, Metadata metaRef)
{
    uint32_t addrCurr = gAppDataStartAddr[pageIdx];
    uint32_t addrNext = flashGetNextAddr(addrCurr);

    Metadata metaCurr = flashMetadataRead(addrCurr);
    Metadata metaNext = flashMetadataRead(addrNext);

    uint32_t cntr = 0;

    while(!metadataIsEqual(&metaNext, &gMetadataNone))
    {
        addrCurr = addrNext;
        addrNext = flashGetNextAddr(addrCurr);

        metaCurr = flashMetadataRead(addrCurr);
        metaNext = flashMetadataRead(addrNext);

        if(metadataIsCommon(&metaCurr, &metaRef))
            ++cntr;
    }

    return cntr;
}

void flashSaveColorHSV(ColorHSV hsv)
{
    uint32_t addr;
    if(flashRecordFindFree(0, &addr) == FlashRetCodeBeyondPage)
    {
        flashPageErase(0);
        flashRecordFindFree(0, &addr);
    }

    Metadata meta =
    {
        .type   = METADATA_TYPE_COLOR_HSV,
        .state  = METADATA_STATE_ACTIVE,
        .length = flashAlignLength(3)
    };

    uint8_t data[DATA_BUFFER_SIZE];
    data[0] = hsv.h;
    data[1] = hsv.s;
    data[2] = hsv.v;

    flashRecordWrite(0, addr, meta, data);
}

void flashLoadColorHSV(ColorHSV* hsv)
{
    Metadata meta =
    {
        .type   = METADATA_TYPE_COLOR_HSV,
        .state  = METADATA_STATE_ACTIVE,
        .length = flashAlignLength(3)
    };

    uint32_t addr;
    if(flashRecordFindLastMeta(0, &addr, meta) == FlashRetCodeSuccess)
    {
        hsv->h = *(uint8_t*)(addr + DATA_OFFSET);
        hsv->s = *(uint8_t*)(addr + DATA_OFFSET + 1);
        hsv->v = *(uint8_t*)(addr + DATA_OFFSET + 2);
    }
}

void flashSaveColorRGBNamed(ColorRGB rgb, const char* name)
{
    uint32_t addr;
    if(flashRecordFindFree(1, &addr) == FlashRetCodeBeyondPage)
    {
        flashPageErase(1);
        flashRecordFindFree(1, &addr);
    }

    Metadata meta =
    {
        .type   = METADATA_TYPE_COLOR_RGB_NAMED,
        .state  = METADATA_STATE_ACTIVE,
        .length = flashAlignLength(3 + strlen(name))
    };

    uint8_t data[DATA_BUFFER_SIZE];
    data[0] = rgb.r;
    data[1] = rgb.g;
    data[2] = rgb.b;
    strcpy((char*)&data[3], name);

    flashRecordWrite(1, addr, meta, data);
}

FlashRetCode flashLoadColorRGBNamed(ColorRGB* rgb, const char* name)
{
    Metadata meta =
    {
        .type   = METADATA_TYPE_COLOR_RGB_NAMED,
        .state  = METADATA_STATE_ACTIVE,
        .length = flashAlignLength(3 + strlen(name))
    };

    uint32_t addr;
    FlashRetCode retCode = flashRecordFindLastMeta(1, &addr, meta);
    if(retCode == FlashRetCodeSuccess)
    {
        rgb->r = *(uint8_t*)(addr + DATA_OFFSET);
        rgb->g = *(uint8_t*)(addr + DATA_OFFSET + 1);
        rgb->b = *(uint8_t*)(addr + DATA_OFFSET + 2);
    }

    return retCode;
}

FlashRetCode flashDeleteColorRGBNamed(const char* name)
{
    Metadata meta =
    {
        .type   = METADATA_TYPE_COLOR_RGB_NAMED,
        .state  = METADATA_STATE_ACTIVE,
        .length = flashAlignLength(3 + strlen(name))
    };

    uint32_t addr;
    FlashRetCode retCode = flashRecordFindLastMeta(1, &addr, meta);
    if(retCode == FlashRetCodeSuccess)
    {
        meta.state = METADATA_STATE_DELETED;
        flashMetadataWrite(addr, meta);
    }

    return retCode;
}
