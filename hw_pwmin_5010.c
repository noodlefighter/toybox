#include "hw_pwmin_5010.h"
#include "io_config.h"
#include "platform.h"

#include "lsuart.h"
#include <stdio.h>
#include <string.h>

#define ARRAY_LENGTH(array) (sizeof(array) / sizeof(array[0]))

#define TIM_FREQ          (1*1000*1000)
#define TIM_PRESCALER     (SDK_HCLK_MHZ-1)

typedef void (*hw_pwm_io_init)(uint8_t pin,bool output,uint8_t default_val);
typedef void (*hw_pwm_io_deinit)(void);
struct timdesc_s {
    reg_timer_t *reg_timer;
    hw_pwm_io_init ch_io_init[4];
    hw_pwm_io_deinit ch_io_deinit[4];
};

static struct timdesc_s timdesc_tb[] =
{
    {
        LSBSTIM
    },
    {
        LSGPTIMA,
        { gptima1_ch1_io_init,gptima1_ch2_io_init,gptima1_ch3_io_init,gptima1_ch4_io_init },
        { gptima1_ch1_io_deinit,gptima1_ch2_io_deinit,gptima1_ch3_io_deinit,gptima1_ch4_io_deinit }
    },
    {
        LSGPTIMB,
        { gptimb1_ch1_io_init,gptimb1_ch2_io_init,gptimb1_ch3_io_init,gptimb1_ch4_io_init },
        { gptimb1_ch1_io_deinit,gptimb1_ch2_io_deinit,gptimb1_ch3_io_deinit,gptimb1_ch4_io_deinit },
    },
    {
        LSGPTIMC,
        { gptimc1_ch1_io_init,gptimc1_ch2_io_init,NULL,NULL },
        { gptimc1_ch1_io_deinit,gptimc1_ch2_io_deinit,NULL,NULL },
    },
    {
        LSADTIM,
        { adtim1_ch1_io_init,adtim1_ch2_io_init,adtim1_ch3_io_init,adtim1_ch4_io_init },
        { adtim1_ch1_io_deinit,adtim1_ch2_io_deinit,adtim1_ch3_io_deinit,adtim1_ch4_io_deinit }
    }
};


static hw_pwmin_t *get_pwmin_obj(TIM_HandleTypeDef *handler)
{
    hw_pwmin_t *obj;
    obj = (hw_pwmin_t *)((char *)handler - offsetof(hw_pwmin_t, handler));
    return (obj);
}

int hw_pwmin_init(hw_pwmin_t *obj, uint8_t timer_id, uint8_t pin, int freq_min, int acc)
{
    TIM_IC_InitTypeDef ICConfig = {0};

    if (timer_id >= ARRAY_LENGTH(timdesc_tb))
        return -1;

    obj->timer_id = timer_id;
    obj->pin = pin;
    obj->acc = acc;

    // init Channel 1 IO
    if (NULL == timdesc_tb[timer_id].ch_io_init[TIM_CHANNEL_1]) {
        return -1;
    }
    timdesc_tb[timer_id].ch_io_init[TIM_CHANNEL_1](pin, false, 0);
    io_pull_write(pin, IO_PULL_DOWN);

    uint16_t tim_prescaler, tim_period;
    if (acc > UINT16_MAX) {
        return -1;
    }
    tim_period = acc - 1;
    int ts_max_us = 1000*1000/freq_min;
    int timclk_us = ts_max_us/acc;
    int ratio = timclk_us*SDK_HCLK_MHZ; // ratio=timclk/(1/sysclk_freq)
    tim_prescaler = (uint16_t)(ratio - 1);
    obj->timer_freq = (int)(SDK_HCLK_MHZ*1000*1000/ratio);

    obj->handler.Instance = timdesc_tb[timer_id].reg_timer;
    obj->handler.Init.Prescaler = tim_prescaler;
    obj->handler.Init.Period = tim_period;
    obj->handler.Init.ClockDivision = 0;
    obj->handler.Init.CounterMode = TIM_COUNTERMODE_UP;
    obj->handler.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_Init(&obj->handler);

    ICConfig.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
    ICConfig.ICSelection = TIM_ICSELECTION_DIRECTTI; // TI1 connect to IC1
    ICConfig.ICPrescaler = TIM_ICPSC_DIV1;
    ICConfig.ICFilter = 0;
    HAL_TIM_IC_ConfigChannel(&obj->handler, &ICConfig, TIM_CHANNEL_1);

    ICConfig.ICPolarity = TIM_INPUTCHANNELPOLARITY_FALLING;
    ICConfig.ICSelection = TIM_ICSELECTION_INDIRECTTI; // TI1 connect to IC2
    ICConfig.ICPrescaler = TIM_ICPSC_DIV1;
    ICConfig.ICFilter = 0;
    HAL_TIM_IC_ConfigChannel(&obj->handler, &ICConfig, TIM_CHANNEL_2);

    obj->handler.Instance->SMCR  = TIM_SLAVEMODE_RESET; // reset mode
    obj->handler.Instance->SMCR |= (TIM_TS_TI1FP1 << TIMER_SMCR_TS_POS); // select TI1FP1

    HAL_TIM_Base_Start_IT(&obj->handler);
    HAL_TIM_IC_Start(&obj->handler,TIM_CHANNEL_1);
    HAL_TIM_IC_Start(&obj->handler,TIM_CHANNEL_2);
    return 0;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    hw_pwmin_t *obj = get_pwmin_obj(htim);
    if (obj != NULL) {
        obj->overflow_cnt++;

        timdesc_tb[obj->timer_id].ch_io_deinit[TIM_CHANNEL_1]();
        io_cfg_input(obj->pin);
        obj->overflow_lv = io_read_pin(obj->pin);
        timdesc_tb[obj->timer_id].ch_io_init[TIM_CHANNEL_1](obj->pin, false, 0);
    }
}

void hw_pwmin_read(hw_pwmin_t *obj, int *out_freq, int *out_duty)
{
    int t1 = HAL_TIM_ReadCapturedValue(&obj->handler, TIM_CHANNEL_1) + 1;
    int t2 = HAL_TIM_ReadCapturedValue(&obj->handler, TIM_CHANNEL_2) + 1;
    int freq, duty;

    if (obj->overflow_cnt > 0) { // 频率过低检测, 时钟频率1MHz, 可测最宽65535us周期即最低15Hz
        freq = 0;
        duty = obj->overflow_lv ? obj->acc : 0;
    }
    else if ((t2 > t1) || // 高频率向低频率跳变时, 可能t2>t1
        ((t1 == obj->last_t1) && (t2 == obj->last_t2))) { // 测量未完成时

        // 保持
        freq = obj->last_freq;
        duty = obj->last_duty;
    }
    else {
        freq = obj->timer_freq / t1;
        duty = t2 * obj->acc / t1;
    }

    // {
    //     extern UART_HandleTypeDef UART_Config;
    //     uint8_t txbuf[100];
    //     int txlen = snprintf((char*)txbuf, sizeof(txbuf),
    //         "t=%d t1=%d t2=%d freq=%d duty=%d overflow=%d\r\n", obj->handler.Instance->CNT, t1, t2, freq, duty, obj->overflow_cnt);
    //     HAL_UART_Transmit(&UART_Config, txbuf, txlen, 1000);
    // }

    obj->last_t1 = t1;
    obj->last_t2 = t2;
    obj->last_freq = freq;
    obj->last_duty = duty;
    obj->overflow_cnt = 0;

    *out_freq = freq;
    *out_duty = duty;
}
