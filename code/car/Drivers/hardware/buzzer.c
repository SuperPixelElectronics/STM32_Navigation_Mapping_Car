#include "buzzer.h"

/**
 * @brief 初始化蜂鸣器
 */
void buzzer_init(){
    car_buzzer_param.status = DISABLE;
    car_buzzer_param.set_freq = 440;
}

/**
 * @brief 蜂鸣器处理函数
 */
void buzzer_process()
{
    // 计算所需的翻转周期（毫秒）
    // 注意：一个完整周期需要两次翻转（高->低->高）
    car_buzzer_param.set_delay = 1000 / car_buzzer_param.set_freq;


    car_buzzer_param.time = HAL_GetTick();
    car_buzzer_param.real_delay = car_buzzer_param.time - car_buzzer_param.previous_time;

    if (car_buzzer_param.real_delay >= car_buzzer_param.set_delay)
    {
        if (car_buzzer_param.status==ENABLE)
        {
            HAL_GPIO_TogglePin(BUZZER_GPIO_Port, BUZZER_Pin);
        }else{
            HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);
        }

        car_buzzer_param.previous_time = car_buzzer_param.time;   // 只在翻转时更新时间戳
        car_buzzer_param.real_freq = 1000 / car_buzzer_param.real_delay;
    }
}
