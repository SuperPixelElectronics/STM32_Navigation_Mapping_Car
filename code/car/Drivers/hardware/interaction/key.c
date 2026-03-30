#include "key.h"

// private start------------------
/**
 * @brief 判断按键键值
 * @param adc_value 按键的ADC数值
 */
static int key_judge(int adc_value)
{
    if (abs(adc_value - KEY_UP_ADC_VAL) < KEY_THRESHOLD)
    {
        return KEY_UP;
    }
    else if (abs(adc_value - KEY_DOWN_ADC_VAL) < KEY_THRESHOLD)
    {
        return KEY_DOWN;
    }
    else if (abs(adc_value - KEY_LEFT_ADC_VAL) < KEY_THRESHOLD)
    {
        return KEY_LEFT;
    }
    else if (abs(adc_value - KEY_RIGHT_ADC_VAL) < KEY_THRESHOLD)
    {
        return KEY_RIGHT;
    }
    else if (abs(adc_value - KEY_CENTER_ADC_VAL) < KEY_THRESHOLD)
    {
        return KEY_CENTER;
    }
    else
    {
        return KEY_NO;
    }
}

/**
 * @brief 根据按键按下情况返回对应的按键信号
 * @return 返回的对应按键信号
 */
static KeySignalType key_signal_process()
{
    // key up!------------------------------------------------------------------------------------------------------------------
    if (car_key[KEY_UP].short_pressed == KEY_ENABLED)
    {
        car_key[KEY_UP].short_pressed = KEY_DISABLED;
        return KEY_UP_SHORT;
    }
    // key down!------------------------------------------------------------------------------------------------------------------
    else if (car_key[KEY_DOWN].short_pressed == KEY_ENABLED)
    {
        car_key[KEY_DOWN].short_pressed = KEY_DISABLED;
        return KEY_DOWN_SHORT;
    }
    // key left!------------------------------------------------------------------------------------------------------------------
    else if (car_key[KEY_LEFT].short_pressed == KEY_ENABLED)
    {
        car_key[KEY_LEFT].short_pressed = KEY_DISABLED;
        return KEY_LEFT_SHORT;
    }
    // key right!------------------------------------------------------------------------------------------------------------------
    else if (car_key[KEY_RIGHT].short_pressed == KEY_ENABLED)
    {
        car_key[KEY_RIGHT].short_pressed = KEY_DISABLED;
        return KEY_RIGHT_SHORT;
    }
    // key center short!------------------------------------------------------------------------------------------------------------------
    else if (car_key[KEY_CENTER].short_pressed == KEY_ENABLED)
    {
        car_key[KEY_CENTER].short_pressed = KEY_DISABLED;
        return KEY_CENTER_SHORT;
    }
    // key center long!------------------------------------------------------------------------------------------------------------------
    else if (car_key[KEY_CENTER].long_pressed == KEY_ENABLED)
    {
        car_key[KEY_CENTER].long_pressed = KEY_DISABLED;
        return KEY_CENTER_LONG;
    }
    else if (car_key[KEY_UP].long_pressed == KEY_ENABLED)
    {
        car_key[KEY_UP].long_pressed = KEY_DISABLED;
        return KEY_UP_LONG;
    }
    else if (car_key[KEY_DOWN].long_pressed == KEY_ENABLED)
    {
        car_key[KEY_DOWN].long_pressed = KEY_DISABLED;
        return KEY_DOWN_LONG;
    }
    else if (car_key[KEY_LEFT].long_pressed == KEY_ENABLED)
    {
        car_key[KEY_LEFT].long_pressed = KEY_DISABLED;
        return KEY_LEFT_LONG;
    }
    else if (car_key[KEY_RIGHT].long_pressed == KEY_ENABLED)
    {
        car_key[KEY_RIGHT].long_pressed = KEY_DISABLED;
        return KEY_RIGHT_LONG;
    }
    else
    {
        return KEY_NO_HAPPEN;
    }
}
// private end------------------

/**
 * @brief 初始化按键
 */
void key_init()
{
    HAL_ADC_Start(&hadc1);
    for (int i = 0; i < 5; i++)
    {
        car_key[i].short_pressed = KEY_DISABLED;
        car_key[i].long_pressed = KEY_DISABLED;
        car_key[i].state = KEY_RELEASING;
    }
}

/**
 * @brief 扫描按键，使用系统时钟方法
 * @note 由于不同页面的刷新速度不一样，当页面卡顿时，用户的短按无法及时反馈，很容易被误触成长按
 */
