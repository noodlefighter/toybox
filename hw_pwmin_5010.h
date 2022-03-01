#ifndef __hw_pwmin_5010_include__
#define __hw_pwmin_5010_include__

#include "lstimer.h"

typedef struct hw_pwmin_s {
    TIM_HandleTypeDef handler;
    uint8_t timer_id;
    uint8_t pin;
    int acc;
    int timer_freq;

    volatile int overflow_cnt;
    volatile int overflow_lv;

    int last_freq, last_duty;
    int last_t1, last_t2;
} hw_pwmin_t;


#ifdef __cplusplus
extern "C" {
#endif

// timer_id: 硬件定时器id, 支持捕获功能的定时器才能用于测量pwm
// freq_min: 输入PWM的最低频率, 单位为Hz
// acc: 占空比测量精度, 同时也是占空比读数的范围, 取值0x0000-0xFFFF
//      要注意的是acc为所测信号频率接近freq_min时的精度, 频率越高实际精度越低
//      例如freq_min=10Hz,acc=100时, 测量100Hz的PWM占空比, 的只有10的精度
int hw_pwmin_init(hw_pwmin_t *obj, uint8_t timer_id, uint8_t pin, int freq_min, int acc);
void hw_pwmin_deinit(hw_pwmin_t *obj);

void hw_pwmin_read(hw_pwmin_t *obj, int *out_freq, int *out_duty);



#ifdef __cplusplus
}
#endif

#endif
