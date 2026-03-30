#include "key.h"

// private start------------------

/**
 * @brief 判断按键键值
 */
static int key_judge()
{
    if (HAL_GPIO_ReadPin(UP_KEY_GPIO_Port, UP_KEY_Pin) == GPIO_PIN_RESET)
    {
        return KEY_UP;
    }
    else if (HAL_GPIO_ReadPin(DOWN_KEY_GPIO_Port, DOWN_KEY_Pin) == GPIO_PIN_RESET)
    {
        return KEY_DOWN;
    }
    else if (HAL_GPIO_ReadPin(LEFT_KEY_GPIO_Port, LEFT_KEY_Pin) == GPIO_PIN_RESET)
    {
        return KEY_LEFT;
    }
    else if (HAL_GPIO_ReadPin(RIGHT_KEY_GPIO_Port, RIGHT_KEY_Pin) == GPIO_PIN_RESET)
    {
        return KEY_RIGHT;
    }
    else if (HAL_GPIO_ReadPin(PUSH_KEY_GPIO_Port, PUSH_KEY_Pin) == GPIO_PIN_RESET)
    {
        return KEY_CENTER;
    }
    else if (HAL_GPIO_ReadPin(EDGE1_KEY_GPIO_Port, EDGE1_KEY_Pin) == GPIO_PIN_RESET)
    {
        return KEY_EDGE1;
    }
    else if (HAL_GPIO_ReadPin(EDGE2_KEY_GPIO_Port, EDGE2_KEY_Pin) == GPIO_PIN_RESET)
    {
        return KEY_EDGE2;
    }
    else if (HAL_GPIO_ReadPin(TRACK_KEY_GPIO_Port, TRACK_KEY_Pin) == GPIO_PIN_RESET)
    {
        return KEY_TRACK_CENTER;
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
{ // key up short!------------------------------------------------------------------------------------------------------------------
    if (controler_key[KEY_UP].short_pressed == KEY_ENABLED)
    {
        controler_key[KEY_UP].short_pressed = KEY_DISABLED;
        return KEY_UP_SHORT;
    }
    // key down short!------------------------------------------------------------------------------------------------------------------
    else if (controler_key[KEY_DOWN].short_pressed == KEY_ENABLED)
    {
        controler_key[KEY_DOWN].short_pressed = KEY_DISABLED;
        return KEY_DOWN_SHORT;
    }
    // key left short!------------------------------------------------------------------------------------------------------------------
    else if (controler_key[KEY_LEFT].short_pressed == KEY_ENABLED)
    {
        controler_key[KEY_LEFT].short_pressed = KEY_DISABLED;
        return KEY_LEFT_SHORT;
    }
    // key right short!------------------------------------------------------------------------------------------------------------------
    else if (controler_key[KEY_RIGHT].short_pressed == KEY_ENABLED)
    {
        controler_key[KEY_RIGHT].short_pressed = KEY_DISABLED;
        return KEY_RIGHT_SHORT;
    }
    // key center short!------------------------------------------------------------------------------------------------------------------
    else if (controler_key[KEY_CENTER].short_pressed == KEY_ENABLED)
    {
        controler_key[KEY_CENTER].short_pressed = KEY_DISABLED;
        return KEY_CENTER_SHORT;
    }

    // key edge1 short!------------------------------------------------------------------------------------------------------------------
    else if (controler_key[KEY_EDGE1].short_pressed == KEY_ENABLED)
    {
        controler_key[KEY_EDGE1].short_pressed = KEY_DISABLED;
        return KEY_EDGE1_SHORT;
    }
    // key edge2 short!------------------------------------------------------------------------------------------------------------------
    else if (controler_key[KEY_EDGE2].short_pressed == KEY_ENABLED)
    {
        controler_key[KEY_EDGE2].short_pressed = KEY_DISABLED;
        return KEY_EDGE2_SHORT;
    }
    // track center short!------------------------------------------------------------------------------------------------------------------
    else if (controler_key[KEY_TRACK_CENTER].short_pressed == KEY_ENABLED)
    {
        controler_key[KEY_TRACK_CENTER].short_pressed = KEY_DISABLED;
        return KEY_TRACK_CENTER_SHORT;
    }

    // key center long!------------------------------------------------------------------------------------------------------------------
    else if (controler_key[KEY_CENTER].long_pressed == KEY_ENABLED)
    {
        controler_key[KEY_CENTER].long_pressed = KEY_DISABLED;
        return KEY_CENTER_LONG;
    }
    // key other long!------------------------------------------------------------------------------------------------------------------
    else if (controler_key[KEY_UP].long_pressed == KEY_ENABLED)
    {
        controler_key[KEY_UP].long_pressed = KEY_DISABLED;
        return KEY_UP_LONG;
    }
    else if (controler_key[KEY_DOWN].long_pressed == KEY_ENABLED)
    {
        controler_key[KEY_DOWN].long_pressed = KEY_DISABLED;
        return KEY_DOWN_LONG;
    }
    else if (controler_key[KEY_LEFT].long_pressed == KEY_ENABLED)
    {
        controler_key[KEY_LEFT].long_pressed = KEY_DISABLED;
        return KEY_LEFT_LONG;
    }
    else if (controler_key[KEY_RIGHT].long_pressed == KEY_ENABLED)
    {
        controler_key[KEY_RIGHT].long_pressed = KEY_DISABLED;
        return KEY_RIGHT_LONG;
    }
    else if (controler_key[KEY_EDGE1].long_pressed == KEY_ENABLED)
    {
        controler_key[KEY_EDGE1].long_pressed = KEY_DISABLED;
        return KEY_EDGE1_LONG;
    }
    else if (controler_key[KEY_EDGE2].long_pressed == KEY_ENABLED)
    {
        controler_key[KEY_EDGE2].long_pressed = KEY_DISABLED;
        return KEY_EDGE2_LONG;
    }
    else if (controler_key[KEY_TRACK_CENTER].long_pressed == KEY_ENABLED)
    {
        controler_key[KEY_TRACK_CENTER].long_pressed = KEY_DISABLED;
        return KEY_TRACK_CENTER_LONG;
    }
    else
    {
        return KEY_NO_HAPPEN;
    }
}

/**
 * @brief 黑莓轨迹球处理函数
 */
static void track_key_process()
{
    // 黑莓轨迹球周围有4个旋转编码器，但是每个旋转编码器都是单相的并且和另一个是互不相干的，当轨迹球往一个方向滚动时对应方向的编码器受到摩擦力而转动，而其他三个编码器是不会动的
    // 因此需要检测4个编码器，其中一个编码器有电平变化时对应方向的数值+1
    // 而不是使用电机的双相旋转编码器的判断方法
    // 轨迹球的编码器不能放到按键的长短按遍历逻辑里面，因为如果轨迹球停止的时候，如果编码器数值电平刚好为高，会被认为“按键正在被按下”，干扰其他按键的判断
    // 只有当电平从 低 变到 高 的那一瞬间（上升沿），才判定为一次滚动
    if (controler_track_key.up_key == 1 && controler_track_key.last_up_key == 0)
    {
        controler_track_key.track_key_value.y += 1;
    }
    if (controler_track_key.down_key == 1 && controler_track_key.last_down_key == 0)
    {
        controler_track_key.track_key_value.y -= 1;
    }
    if (controler_track_key.left_key == 1 && controler_track_key.last_left_key == 0)
    {
        controler_track_key.track_key_value.x -= 1;
    }
    if (controler_track_key.right_key == 1 && controler_track_key.last_right_key == 0)
    {
        controler_track_key.track_key_value.x += 1;
    }

    controler_track_key.last_up_key = controler_track_key.up_key;
    controler_track_key.last_down_key = controler_track_key.down_key;
    controler_track_key.last_left_key = controler_track_key.left_key;
    controler_track_key.last_right_key = controler_track_key.right_key;
}

// private end------------------

/**
 * @brief 按键初始化函数
 */
void key_init()
{
    for (int i = 0; i < 5; i++)
    {
        controler_key[i].short_pressed = KEY_DISABLED;
        controler_key[i].long_pressed = KEY_DISABLED;
        controler_key[i].state = KEY_RELEASING;
    }
    controler_track_key.up_key = GPIO_PIN_RESET;
    controler_track_key.up_key = GPIO_PIN_RESET;
    controler_track_key.down_key = GPIO_PIN_RESET;
    controler_track_key.left_key = GPIO_PIN_RESET;
    controler_track_key.right_key = GPIO_PIN_RESET;
    controler_track_key.last_up_key = GPIO_PIN_RESET;
    controler_track_key.last_down_key = GPIO_PIN_RESET;
    controler_track_key.last_left_key = GPIO_PIN_RESET;
    controler_track_key.last_right_key = GPIO_PIN_RESET;
    controler_track_key.track_key_value.x = 0;
    controler_track_key.track_key_value.y = 0;
    controler_track_key.track_key_value.z = 0;
}

/**
 * @brief 扫描按键，使用main tick方法
 */
void key_read()
{
    int key_now = key_judge();
    uint32_t now_tick = HAL_GetTick(); // 获取当前系统绝对时间

    for (int i = 0; i < 8; i++)
    {
        if (controler_key[i].state == KEY_RELEASING)
        {
            if (key_now == i)
            {
                HAL_Delay(10);
                if (key_now == i)
                {
                    controler_key[i].state = KEY_PRESSING;
                }
            }
        }
        else if (controler_key[i].state == KEY_PRESSING)
        {
            controler_key[i].press_time++;
            if (key_now != i)
            {
                if (controler_key[i].press_time > KEY_LONG_CNT)
                {
                    controler_key[i].state = KEY_RELEASING;
                    controler_key[i].long_pressed = KEY_ENABLED;
                    controler_key[i].press_time = 0;
                }
                else
                {
                    controler_key[i].state = KEY_RELEASING;
                    controler_key[i].short_pressed = KEY_ENABLED;
                    controler_key[i].press_time = 0;
                }
            }
        }
    }

    controler_track_key.up_key = HAL_GPIO_ReadPin(TRACK_UP_GPIO_Port, TRACK_UP_Pin);
    controler_track_key.down_key = HAL_GPIO_ReadPin(TRACK_DOWN_GPIO_Port, TRACK_DOWN_Pin);
    controler_track_key.left_key = HAL_GPIO_ReadPin(TRACK_LEFT_GPIO_Port, TRACK_LEFT_Pin);
    controler_track_key.right_key = HAL_GPIO_ReadPin(TRACK_RIGHT_GPIO_Port, TRACK_RIGHT_Pin);
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
    case CONTROL_PAGE:
        control_page_handler(key_signal_process());
        break;
    case SOLVE_GRAPH_PAGE:
        solve_graph_page_handler(key_signal_process());
        break;
    case TRACK_DATA_PAGE:
        track_data_page_handler(key_signal_process());
        break;
    case MPU6050_DATA_PAGE:
        mpu6050_data_page_handler(key_signal_process());
        break;
    case LED_BUZZER_PAGE:
        led_buzzer_page_handler(key_signal_process());
        break;
    case NRF_PAGE:
        nrf_page_handler(key_signal_process());
        break;
    default:
        break;
    }

    // joy!------------------------------------------------------------------------------------------------------------------
    if (oled_page == CONTROL_PAGE && oled_curser.control_page.curser == 4 && oled_curser.control_page.selected == ENABLE)
    {
        joy_handler();
    }

    // track!------------------------------------------------------------------------------------------------------------------
    track_key_process();
}
