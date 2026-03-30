#include "user_interaction.h"
// private start------------------

/**
 * @brief 对OLED光标超越最小值的情况进行处理
 * @param current 当前光标位置
 * @param min 光标可取的最小数值
 * @param max 光标可取的最大数值
 */
int min_curser_process(int current, int min, int max)
{
    if (current < min)
    {
        current = max - 1;
    }
    return current;
}

/**
 * @brief 对OLED光标超越最大值的情况进行处理
 * @param current 当前光标位置
 * @param min 光标可取的最小数值
 * @param max 光标可取的最大数值
 */
int max_curser_process(int current, int min, int max)
{
    if (current > max - 1)
    {
        current = min;
    }
    return current;
}

/**
 * @brief 通过修改光标或页面状态实现退出选中或退出当前页面回到菜单
 * @param single_curser 当前光标指针
 */
void menu_exit(Single_Curser_Tpye *single_curser)
{
    if (single_curser->selected == ENABLE)
    {
        single_curser->selected = DISABLE;
    }
    else if (single_curser->selected == DISABLE)
    {
        oled_page = MENU_PAGE;
    }
}

/**
 * @brief 通过修改光标状态实现进入选中状态
 * @param single_curser 当前光标指针
 */
void menu_enter(Single_Curser_Tpye *single_curser)
{
    if (single_curser->selected == DISABLE)
    {
        single_curser->selected = ENABLE;
    }
}

/**
 * @brief 通过修改光标状态实现光标的移动
 * @param single_curser 当前光标指针
 * @param num           移动的步数
 */
void menu_change_curser(Single_Curser_Tpye *single_curser, int num)
{
    if (num >= 0)
    {
        single_curser->curser += num;
        single_curser->curser = max_curser_process(single_curser->curser, 0, single_curser->maxnum);
    }
    else if (num <= 0)
    {
        single_curser->curser += num;
        single_curser->curser = min_curser_process(single_curser->curser, 0, single_curser->maxnum);
    }
}

// private end------------------

/**
 * @brief 各种页面分别的菜单交互逻辑
 * @note  由于菜单里可以调整的参数太多太复杂了，因此没有编写参数数值限幅逻辑
 */

