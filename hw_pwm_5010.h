#ifndef __hw_pwm_5010_include__
#define __hw_pwm_5010_include__

#include "lstimer.h"

typedef struct hw_pwm_s hw_pwm_t;

typedef void (*hw_pwm_callback)(hw_pwm_t *obj);

typedef struct hw_pwm_s {
    TIM_HandleTypeDef handler;
    uint8_t timer_id;
    uint8_t channel_pin[4];
    hw_pwm_callback callback;
} hw_pwm_t;

uint8_t hw_pwm_init(hw_pwm_t *obj,uint8_t id,uint32_t accuracy,uint32_t frequency);
void hw_pwm_deinit(hw_pwm_t *obj);

uint8_t hw_pwm_attach_channel(hw_pwm_t *obj,uint8_t id,uint8_t pin,uint32_t val);
uint8_t hw_pwm_detach_channel(hw_pwm_t *obj,uint8_t id);

uint8_t hw_pwm_start(hw_pwm_t *obj,uint8_t id);
uint8_t hw_pwm_stop(hw_pwm_t *obj,uint8_t id);

uint8_t hw_pwm_start_isr(hw_pwm_t *obj,uint8_t id);
uint8_t hw_pwm_stop_isr(hw_pwm_t *obj,uint8_t id);

void hw_pwm_set(hw_pwm_t *obj,uint8_t id,uint32_t val);

#define hw_pwm_AR(a,b)  ((hw_pwm_t *)(a))->handler.Instance->ARR = (b)
#define hw_pwm_PS(a,b)  ((hw_pwm_t *)(a))->handler.Instance->PSC = (b)
#define hw_pwm_HZ(obj,maxv,hz)  (((hw_pwm_t *)(obj))->handler.Instance->PSC = (((SDK_HCLK_MHZ * 1000000) / (maxv * hz)) - 1),((hw_pwm_t *)(obj))->handler.Instance->ARR = ((maxv) - 1))

void hw_pwm_attach_callback(hw_pwm_t *obj, hw_pwm_callback callback);

#endif
