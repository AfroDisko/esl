#include <math.h>

#include "app_timer.h"
#include "nrf_gpio.h"
#include "nrfx_pwm.h"

#include "leds.h"

#define LED1_G_PRT 0
#define LED1_G_PIN 6

#define LED2_R_PRT 0
#define LED2_R_PIN 8

#define LED2_G_PRT 1
#define LED2_G_PIN 9

#define LED2_B_PRT 0
#define LED2_B_PIN 12

#define LED1_FLASH_PERIOD_DFLT_MS 1000
#define LED1_FLASH_PERIOD_SLOW_MS 2000
#define LED1_FLASH_PERIOD_FAST_MS 500

#define LED1_SHIFT_PERIOD_MS 1

APP_TIMER_DEF(gTimerLED1Shift);

static uint16_t gLED1FlashPeriod = LED1_FLASH_PERIOD_DFLT_MS;
static uint64_t gLED1ShiftTick   = 0;

static uint8_t  gLED1State = 0;
static ColorRGB gLED2State =
{
    .r = 0,
    .g = 0,
    .b = 0
};

static nrf_pwm_values_individual_t gPWMSeqValues = 
{
    .channel_0 = 0,
    .channel_1 = 0,
    .channel_2 = 0,
    .channel_3 = 0
};
static const nrf_pwm_sequence_t gPWMSeq =
{
    .values.p_individual = &gPWMSeqValues,
    .length              = NRF_PWM_VALUES_LENGTH(gPWMSeqValues),
    .repeats             = 0,
    .end_delay           = 0
};

// gPWMTopValue = 1020 so that PWM period = 1020/1e6 ~ 1ms, and 1020/UINT8_MAX is integer
static const uint16_t          gPWMTopValue = 1020;
static const nrfx_pwm_t        gPWMInstance = NRFX_PWM_INSTANCE(0);
static const nrfx_pwm_config_t gPWMConfig   =
{
    .output_pins =
    {
        NRF_GPIO_PIN_MAP(LED1_G_PRT, LED1_G_PIN) | NRFX_PWM_PIN_INVERTED,
        NRF_GPIO_PIN_MAP(LED2_R_PRT, LED2_R_PIN) | NRFX_PWM_PIN_INVERTED,
        NRF_GPIO_PIN_MAP(LED2_G_PRT, LED2_G_PIN) | NRFX_PWM_PIN_INVERTED,
        NRF_GPIO_PIN_MAP(LED2_B_PRT, LED2_B_PIN) | NRFX_PWM_PIN_INVERTED
    },
    .irq_priority = APP_IRQ_PRIORITY_LOWEST,
    .base_clock   = NRF_PWM_CLK_1MHz,
    .count_mode   = NRF_PWM_MODE_UP_AND_DOWN,
    .top_value    = gPWMTopValue,
    .load_mode    = NRF_PWM_LOAD_INDIVIDUAL,
    .step_mode    = NRF_PWM_STEP_AUTO
};

static uint32_t ledsColor2Pin(char color)
{
    switch(color)
    {
    case 'g':
        return NRF_GPIO_PIN_MAP(LED1_G_PRT, LED1_G_PIN);

    case 'R':
        return NRF_GPIO_PIN_MAP(LED2_R_PRT, LED2_R_PIN);

    case 'G':
        return NRF_GPIO_PIN_MAP(LED2_G_PRT, LED2_G_PIN);

    case 'B':
        return NRF_GPIO_PIN_MAP(LED2_B_PRT, LED2_B_PIN);

    default:
        return 0;
    }
}

static void ledsResetPins(void)
{
    nrf_gpio_pin_write(NRF_GPIO_PIN_MAP(LED1_G_PRT, LED1_G_PIN), 1);
    nrf_gpio_pin_write(NRF_GPIO_PIN_MAP(LED2_R_PRT, LED2_R_PIN), 1);
    nrf_gpio_pin_write(NRF_GPIO_PIN_MAP(LED2_G_PRT, LED2_G_PIN), 1);
    nrf_gpio_pin_write(NRF_GPIO_PIN_MAP(LED2_B_PRT, LED2_B_PIN), 1);
}

