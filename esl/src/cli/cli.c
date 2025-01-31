#include <string.h>
#include <stdlib.h>

#include "app_usbd_cdc_acm.h"

#include "queue.h"
#include "leds.h"
#include "utils.h"
#include "flash.h"
#include "metadata.h"

#include "cmd.h"
#include "cli.h"

#define BUFFER_SIZE_ECHO 1
#define BUFFER_SIZE_MAIN 256

#define COMMAND_WORD_NUM_MAX 8
#define COMMAND_WORD_LEN_MAX 32

static char gBufferEcho[BUFFER_SIZE_ECHO];
static char gBufferMain[BUFFER_SIZE_MAIN];

static char gCommand[COMMAND_WORD_NUM_MAX][COMMAND_WORD_LEN_MAX];

static void usbdHandler(const app_usbd_class_inst_t* p_inst,
                        app_usbd_cdc_acm_user_event_t event);

APP_USBD_CDC_ACM_GLOBAL_DEF(usbdInstance, usbdHandler,
                            CDC_ACM_COMM_INTERFACE, CDC_ACM_DATA_INTERFACE,
                            CDC_ACM_COMM_EPIN, CDC_ACM_DATA_EPIN, CDC_ACM_DATA_EPOUT,
                            APP_USBD_CDC_COMM_PROTOCOL_NONE);

static void cliBufferReset(void)
{
    gBufferEcho[0] = '\0';
    gBufferMain[0] = '\0';
    for(uint8_t idx = 0; idx < COMMAND_WORD_NUM_MAX; ++idx)
        gCommand[idx][0] = '\0';
}

void cliSetup(void)
{
    cliBufferReset();
    app_usbd_class_append(app_usbd_cdc_acm_class_inst_get(&usbdInstance));
}

static void cliBufferAppend(void)
{
    if(strlen(gBufferMain) + 1 < BUFFER_SIZE_MAIN)
        strcat(gBufferMain, gBufferEcho);
}

static void cliBufferParse(void)
{
    char*   token = strtok(gBufferMain, " ");
    uint8_t tokenIdx = 0;
    while(token != NULL && tokenIdx < COMMAND_WORD_NUM_MAX)
    {
        strcpy(gCommand[tokenIdx++], token);
        token = strtok(NULL, " ");
    }
}

static void cliExecCommand(void)
{
    if(strcmp(gCommand[0], gCmdHelp) == 0)
    {
        app_usbd_cdc_acm_write(&usbdInstance, gCmdResponseHelp, sizeof(gCmdResponseHelp));
        return;
    }

    if(strcmp(gCommand[0], gCmdRgb) == 0)
    {
        ColorRGB rgb =
        {
            .r = strtoul(gCommand[1], NULL, 10),
            .g = strtoul(gCommand[2], NULL, 10),
            .b = strtoul(gCommand[3], NULL, 10)
        };
        queueEventEnqueue((Event){EventChangeColorRGB, {.rgb = rgb}});
        return;
    }

    if(strcmp(gCommand[0], gCmdHsv) == 0)
    {
        ColorHSV hsv =
        {
            .h = strtoul(gCommand[1], NULL, 10),
            .s = strtoul(gCommand[2], NULL, 10),
            .v = strtoul(gCommand[3], NULL, 10)
        };
        queueEventEnqueue((Event){EventChangeColorHSV, {.hsv = hsv}});
        return;
    }

    if(strcmp(gCommand[0], gCmdColorAddRgb) == 0)
    {
        Metadata meta =
        {
            .type   = METADATA_TYPE_COLOR_RGB_NAMED,
            .state  = METADATA_STATE_ACTIVE,
            .length = 0
        };
        if(flashRecordCountMeta(1, meta) > 10)
        {
            app_usbd_cdc_acm_write(&usbdInstance, gCmdResponseNoSpace, sizeof(gCmdResponseNoSpace));
            return;
        }
        ColorRGB rgb =
        {
            .r = strtoul(gCommand[1], NULL, 10),
            .g = strtoul(gCommand[2], NULL, 10),
            .b = strtoul(gCommand[3], NULL, 10)
        };
        flashSaveColorRGBNamed(rgb, gCommand[4]);
        return;
    }

    if(strcmp(gCommand[0], gCmdColorAddCur) == 0)
    {
        ColorRGB rgb = ledsGetLED2State();
        flashSaveColorRGBNamed(rgb, gCommand[1]);
        return;
    }

    if(strcmp(gCommand[0], gCmdColorSet) == 0)
    {
        ColorRGB rgb;
        FlashRetCode retCode = flashLoadColorRGBNamed(&rgb, gCommand[1]);
        if(retCode == FlashRetCodeSuccess)
            queueEventEnqueue((Event){EventChangeColorRGB, {.rgb = rgb}});
        if(retCode == FlashRetCodeMetaNotFound)
            app_usbd_cdc_acm_write(&usbdInstance, gCmdResponseNoColor, sizeof(gCmdResponseNoColor));
        return;
    }

    if(strcmp(gCommand[0], gCmdColorDel) == 0)
    {
        FlashRetCode retCode = flashDeleteColorRGBNamed(gCommand[1]);
        if(retCode == FlashRetCodeMetaNotFound)
            app_usbd_cdc_acm_write(&usbdInstance, gCmdResponseNoColor, sizeof(gCmdResponseNoColor));
        return;
    }

    if(strcmp(gCommand[0], "") != 0)
        app_usbd_cdc_acm_write(&usbdInstance, gCmdResponseUnknownCmd, sizeof(gCmdResponseUnknownCmd));
}

static bool cliCRLF(void)
{
    return gBufferEcho[0] == '\r' || gBufferEcho[0] == '\n';
}

static void cliProcessInput(void)
{
    if(cliCRLF())
    {
        app_usbd_cdc_acm_write(&usbdInstance, "\r\n", 2);
        cliBufferParse();
        cliExecCommand();
        cliBufferReset();
    }
    else
    {
        cliBufferAppend();
        app_usbd_cdc_acm_write(&usbdInstance, gBufferMain + strlen(gBufferMain) - 1, 1);
    }
}

static void usbdHandler(app_usbd_class_inst_t const* p_inst,
                        app_usbd_cdc_acm_user_event_t event)
{
    switch(event)
    {
    case APP_USBD_CDC_ACM_USER_EVT_PORT_OPEN:
        app_usbd_cdc_acm_read(&usbdInstance, gBufferEcho, BUFFER_SIZE_ECHO);
        break;

    case APP_USBD_CDC_ACM_USER_EVT_PORT_CLOSE:
        break;

    case APP_USBD_CDC_ACM_USER_EVT_TX_DONE:
        break;

    case APP_USBD_CDC_ACM_USER_EVT_RX_DONE:
        app_usbd_cdc_acm_read(&usbdInstance, gBufferEcho, BUFFER_SIZE_ECHO);
        cliProcessInput();
        break;

    default:
        break;
    }
}
