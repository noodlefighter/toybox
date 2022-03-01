#include <app_config.h>
#include <stddef.h>
#include "hw_pwm_5010.h"
#include "io_config.h"

#define NOPIN 0xFF

static hw_pwm_t *get_pwm_obj(TIM_HandleTypeDef *handler)
{
    hw_pwm_t *obj;
    obj = (hw_pwm_t *)((char *)handler - offsetof(hw_pwm_t, handler));
    return (obj);
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    hw_pwm_t *obj = get_pwm_obj(htim);

    if(NULL != obj->callback)
        obj->callback(obj);
}

void hw_pwm_attach_callback(hw_pwm_t *obj, hw_pwm_callback callback)
{
    obj->callback = callback;
}

//设置pwm timer
//      id = timer id
//      accuracy = 精度，比如 200,256,1000 ..
//      frequency = PWM频率 1000=1Khz 2000=2Khz ...
//
uint8_t hw_pwm_init(hw_pwm_t *obj,uint8_t id,uint32_t accuracy,uint32_t frequency)
{
    reg_timer_t *timer_map[] = {LSBSTIM,LSGPTIMA,LSGPTIMB,LSGPTIMC,LSADTIM};

    uint32_t prescaler = (SDK_HCLK_MHZ * 1000000) / (accuracy * frequency) - 1;
    uint32_t period = accuracy - 1;

    if(id == 0 || id >= sizeof(timer_map) / sizeof(timer_map[0]))
        return -1;

    obj->timer_id                   = id;

    obj->handler.Instance           = timer_map[id];

    obj->handler.Init.Prescaler     = prescaler;
    obj->handler.Init.Period        = period;

    obj->handler.Init.ClockDivision = 0;
    obj->handler.Init.CounterMode   = TIM_COUNTERMODE_UP;
    obj->handler.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    obj->channel_pin[0] = NOPIN;
    obj->channel_pin[1] = NOPIN;
    obj->channel_pin[2] = NOPIN;
    obj->channel_pin[3] = NOPIN;

    HAL_TIM_Init(&obj->handler);

    return 0;
}

void hw_pwm_deinit(hw_pwm_t *obj)
{
    uint8_t id;

    id = 0; if(NOPIN != obj->channel_pin[id]) { hw_pwm_stop(obj,id + 1); hw_pwm_detach_channel(obj,id + 1); }
    id = 1; if(NOPIN != obj->channel_pin[id]) { hw_pwm_stop(obj,id + 1); hw_pwm_detach_channel(obj,id + 1); }
    id = 2; if(NOPIN != obj->channel_pin[id]) { hw_pwm_stop(obj,id + 1); hw_pwm_detach_channel(obj,id + 1); }
    id = 3; if(NOPIN != obj->channel_pin[id]) { hw_pwm_stop(obj,id + 1); hw_pwm_detach_channel(obj,id + 1); }

    HAL_TIM_DeInit(&obj->handler);
}

typedef void (*hw_pwm_io_init)(uint8_t pin,bool output,uint8_t default_val);

uint8_t hw_pwm_attach_channel(hw_pwm_t *obj,uint8_t id,uint8_t pin,uint32_t val)
{
    uint8_t channel_map[] = { 0,TIM_CHANNEL_1,TIM_CHANNEL_2,TIM_CHANNEL_3,TIM_CHANNEL_4 };

    hw_pwm_io_init channel_io_init[4][4] = {
        { gptima1_ch1_io_init,gptima1_ch2_io_init,gptima1_ch3_io_init,gptima1_ch4_io_init },
        { gptimb1_ch1_io_init,gptimb1_ch2_io_init,gptimb1_ch3_io_init,gptimb1_ch4_io_init },
        { gptimc1_ch1_io_init,gptimc1_ch2_io_init,NULL,NULL },
        { adtim1_ch1_io_init,adtim1_ch2_io_init,adtim1_ch3_io_init,adtim1_ch4_io_init }
    };

    if(id == 0 || id >= sizeof(channel_map) / sizeof(channel_map[0]))
        return -1;

    hw_pwm_io_init pwm_io_init_func = (channel_io_init[obj->timer_id - 1][id - 1]);

    if(NULL == pwm_io_init_func)
        return -2;

    obj->channel_pin[id - 1] = pin;
    pwm_io_init_func(pin,true,0);
    
    // gptimb1_ch2_io_init(PA01, true, 0);

    TIM_OC_InitTypeDef pwm_cfg = {0};

    pwm_cfg.OCMode = TIM_OCMODE_PWM1;
    pwm_cfg.OCPolarity = TIM_OCPOLARITY_HIGH;
    pwm_cfg.OCFastMode = TIM_OCFAST_DISABLE;

    pwm_cfg.Pulse = val;

    HAL_TIM_PWM_ConfigChannel(&obj->handler, &pwm_cfg, channel_map[id]);

    return 0;
}