// void key_read()
// {
//     HAL_ADC_Start(&hadc1);
//     key_adc_val = HAL_ADC_GetValue(&hadc1);
//     int key_now = key_judge(key_adc_val);
//     uint32_t now_tick = HAL_GetTick(); // 获取当前系统绝对时间
//     for (int i = 0; i < 5; i++)
//     {
//         if (car_key[i].state == KEY_RELEASING)
//         {
//             if (key_now == i)
//             {
//                 // 1. 去抖动逻辑：改为非阻塞判断
//                 // 如果这是刚检测到按下，记录开始时间
//                 if (car_key[i].start_tick == 0)
//                     car_key[i].start_tick = now_tick;
//                 // 持续按下超过 20ms 判定为真按下
//                 if (now_tick - car_key[i].start_tick > 20)
//                 {
//                     car_key[i].state = KEY_PRESSING;
//                     // 这里记录的是正式开始计时的时刻
//                     car_key[i].press_start_tick = now_tick;
//                 }
//             }
//             else
//             {
//                 car_key[i].start_tick = 0; // 没按够时间就松开了，重置去抖计时
//             }
//         }
//         else if (car_key[i].state == KEY_PRESSING)
//         {
//             // 2. 实时计算差值
//             car_key[i].press_time = now_tick - car_key[i].press_start_tick;
//             if (key_now != i) // 按键松开
//             {
//                 if (car_key[i].press_time > 500) // 硬性规定：超过 500ms 算长按
//                 {
//                     car_key[i].long_pressed = KEY_ENABLED;
//                 }
//                 else if (car_key[i].press_time > 50) // 50ms-500ms 算短按
//                 {
//                     car_key[i].short_pressed = KEY_ENABLED;
//                 }
//                 car_key[i].state = KEY_RELEASING;
//                 car_key[i].start_tick = 0;
//                 car_key[i].press_start_tick = 0;
//                 car_key[i].press_time = 0;
//             }
//         }
//     }
// }

/**
 * @brief 扫描按键，使用main tick方法
 */
void key_read()
{
    HAL_ADC_Start(&hadc1);
    key_adc_val = HAL_ADC_GetValue(&hadc1);
    int key_now = key_judge(key_adc_val);
    for (int i = 0; i < 5; i++)
    {
        if (car_key[i].state == KEY_RELEASING)
        {
            if (key_now == i)
            {
                HAL_Delay(10);
                if (key_now == i)
                {
                    car_key[i].state = KEY_PRESSING;
                }
            }
        }
        else if (car_key[i].state == KEY_PRESSING)
        {
            car_key[i].press_time++;
            if (key_now != i)
            {
                if (car_key[i].press_time > KEY_LONG_CNT)
                {
                    car_key[i].state = KEY_RELEASING;
                    car_key[i].long_pressed = KEY_ENABLED;
                    car_key[i].press_time = 0;
                }
                else
                {
                    car_key[i].state = KEY_RELEASING;
                    car_key[i].short_pressed = KEY_ENABLED;
                    car_key[i].press_time = 0;
                }
            }
        }
    }
}

/**
 * @brief 按键键值处理函数
 * @note 在这里实现具体的按键功能
 */
void key_process()
{
    switch (oled_page)
    {
    case MENU_PAGE:
        menu_page_handler(key_signal_process());
        break;
    case ENCODER_DATA_PAGE:
        encoder_data_page_handler(key_signal_process());
        break;
    case ENCODER_GRAPH_PAGE:
        encoder_graph_page_handler(key_signal_process());
        break;
    case MPU6050_DATA_PAGE:
        mpu6050_data_page_handler(key_signal_process());
        break;
    case MPU6050_GRAPH_PAGE:
        mpu6050_graph_page_handler(key_signal_process());
        break;
    case COMBINED_DATA_PAGE:
        combined_data_page_handler(key_signal_process());
        break;
    case COMBINED_GRAPH_PAGE:
        combined_graph_page_handler(key_signal_process());
        break;
    case MOTOR_CONTROL_PAGE:
        motor_control_page_handler(key_signal_process());
        break;
    case NRF_PAGE:
        nrf_page_handler(key_signal_process());
        break;
    case LED_BUZZER_SYS_PAGE:
        led_buzzer_sys_page_handler(key_signal_process());
        break;
    case TRACK_DATA_PAGE:
        track_data_page_handler(key_signal_process());
        break;
    case TRACK_GRAPH_PAGE:
        track_graph_page_handler(key_signal_process());
        break;
    case MAP_DATA_PAGE:
        map_data_page_handler(key_signal_process());
        break;
    case MAP_GRAPH_PAGE:
        map_graph_page_handler(key_signal_process());
        break;
    case MAP_PURE_PAGE:
        map_pure_page_handler(key_signal_process());
        break;
    case SOLVE_DATA_PAGE:
        solve_data_page_handler(key_signal_process());
        break;
    case SOLVE_GRAPH_PAGE:
        solve_graph_page_handler(key_signal_process());
        break;
    default:
        break;
    }
}
