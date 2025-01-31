#include "nrf_gpio.h"
#include "nrfx_gpiote.h"
#include "app_timer.h"

#include "switch.h"
#include "common.h"
#include "queue.h"

#define SW1_PRT 1
#define SW1_PIN 6

#define LISTENER_DURATION_MS 400
#define DEBOUNCE_DURATION_MS 100

APP_TIMER_DEF(gTimerListener);

APP_TIMER_DEF(gTimerDebounce);

static uint8_t gConfirmedPresses = 0;
static bool    gConfirmedPContin = false;

static const nrfx_gpiote_in_config_t gSwitchGPIOTEConfig =
{
    .sense           = NRF_GPIOTE_POLARITY_TOGGLE,
    .pull            = NRF_GPIO_PIN_PULLUP,
    .is_watcher      = false,
    .hi_accuracy     = true,
    .skip_gpio_setup = false
};

void switchSetupGPIO(void)
{
    nrf_gpio_cfg_input(NRF_GPIO_PIN_MAP(SW1_PRT, SW1_PIN), NRF_GPIO_PIN_PULLUP);
}

static LogicalState switchGetSwitchState(void)
{
    return nrf_gpio_pin_read(NRF_GPIO_PIN_MAP(SW1_PRT, SW1_PIN)) == 0 ? LogicalStateOn : LogicalStateOff;
}

static void switchHandlerSW1(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    switch(action)
    {
    case NRF_GPIOTE_POLARITY_TOGGLE:
        app_timer_start(gTimerListener, APP_TIMER_TICKS(LISTENER_DURATION_MS), NULL);
        app_timer_start(gTimerDebounce, APP_TIMER_TICKS(DEBOUNCE_DURATION_MS), NULL);
        break;

    default:
        break;
    }
}

void switchSetupGPIOTE(void)
{
    nrfx_gpiote_in_init(NRF_GPIO_PIN_MAP(SW1_PRT, SW1_PIN), &gSwitchGPIOTEConfig, switchHandlerSW1);
    nrfx_gpiote_in_event_enable(NRF_GPIO_PIN_MAP(SW1_PRT, SW1_PIN), true);
}

static void switchHandlerListener(void* p_context)
{
    if(gConfirmedPresses > 0)
    {
        if(switchGetSwitchState() == LogicalStateOn)
        {
            gConfirmedPContin = true;
            queueEventEnqueue((Event){EventSwitchPressedContinuous});
        }
        else
            queueEventEnqueue((Event){EventSwitchPressed, {.num = gConfirmedPresses}});
        gConfirmedPresses = 0;
    }
}

static void switchHandlerDebounce(void* p_context)
{
    if(switchGetSwitchState() == LogicalStateOn)
        ++gConfirmedPresses;
    else if(gConfirmedPContin)
    {
        gConfirmedPContin = false;
        queueEventEnqueue((Event){EventSwitchReleased});
    }
}

void switchSetupTimers(void)
{
    app_timer_create(&gTimerListener, APP_TIMER_MODE_SINGLE_SHOT, switchHandlerListener);
    app_timer_create(&gTimerDebounce, APP_TIMER_MODE_SINGLE_SHOT, switchHandlerDebounce);
}