void menu_page_handler(KeySignalType keysignal)
{
    if (keysignal == KEY_UP_SHORT)
    {
        menu_change_curser(&oled_curser.menu_page, -1);
    }
    else if (keysignal == KEY_DOWN_SHORT)
    {
        menu_change_curser(&oled_curser.menu_page, 1);
    }
    else if(keysignal == KEY_CENTER_SHORT)
    {
        switch (oled_curser.menu_page.curser)
        {
        case 0:
            oled_page = CONTROL_PAGE;
            break;
        case 1:
            oled_page = SOLVE_GRAPH_PAGE;
            break;
        case 2:
            oled_page = TRACK_DATA_PAGE;
            break;
        case 3:
            oled_page = MPU6050_DATA_PAGE;
            break;
        case 4:
            oled_page = LED_BUZZER_PAGE;
            break;
        case 5:
            oled_page = NRF_PAGE;
            break;
        default:
            break;
        }
    }
}
void control_page_handler(KeySignalType keysignal)
{
    if (keysignal == KEY_UP_SHORT)
    {
        if (oled_curser.control_page.selected == DISABLE)
        {
            menu_change_curser(&oled_curser.control_page, -1);
        }
        else if (oled_curser.control_page.selected == ENABLE)
        {
            switch (oled_curser.control_page.curser)
            {
            case 0:
                car_set_velocity.v += 0.1;

                break;
            case 1:
                car_set_velocity.angular += 0.1;

                break;
            case 2:
                car_goal_position_set.x += 0.1;
                break;
            case 3:
                car_goal_position_set.y += 0.1;
                break;
            case 5:
                map_detect_switch_set = (FunctionalState)!map_detect_switch_set;
                break;
            case 6:
                tracker.state_set = (FunctionalState)!tracker.state_set;
                break;
            default:
                break;
            }
        }
    }
    else if (keysignal == KEY_DOWN_SHORT)
    {
        if (oled_curser.control_page.selected == DISABLE)
        {
            menu_change_curser(&oled_curser.control_page, 1);
        }
        else if (oled_curser.control_page.selected == ENABLE)
        {
            switch (oled_curser.control_page.curser)
            {
            case 0:
                car_set_velocity.v -= 0.1;

                break;
            case 1:
                car_set_velocity.angular -= 0.1;

                break;
            case 2:
                car_goal_position_set.x -= 0.1;
                break;
            case 3:
                car_goal_position_set.y -= 0.1;
                break;
            case 5:
                map_detect_switch_set = (FunctionalState)!map_detect_switch_set;
                break;
            case 6:
                tracker.state_set = (FunctionalState)!tracker.state_set;
                break;
            default:
                break;
            }
        }
    }
    else if (keysignal == KEY_LEFT_SHORT)
    {
        if (oled_curser.control_page.selected == DISABLE)
        {
            menu_change_curser(&oled_curser.control_page, -1);
        }
    }
    else if (keysignal == KEY_RIGHT_SHORT)
    {
        if (oled_curser.control_page.selected == DISABLE)
        {
            menu_change_curser(&oled_curser.control_page, 1);
        }
    }
    else if (keysignal == KEY_CENTER_SHORT)
    {
        if (oled_curser.control_page.selected == ENABLE)
        {
            if (oled_curser.control_page.curser == 4)
            {
                car_set_velocity.v = 0;
                car_set_velocity.angular = 0;
            }
        }
        else if (oled_curser.control_page.selected == DISABLE)
        {
            switch (oled_curser.control_page.curser)
            {
            case 7:
                nrf_send_image_process_command();
                break;
            case 8:
                nrf_send_solve_command();
                break;
            case 9:
                nrf_send_info_request();
                break;
            case 10:
                nrf_send_map_request();
                break;
            case 11:
                nrf_send_track_request();
                break;
            default:
                menu_enter(&oled_curser.control_page);
                break;
            }
        }
    }
    else if (keysignal == KEY_EDGE1_SHORT)
    {
        menu_exit(&oled_curser.control_page);
    }
}
void solve_graph_page_handler(KeySignalType keysignal)
{
    if (keysignal == KEY_UP_SHORT)
    {
        if (oled_curser.solve_graph_page.selected == DISABLE)
        {
            menu_change_curser(&oled_curser.solve_graph_page, -1);
        }
        else if (oled_curser.solve_graph_page.selected == ENABLE)
        {
            switch (oled_curser.solve_graph_page.curser)
            {
            case 0:
                map_show_level += 1;
                break;
            case 1:
                car_goal_position_set.y += 0.1;
                break;
            default:
                break;
            }
        }
    }
    else if (keysignal == KEY_DOWN_SHORT)
    {
        if (oled_curser.solve_graph_page.selected == DISABLE)
        {
            menu_change_curser(&oled_curser.solve_graph_page, 1);
        }
        else if (oled_curser.solve_graph_page.selected == ENABLE)
        {
            switch (oled_curser.solve_graph_page.curser)
            {
            case 0:
                map_show_level -= 1;
                break;
            case 1:
                car_goal_position_set.y -= 0.1;
                break;
            default:
                break;
            }
        }
    }
    else if (keysignal == KEY_LEFT_SHORT)
    {
        if (oled_curser.solve_graph_page.selected == DISABLE)
        {
            menu_change_curser(&oled_curser.solve_graph_page, -1);
        }
        else if (oled_curser.solve_graph_page.selected == ENABLE)
        {
            switch (oled_curser.solve_graph_page.curser)
            {
            case 1:
                car_goal_position_set.x -= 0.1;
                break;
            default:
                break;
            }
        }
    }
    else if (keysignal == KEY_RIGHT_SHORT)
    {
        if (oled_curser.solve_graph_page.selected == DISABLE)
        {
            menu_change_curser(&oled_curser.solve_graph_page, 1);
        }
        else if (oled_curser.solve_graph_page.selected == ENABLE)
        {
            switch (oled_curser.solve_graph_page.curser)
            {
            case 1:
                car_goal_position_set.x += 0.1;
                break;
            default:
                break;
            }
        }
    }
    else if (keysignal == KEY_CENTER_SHORT)
    {
        menu_enter(&oled_curser.solve_graph_page);
    }
    else if (keysignal == KEY_EDGE1_SHORT)
    {
        menu_exit(&oled_curser.solve_graph_page);
    }
}
void track_data_page_handler(KeySignalType keysignal)
{
    if (keysignal == KEY_UP_SHORT)
    {
        if (oled_curser.track_data_page.selected == ENABLE)
        {
            track_data_page -= 1;
        }
    }
    else if (keysignal == KEY_DOWN_SHORT)
    {
        if (oled_curser.track_data_page.selected == ENABLE)
        {
            track_data_page += 1;
        }
    }
    else if (keysignal == KEY_CENTER_SHORT)
    {
        menu_enter(&oled_curser.track_data_page);
    }
    else if (keysignal == KEY_EDGE1_SHORT)
    {
        menu_exit(&oled_curser.track_data_page);
    }
}
void mpu6050_data_page_handler(KeySignalType keysignal)
{
    if (keysignal == KEY_EDGE1_SHORT)
    {
        oled_page = MENU_PAGE;
    }
}
void led_buzzer_page_handler(KeySignalType keysignal)
{
    if (keysignal == KEY_UP_SHORT)
    {
        if (oled_curser.led_buzzer_page.selected == DISABLE)
        {
            menu_change_curser(&oled_curser.led_buzzer_page, -1);
        }
        else if (oled_curser.led_buzzer_page.selected == ENABLE)
        {
            switch (oled_curser.led_buzzer_page.curser)
            {
            case 0:
                controler_led_color++;
                break;
            case 1:
                controler_buzzer_param.status = (FunctionalState)!controler_buzzer_param.status;
                if (controler_buzzer_param.status == ENABLE)
                {
                    buzzer_on();
                }else
                {
                    buzzer_off();
                }      
                break;
            case 2:
                controler_buzzer_param.set_freq += 10;
                break;
            case 3:
                controler_buzzer_param.set_volume += 10;
                break;
            default:
                break;
            }
        }
    }
    else if (keysignal == KEY_DOWN_SHORT)
    {
        if (oled_curser.led_buzzer_page.selected == DISABLE)
        {
            menu_change_curser(&oled_curser.led_buzzer_page, 1);
        }
        else if (oled_curser.led_buzzer_page.selected == ENABLE)
        {
            switch (oled_curser.led_buzzer_page.curser)
            {
            case 0:
                controler_led_color--;
                break;
            case 1:
                controler_buzzer_param.status = (FunctionalState)!controler_buzzer_param.status;
                if (controler_buzzer_param.status == ENABLE)
                {
                    buzzer_on();
                }
                else
                {
                    buzzer_off();
                }
                break;
            case 2:
                controler_buzzer_param.set_freq -= 10;
                break;
            case 3:
                controler_buzzer_param.set_volume -= 10;
                break;
            default:
                break;
            }
        }
    }
    else if (keysignal == KEY_LEFT_SHORT)
    {
        if (oled_curser.led_buzzer_page.selected == DISABLE)
        {
            menu_change_curser(&oled_curser.led_buzzer_page, -1);
        }
    }
    else if (keysignal == KEY_RIGHT_SHORT)
    {
        if (oled_curser.led_buzzer_page.selected == DISABLE)
        {
            menu_change_curser(&oled_curser.led_buzzer_page, 1);
        }
    }
    else if (keysignal == KEY_CENTER_SHORT)
    {
        menu_enter(&oled_curser.led_buzzer_page);
    }
    else if (keysignal == KEY_EDGE1_SHORT)
    {
        menu_exit(&oled_curser.led_buzzer_page);
    }
}
void joy_handler()
{ 
    // 使用 static 关键字，使变量在函数退出后依然保留数值
    // 用来记录上一次循环时的按键状态（0为松开，1为按下）
    static uint8_t last_up_state = 0;
    static uint8_t last_down_state = 0;
    static uint8_t last_left_state = 0;
    static uint8_t last_right_state = 0;

    // 获取当前按键状态（根据你状态机里的 PRESSING 判断）
    uint8_t current_up_state = (controler_key[KEY_UP].state == KEY_PRESSING);
    uint8_t current_down_state = (controler_key[KEY_DOWN].state == KEY_PRESSING);
    uint8_t current_left_state = (controler_key[KEY_LEFT].state == KEY_PRESSING);
    uint8_t current_right_state = (controler_key[KEY_RIGHT].state == KEY_PRESSING);

    if (current_up_state == 1 && last_up_state == 0)
    {
        car_set_velocity.v = four_dirction_velocity_stright;
        car_set_velocity.angular = 0;
    }
    else if (current_up_state == 0 && last_up_state == 1)
    {
        car_set_velocity.v = 0;
        car_set_velocity.angular = 0;
    }

    if (current_down_state == 1 && last_down_state == 0)
    {
        car_set_velocity.v = -four_dirction_velocity_stright;
        car_set_velocity.angular = 0;
    }
    else if (current_down_state == 0 && last_down_state == 1)
    {
        car_set_velocity.v = 0;
        car_set_velocity.angular = 0;
    }

    if (current_left_state == 1 && last_left_state == 0)
    {
        car_set_velocity.v = four_dirction_velocity_turning;
        car_set_velocity.angular = four_dirction_angular;
    }
    else if (current_left_state == 0 && last_left_state == 1)
    {
        car_set_velocity.v = 0;
        car_set_velocity.angular = 0;
    }

    if (current_right_state == 1 && last_right_state == 0)
    {
        car_set_velocity.v = four_dirction_velocity_turning;
        car_set_velocity.angular = -four_dirction_angular;
    }
    else if (current_right_state == 0 && last_right_state == 1)
    {
        car_set_velocity.v = 0;
        car_set_velocity.angular = 0;
    }

    // ---  更新状态，为下一次循环做准备 ---
    last_up_state = current_up_state;
    last_down_state = current_down_state;
    last_left_state = current_left_state;
    last_right_state = current_right_state;
}
void nrf_page_handler(KeySignalType keysignal){
    if (keysignal == KEY_EDGE1_SHORT)
    {
        oled_page = MENU_PAGE;
    }
}