void ledsSetupGPIO(void)
{
    nrf_gpio_cfg_output(NRF_GPIO_PIN_MAP(LED1_G_PRT, LED1_G_PIN));
    nrf_gpio_cfg_output(NRF_GPIO_PIN_MAP(LED2_R_PRT, LED2_R_PIN));
    nrf_gpio_cfg_output(NRF_GPIO_PIN_MAP(LED2_G_PRT, LED2_G_PIN));
    nrf_gpio_cfg_output(NRF_GPIO_PIN_MAP(LED2_B_PRT, LED2_B_PIN));
    ledsResetPins();
}

void ledsSetLEDState(char color, LogicalState state)
{
    nrf_gpio_pin_write(ledsColor2Pin(color), state == LogicalStateOn ? 0 : 1);
}

LogicalState ledsGetLEDState(char color)
{
    return nrf_gpio_pin_out_read(ledsColor2Pin(color)) == 0 ? LogicalStateOn : LogicalStateOff;
}

void ledsSetupPWM(void)
{
    nrfx_pwm_init(&gPWMInstance, &gPWMConfig, NULL);
    nrfx_pwm_simple_playback(&gPWMInstance, &gPWMSeq, 1, NRFX_PWM_FLAG_LOOP);
}

ColorRGB ledsGetLED2State(void)
{
    return gLED2State;
}

static void ledsUpdatePWMSeqValuesLED1(void)
{
    gPWMSeqValues.channel_0 = gPWMTopValue * gLED1State / UINT8_MAX;
}

static void ledsUpdatePWMSeqValuesLED2(void)
{
    gPWMSeqValues.channel_1 = gPWMTopValue * gLED2State.r / UINT8_MAX;
    gPWMSeqValues.channel_2 = gPWMTopValue * gLED2State.g / UINT8_MAX;
    gPWMSeqValues.channel_3 = gPWMTopValue * gLED2State.b / UINT8_MAX;
}

void ledsSetLED1State(uint8_t state)
{
    gLED1State = state;
    ledsUpdatePWMSeqValuesLED1();
}

void ledsSetLED2StateRGB(ColorRGB rgb)
{
    gLED2State = rgb;
    ledsUpdatePWMSeqValuesLED2();
}

void ledsSetLED2StateHSV(ColorHSV hsv)
{
    gLED2State = hsv2rgb(hsv);
    ledsUpdatePWMSeqValuesLED2();
}

static void ledsShiftLED1State(void)
{
    // cos() is used for automatic periodic behavior
    gLED1State = UINT8_MAX * (0.5 - 0.5 * cos(2 * M_PI * gLED1ShiftTick++ * LED1_SHIFT_PERIOD_MS / gLED1FlashPeriod));
    ledsUpdatePWMSeqValuesLED1();
}

static void ledsHandlerLED1Shift(void* p_context)
{
    ledsShiftLED1State();
}

void ledsSetupLED1Timer(void)
{
    app_timer_create(&gTimerLED1Shift, APP_TIMER_MODE_REPEATED, ledsHandlerLED1Shift);
}

void ledsFlashLED1(FlashMode mode)
{
    gLED1ShiftTick = 0;
    switch(mode)
    {
    case FlashModeSlow:
        gLED1FlashPeriod = LED1_FLASH_PERIOD_SLOW_MS;
        break;

    case FlashModeFast:
        gLED1FlashPeriod = LED1_FLASH_PERIOD_FAST_MS;
        break;

    default:
        break;
    }
    app_timer_start(gTimerLED1Shift, APP_TIMER_TICKS(LED1_SHIFT_PERIOD_MS), NULL);
}

void ledsFlashLED1Halt(void)
{
    app_timer_stop(gTimerLED1Shift);
}
