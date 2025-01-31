#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "nrf_pwr_mgmt.h"
#include "nrfx_gpiote.h"
#include "app_timer.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_log_backend_usb.h"

#include "main.h"
#include "queue.h"
#include "switch.h"
#include "leds.h"
#include "flash.h"
#include "cli.h"
#include "stack.h"
#include "service.h"

#define COLOR_MOD_PERIOD_MS 10

APP_TIMER_DEF(gTimerColorMod);

static Context gCtx =
{
    .mode  = InputModeNone,
    .color =
    {
        .h = 247,
        .s = 255,
        .v = 255
    },
    .ptrColorParam = NULL
};

static void modifyColorParam(void* p_context)
{
    Context* ctx = (Context*)p_context;

    static bool increase = true;

    if(*(ctx->ptrColorParam) == 0)
        increase = true;

    if(*(ctx->ptrColorParam) == 255)
        increase = false;

    if(ctx->mode == InputModeModifyH)
        increase = true;

    if(increase)
        ++*(ctx->ptrColorParam);
    else
        --*(ctx->ptrColorParam);

    ledsSetLED2StateHSV(ctx->color);
    bleServiceAttrHSVNotify();
}

static void switchMode(Context* ctx)
{
    switch(ctx->mode)
    {
    case InputModeNone:
        ctx->mode = InputModeModifyH;
        ctx->ptrColorParam = &(ctx->color).h;
        break;

    case InputModeModifyH:
        ctx->mode = InputModeModifyS;
        ctx->ptrColorParam = &(ctx->color).s;
        break;

    case InputModeModifyS:
        ctx->mode = InputModeModifyV;
        ctx->ptrColorParam = &(ctx->color).v;
        break;

    case InputModeModifyV:
        ctx->mode = InputModeNone;
        ctx->ptrColorParam = NULL;
        break;

    default:
        break;
    }
}

static void updateState(Context* ctx)
{
    switch(ctx->mode)
    {
    case InputModeNone:
        flashSaveColorHSV(ctx->color);
        ledsFlashLED1Halt();
        ledsSetLED1State(0);
        break;

    case InputModeModifyH:
        ledsFlashLED1(FlashModeSlow);
        break;

    case InputModeModifyS:
        ledsFlashLED1(FlashModeFast);
        break;

    case InputModeModifyV:
        ledsFlashLED1Halt();
        ledsSetLED1State(255);
        break;

    default:
        break;
    }
}

int main(void)
{
    NRF_LOG_INIT(NULL);
    NRF_LOG_DEFAULT_BACKENDS_INIT();

    nrf_pwr_mgmt_init();
    nrfx_gpiote_init();

    app_timer_init();
    app_timer_create(&gTimerColorMod, APP_TIMER_MODE_REPEATED, modifyColorParam);

    switchSetupGPIO();
    switchSetupGPIOTE();
    switchSetupTimers();

    flashSetup(false);
    flashLoadColorHSV(&gCtx.color);

    ledsSetupPWM();
    ledsSetupLED1Timer();
    ledsSetLED2StateHSV(gCtx.color);

    bleStackSetup();
    bleServiceSetup();
    bleServiceAttrHSVSetup(&gCtx.color);
    bleServiceAttrHSVNotify();
    bleServiceAttrInputSetup(NULL);

    while(true)
    {
        if(!NRF_LOG_PROCESS())
            nrf_pwr_mgmt_run();
        LOG_BACKEND_USB_PROCESS();

        Event event = queueEventDequeue();
        switch(event.type)
        {
        case EventSwitchPressed:
            switch(event.data.num)
            {
            case 1:
                flashSetup(true);
                break;

            case 2:
                switchMode(&gCtx);
                updateState(&gCtx);
                break;

            default:
                break;
            }
            break;

        case EventSwitchPressedContinuous:
            app_timer_start(gTimerColorMod, APP_TIMER_TICKS(COLOR_MOD_PERIOD_MS), &gCtx);
            break;

        case EventSwitchReleased:
            app_timer_stop(gTimerColorMod);
            break;

        case EventChangeColorRGB:
            gCtx.color = rgb2hsv(event.data.rgb);
            ledsSetLED2StateHSV(gCtx.color);
            bleServiceAttrHSVNotify();
            break;

        case EventChangeColorHSV:
            gCtx.color = event.data.hsv;
            ledsSetLED2StateHSV(gCtx.color);
            bleServiceAttrHSVNotify();
            break;

        default:
            break;
        }
    }
}
