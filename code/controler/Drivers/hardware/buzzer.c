#include "buzzer.h"

// private start------------------
/**
 * @brief  调节蜂鸣器频率与音量
 * @param  freq: 目标频率 (Hz)
 * @param  volume: 音量大小 (0-100), 50时声音最大（占空比50%）
 */
static void buzzer_set_param(uint32_t freq, uint8_t volume)
{
    if (freq == 0 || volume == 0)
    {
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, 0);
        return;
    }

    // 1. 计算预分频系数 PSC
    // 公式: PSC = 8,000,000 / (freq * 100) - 1
    uint32_t psc = (8000000 / (freq * 100)) - 1;

    // 限制 PSC 范围防止溢出 (uint16_t)
    if (psc > 65535)
        psc = 65535;

    // 2. 计算比较寄存器值 CCR (实现占空比控制音量)
    // 蜂鸣器最响时占空比为50%，即 CCR = ARR / 2 = 50
    // 我们将传入的 volume(0-100) 映射到 0-50 的占空比
    uint32_t ccr = (volume > 100) ? 50 : (volume / 2);

    // 3. 写入寄存器
    __HAL_TIM_SET_PRESCALER(&htim1, psc);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, ccr);

    /* 注意：由于修改了 PSC，可能需要触发一个更新事件使设置立即生效。
       但对于蜂鸣器这种非精密定时应用，不刷更新寄存器通常也能接受。
    */
}
// private end------------------

/**
 * @brief  蜂鸣器初始化
 */
void buzzer_init()
{
    buzzer_on();
}

/**
 * @brief  开启蜂鸣器
 */
void buzzer_on()
{
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);
}
/**
 * @brief  关闭蜂鸣器
 */
void buzzer_off()
{
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_4);
}
/**
 * @brief  蜂鸣器处理主函数
 */
void buzzer_process()
{
    if (controler_buzzer_param.status==ENABLE)
    {
        buzzer_set_param(controler_buzzer_param.set_freq, controler_buzzer_param.set_volume);
    }
}