typedef void (*hw_pwm_io_deinit)(void);

uint8_t hw_pwm_detach_channel(hw_pwm_t *obj,uint8_t id)
{
    uint8_t channel_map[] = { 0,TIM_CHANNEL_1,TIM_CHANNEL_2,TIM_CHANNEL_3,TIM_CHANNEL_4 };

    hw_pwm_io_deinit channel_io_deinit[4][4] = {
        { gptima1_ch1_io_deinit,gptima1_ch2_io_deinit,gptima1_ch3_io_deinit,gptima1_ch4_io_deinit },
        { gptimb1_ch1_io_deinit,gptimb1_ch2_io_deinit,gptimb1_ch3_io_deinit,gptimb1_ch4_io_deinit },
        { gptimc1_ch1_io_deinit,gptimc1_ch2_io_deinit,NULL,NULL },
        { adtim1_ch1_io_deinit,adtim1_ch2_io_deinit,adtim1_ch3_io_deinit,adtim1_ch4_io_deinit }
    };

    if(id == 0 || id >= sizeof(channel_map) / sizeof(channel_map[0]))
        return -1;

    hw_pwm_io_deinit pwm_io_deinit_func = (channel_io_deinit[obj->timer_id - 1][id - 1]);

    if(NULL == pwm_io_deinit_func)
        return -2;

    if(NOPIN == obj->channel_pin[id - 1])
        return -3;
    
    obj->channel_pin[id - 1] = NOPIN;

    pwm_io_deinit_func();
    return 0;
}

int32_t id_to_ch(hw_pwm_t *obj,uint8_t id)
{
    uint8_t channel_map[] = { 0,TIM_CHANNEL_1,TIM_CHANNEL_2,TIM_CHANNEL_3,TIM_CHANNEL_4 };

    if(id == 0 || id >= sizeof(channel_map) / sizeof(channel_map[0]))
        return -1;

    if(NOPIN == obj->channel_pin[id - 1])
        return -3;

    return channel_map[id];
}

uint8_t hw_pwm_start(hw_pwm_t *obj,uint8_t id)
{    
    int32_t ch = id_to_ch(obj,id);
    if(ch < 0)
        return ch;

    HAL_TIM_PWM_Start(&obj->handler, ch);
    return 0;
}

uint8_t hw_pwm_stop(hw_pwm_t *obj,uint8_t id)
{
    int32_t ch = id_to_ch(obj,id);
    if(ch < 0)
        return ch;

    HAL_TIM_PWM_Stop(&obj->handler, ch);
    return 0;
}

uint8_t hw_pwm_start_isr(hw_pwm_t *obj,uint8_t id)
{
    int32_t ch = id_to_ch(obj,id);
    if(ch < 0)
        return ch;

    HAL_TIM_PWM_Start_IT(&obj->handler, ch);
    return 0;
}

uint8_t hw_pwm_stop_isr(hw_pwm_t *obj,uint8_t id)
{
    int32_t ch = id_to_ch(obj,id);
    if(ch < 0)
        return ch;

    HAL_TIM_PWM_Stop_IT(&obj->handler, ch);
    return 0;
}

// uint8_t hw_pwm_set(hw_pwm_t *obj,uint8_t id,uint32_t val)
// {
//     switch(id)
//     {
//         case 1:
//             obj->handler.Instance->CCR1 = val;
//         break;

//         case 2:
//             obj->handler.Instance->CCR2 = val;
//         break;

//         case 3:
//             obj->handler.Instance->CCR3 = val;
//         break;

//         case 4:
//             obj->handler.Instance->CCR4 = val;
//         break;

//         default:
//             return -1;
//         break;
//     }

//     return 0;
// }

void hw_pwm_set(hw_pwm_t *obj,uint8_t id,uint32_t val)
{
    ((uint32_t *)(&(obj->handler.Instance->CCR1)))[id - 1] = val;
}
