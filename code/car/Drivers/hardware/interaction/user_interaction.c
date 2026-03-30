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
    else if (keysignal == KEY_LEFT_SHORT)
    {
        menu_change_curser(&oled_curser.menu_page, -8);
    }
    else if (keysignal == KEY_RIGHT_SHORT)
    {
        menu_change_curser(&oled_curser.menu_page, 8);
    }
    else if (keysignal == KEY_CENTER_SHORT)
    {
        switch (oled_curser.menu_page.curser)
        {
        case 0:
            oled_page = ENCODER_DATA_PAGE;
            break;
        case 1:
            oled_page = ENCODER_GRAPH_PAGE;
            break;
        case 2:
            oled_page = MPU6050_DATA_PAGE;
            break;
        case 3:
            oled_page = MPU6050_GRAPH_PAGE;
            break;
        case 4:
            oled_page = COMBINED_DATA_PAGE;
            break;
        case 5:
            oled_page = COMBINED_GRAPH_PAGE;
            break;
        case 6:
            oled_page = MOTOR_CONTROL_PAGE;
            break;
        case 7:
            oled_page = NRF_PAGE;
            break;
        case 8:
            oled_page = LED_BUZZER_SYS_PAGE;
            break;
        case 9:
            oled_page = TRACK_DATA_PAGE;
            break;
        case 10:
            oled_page = TRACK_GRAPH_PAGE;
            break;
        case 11:
            oled_page = MAP_DATA_PAGE;
            break;
        case 12:
            oled_page = MAP_GRAPH_PAGE;
            break;
        case 13:
            oled_page = MAP_PURE_PAGE;
            break;
        case 14:
            oled_page = SOLVE_DATA_PAGE;
            break;
        case 15:
            oled_page = SOLVE_GRAPH_PAGE;
            break;
        default:
            break;
        }
    }
    else if (keysignal == KEY_CENTER_LONG)
    {
    }
}

void encoder_data_page_handler(KeySignalType keysignal)
{
    if (keysignal == KEY_UP_SHORT)
    {
        if (oled_curser.encoder_data_page.selected == DISABLE)
        {
            menu_change_curser(&oled_curser.encoder_data_page, -2);
        }
        else if (oled_curser.encoder_data_page.selected == ENABLE)
        {
            switch (oled_curser.encoder_data_page.curser)
            {
            case 0:
                k_motor_speed += 0.01;
                break;
            case 1:
                b_motor_speed += 0.01;
                break;
            case 2:
                k_motor_delta += 0.01;
                break;
            case 3:
                b_motor_delta += 0.01;
                break;
            default:
                break;
            }
        }
    }
    else if (keysignal == KEY_DOWN_SHORT)
    {
        if (oled_curser.encoder_data_page.selected == DISABLE)
        {
            menu_change_curser(&oled_curser.encoder_data_page, 2);
        }
        else if (oled_curser.encoder_data_page.selected == ENABLE)
        {
            switch (oled_curser.encoder_data_page.curser)
            {
            case 0:
                k_motor_speed -= 0.01;
                break;
            case 1:
                b_motor_speed -= 0.01;
                break;
            case 2:
                k_motor_delta -= 0.01;
                break;
            case 3:
                b_motor_delta -= 0.01;
                break;
            default:
                break;
            }
        }
    }
    else if (keysignal == KEY_LEFT_SHORT)
    {
        menu_change_curser(&oled_curser.encoder_data_page, -1);
    }
    else if (keysignal == KEY_RIGHT_SHORT)
    {
        menu_change_curser(&oled_curser.encoder_data_page, 1);
    }
    else if (keysignal == KEY_CENTER_SHORT)
    {
        menu_enter(&oled_curser.encoder_data_page);
    }
    else if (keysignal == KEY_CENTER_LONG)
    {
        menu_exit(&oled_curser.encoder_data_page);
    }
}

void encoder_graph_page_handler(KeySignalType keysignal)
{
    if (keysignal == KEY_CENTER_LONG)
    {
        oled_page = MENU_PAGE;
    }
}

void mpu6050_data_page_handler(KeySignalType keysignal)
{
    if (keysignal == KEY_CENTER_LONG)
    {
        oled_page = MENU_PAGE;
    }
}
void mpu6050_graph_page_handler(KeySignalType keysignal)
{
    if (keysignal == KEY_CENTER_LONG)
    {
        oled_page = MENU_PAGE;
    }
}
void combined_data_page_handler(KeySignalType keysignal)
{
    if (keysignal == KEY_UP_SHORT)
    {
        if (oled_curser.combined_data_page.selected == ENABLE)
        {
            position_angle_combined_k += 0.01;
        }
    }
    else if (keysignal == KEY_DOWN_SHORT)
    {
        if (oled_curser.combined_data_page.selected == ENABLE)
        {
            position_angle_combined_k -= 0.01;
        }
    }
    else if (keysignal == KEY_CENTER_SHORT)
    {
        menu_enter(&oled_curser.combined_data_page);
    }
    else if (keysignal == KEY_CENTER_LONG)
    {
        menu_exit(&oled_curser.combined_data_page);
    }
}
void combined_graph_page_handler(KeySignalType keysignal)
{
    if (keysignal == KEY_CENTER_LONG)
    {
        oled_page = MENU_PAGE;
    }
}
void motor_control_page_handler(KeySignalType keysignal)
{
    if (keysignal == KEY_UP_SHORT)
    {
        if (oled_curser.motor_control_page.selected == DISABLE)
        {
            menu_change_curser(&oled_curser.motor_control_page, -2);
        }
        else if (oled_curser.motor_control_page.selected == ENABLE)
        {
            switch (oled_curser.motor_control_page.curser)
            {
            case 0:
            {
                // 由于control_mode是无符号枚举类型，因此不适用max_curser_process和min_curser_process
                control_mode++;
                if (control_mode >= NRF_CTRL)
                {
                    control_mode = NRF_CTRL;
                }
                break;
            }
            case 1:
                car_set_velocity.v += 0.1;
                break;
            case 2:
                car_set_velocity.angular += 0.1;
                break;
            case 4:
                four_dirction_velocity += 0.1;
                break;
            case 5:
                four_dirction_angular += 0.1;
                break;
            case 6:
                MOTOR_A.Motor_Pwm += 100;
                break;
            case 7:
                MOTOR_B.Motor_Pwm += 100;
                break;
            case 8:
                MOTOR_A.Target += 0.1;
                break;
            case 9:
                MOTOR_B.Target += 0.1;
                break;
            case 10:
                MOTOR_A.Velocity_KP += 1;
                break;
            case 11:
                MOTOR_B.Velocity_KP += 1;
                break;
            case 12:
                MOTOR_A.Velocity_KI += 1;
                break;
            case 13:
                MOTOR_B.Velocity_KI += 1;
                break;

            default:
                break;
            }
        }
    }
    else if (keysignal == KEY_DOWN_SHORT)
    {
        if (oled_curser.motor_control_page.selected == DISABLE)
        {
            menu_change_curser(&oled_curser.motor_control_page, 2);
        }
        else if (oled_curser.motor_control_page.selected == ENABLE)
        {
            switch (oled_curser.motor_control_page.curser)
            {
            case 0:
            {
                control_mode--;
                // 由于control_mode是无符号枚举类型，因此不适用max_curser_process和min_curser_process
                if (control_mode == 0xff)
                {
                    control_mode = HAND_PWM;
                }
                break;
            }
            case 1:
                car_set_velocity.v -= 0.1;
                break;
            case 2:
                car_set_velocity.angular -= 0.1;
                break;
            case 4:
                four_dirction_velocity -= 0.1;
                break;
            case 5:
                four_dirction_angular -= 0.1;
                break;
            case 6:
                MOTOR_A.Motor_Pwm -= 100;
                break;
            case 7:
                MOTOR_B.Motor_Pwm -= 100;
                break;
            case 8:
                MOTOR_A.Target -= 0.1;
                break;
            case 9:
                MOTOR_B.Target -= 0.1;
                break;
            case 10:
                MOTOR_A.Velocity_KP -= 1;
                break;
            case 11:
                MOTOR_B.Velocity_KP -= 1;
                break;
            case 12:
                MOTOR_A.Velocity_KI -= 1;
                break;
            case 13:
                MOTOR_B.Velocity_KI -= 1;
                break;

            default:
                break;
            }
        }
    }
    else if (keysignal == KEY_LEFT_SHORT)
    {
        if (oled_curser.motor_control_page.selected == DISABLE)
        {
            menu_change_curser(&oled_curser.motor_control_page, -1);
        }
    }
    else if (keysignal == KEY_RIGHT_SHORT)
    {
        if (oled_curser.motor_control_page.selected == DISABLE)
        {
            menu_change_curser(&oled_curser.motor_control_page, 1);
        }
    }
    else if (keysignal == KEY_CENTER_SHORT)
    {
        menu_enter(&oled_curser.motor_control_page);
    }
    else if (keysignal == KEY_CENTER_LONG)
    {
        menu_exit(&oled_curser.motor_control_page);
    }
}
void nrf_page_handler(KeySignalType keysignal)
{
    if (keysignal == KEY_CENTER_LONG)
    {
        oled_page = MENU_PAGE;
    }
}
void led_buzzer_sys_page_handler(KeySignalType keysignal)
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
            {
                car_led_color++;
                break;
            }
            case 1:
                car_buzzer_param.status = (FunctionalState)!car_buzzer_param.status;
                break;
            case 2:
                car_buzzer_param.set_freq += 10;
                break;
            case 3:
                oled_refresh_gap += 1;
                break;
            case 4:
                display_max_time = (FunctionalState)!display_max_time;
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
            {
                car_led_color--;
                break;
            }
            case 1:
                car_buzzer_param.status = (FunctionalState)!car_buzzer_param.status;
                break;
            case 2:
                car_buzzer_param.set_freq -= 10;
                break;
            case 3:
                oled_refresh_gap -= 1;
                break;
            case 4:
                display_max_time = (FunctionalState)!display_max_time;
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
    else if (keysignal == KEY_CENTER_LONG)
    {
        menu_exit(&oled_curser.led_buzzer_page);
    }
}
void track_data_page_handler(KeySignalType keysignal)
{
    if (keysignal == KEY_UP_SHORT)
    {
        if (oled_curser.track_data_page.selected == DISABLE)
        {
            menu_change_curser(&oled_curser.track_data_page, -1);
        }
        else if (oled_curser.track_data_page.selected == ENABLE)
        {
            switch (oled_curser.track_data_page.curser)
            {
                case 0:  case 1:  case 2:  case 3:
                case 4:  case 5:  case 6:  case 7:
                case 8:  case 9:  case 10: case 11:
                case 12: case 13: case 14: case 15:
            {
                // 核心逻辑：将当前的 case 索引转换为数组中的实际位置
                // 每页起始索引是 track_data_page * 8
                // cmd_idx / 2 得到的是这一页的第几行
                uint16_t real_idx = (track_data_page * 8) + (oled_curser.track_data_page.curser / 2);

                // 越界检查（复利思维：永远保护你的内存）
                if (real_idx < TRACK_LEN_MAX)
                {
                    if (oled_curser.track_data_page.curser % 2 == 0) // 偶数 case 改 X
                    {
                        car_track[real_idx].x += 0.1f;
                    }
                    else // 奇数 case 改 Y
                    {
                        car_track[real_idx].y += 0.1f;
                    }
                }
                break;
            }
            case 16:
                track_data_page -= 1;
                break;
            case 17:
                current_track_length += 1;
                break;

            case 19:
                tracker.target_speed += 0.1;
                break;
            case 20:
                tracker.k_pure_pursuit += 0.1;
                break;
            case 21:
                tracker.slowdown_start_dist += 0.1;
                break;
            case 22:
                tracker.stop_dist += 0.1;
                break;
            case 23:
                tracker.state = (FunctionalState)!tracker.state;
                break;
            case 25:
                tracker.lookahead_distance += 0.1;
                break;
            default:
                break;
            }
        }
    }
    else if (keysignal == KEY_DOWN_SHORT)
    {
        if (oled_curser.track_data_page.selected == DISABLE)
        {
            menu_change_curser(&oled_curser.track_data_page, 1);
        }
        else if (oled_curser.track_data_page.selected == ENABLE)
        {
            switch (oled_curser.track_data_page.curser)
            {
                case 0:  case 1:  case 2:  case 3:
                case 4:  case 5:  case 6:  case 7:
                case 8:  case 9:  case 10: case 11:
                case 12: case 13: case 14: case 15:
            {
                // 核心逻辑：将当前的 case 索引转换为数组中的实际位置
                // 每页起始索引是 track_data_page * 8
                // cmd_idx / 2 得到的是这一页的第几行
                uint16_t real_idx = (track_data_page * 8) + (oled_curser.track_data_page.curser / 2);

                // 越界检查（复利思维：永远保护你的内存）
                if (real_idx < TRACK_LEN_MAX)
                {
                    if (oled_curser.track_data_page.curser % 2 == 0) // 偶数 case 改 X
                    {
                        car_track[real_idx].x -= 0.1f;
                    }
                    else // 奇数 case 改 Y
                    {
                        car_track[real_idx].y -= 0.1f;
                    }
                }
                break;
            }
            case 16:
                track_data_page += 1;
                break;
            case 17:
                current_track_length -= 1;
                break;
            case 19:
                tracker.target_speed -= 0.1;
                break;
            case 20:
                tracker.k_pure_pursuit -= 0.1;
                break;
            case 21:
                tracker.slowdown_start_dist -= 0.1;
                break;
            case 22:
                tracker.stop_dist -= 0.1;
                break;
            case 23:
                tracker.state = (FunctionalState)!tracker.state;
                break;
            case 25:
                tracker.lookahead_distance -= 0.1;
                break;
            default:
                break;
            }
        }
    }
    else if (keysignal == KEY_LEFT_SHORT)
    {
        if (oled_curser.track_data_page.selected == DISABLE)
        {
            menu_change_curser(&oled_curser.track_data_page, -1);
        }
    }
    else if (keysignal == KEY_RIGHT_SHORT)
    {
        if (oled_curser.track_data_page.selected == DISABLE)
        {
            menu_change_curser(&oled_curser.track_data_page, 1);
        }
    }
    else if (keysignal == KEY_CENTER_SHORT)
    {
        if (oled_curser.track_data_page.curser == 18)
        {
            PointType temp[] = {{0.0f, 0.0f}, {0.0f, 0.5f}, {0.5f, 0.5f}, {1.0f, 0.5f}, {1.5f, 0.5f}, {1.5f, 0.0f}, {1.5f, -0.5f}, {1.0f, -0.5f}};
            for (int i = 0; i < 8; i++)
            {
                car_track[i] = temp[i];
            }
            current_track_length = 8;
            Tracker_Init(&tracker, car_track, current_track_length, 0.1f, 0.8f);
        }
        else if (oled_curser.track_data_page.curser == 24)
        {
            Tracker_Init(&tracker, car_track, current_track_length, 0.1f, 0.8f);
        }
        else
        {
            menu_enter(&oled_curser.track_data_page);
        }
    }
    else if (keysignal == KEY_CENTER_LONG)
    {
        menu_exit(&oled_curser.track_data_page);
    }
}
void track_graph_page_handler(KeySignalType keysignal)
{
    if (keysignal == KEY_CENTER_LONG)
    {
        oled_page = MENU_PAGE;
    }
}
void map_data_page_handler(KeySignalType keysignal)
{
    if (keysignal == KEY_UP_SHORT)
    {
        if (oled_curser.map_data_page.selected == DISABLE)
        {
            menu_change_curser(&oled_curser.map_data_page, -1);
        }
        else if (oled_curser.map_data_page.selected == ENABLE)
        {
            if (oled_curser.map_data_page.curser == 4)
            {
                map_detect_switch = (FunctionalState)!map_detect_switch;
            }
        }
    }
    else if (keysignal == KEY_DOWN_SHORT)
    {
        if (oled_curser.map_data_page.selected == DISABLE)
        {
            menu_change_curser(&oled_curser.map_data_page, 1);
        }
        else if (oled_curser.map_data_page.selected == ENABLE)
        {
            if (oled_curser.map_data_page.curser == 4)
            {
                map_detect_switch = (FunctionalState)!map_detect_switch;
            }
        }
    }
    else if (keysignal == KEY_LEFT_SHORT)
    {
        menu_change_curser(&oled_curser.map_data_page, -3);
    }
    else if (keysignal == KEY_RIGHT_SHORT)
    {
        menu_change_curser(&oled_curser.map_data_page, 3);
    }
    else if (keysignal == KEY_CENTER_SHORT)
    {
        switch (oled_curser.map_data_page.curser)
        {
        case 0:
            Map_Global_Opening_InPlace();
            Map_Compress_Process(METER_PIXEL);
            break;
        case 1:
            erode_map();
            break;
        case 2:
            dilate_map();
            break;
        case 3:
            Map_Generate_Boundary_Contour();
            break;
        case 4:
            menu_enter(&oled_curser.map_data_page);
            break;
        case 5:
        {
            Map_Generate_Test_C_Shape_Advanced();
            Map_Compress_Process(METER_PIXEL);
            car_goal_position.x = 0;
            car_goal_position.y = 0.7;
            PlanPath_Iterative(car_combined_processed.CombinedPosition, car_goal_position, car_track, &current_track_length);
            Tracker_Init(&tracker, car_track, current_track_length, 0.1f, 0.8f);
            break;
        }
        default:
            break;
        }
    }
    else if (keysignal == KEY_CENTER_LONG)
    {
        menu_exit(&oled_curser.map_data_page);
    }
}
void map_graph_page_handler(KeySignalType keysignal)
{
    if (keysignal == KEY_UP_SHORT)
    {
        if (oled_curser.map_graph_page.selected == ENABLE)
        {
            map_show_level += 1;
        }
    }
    else if (keysignal == KEY_DOWN_SHORT)
    {
        if (oled_curser.map_graph_page.selected == ENABLE)
        {
            map_show_level -= 1;
        }
    }
    else if (keysignal == KEY_LEFT_SHORT)
    {
    }
    else if (keysignal == KEY_RIGHT_SHORT)
    {
    }
    else if (keysignal == KEY_CENTER_SHORT)
    {
        menu_enter(&oled_curser.map_graph_page);
    }
    else if (keysignal == KEY_CENTER_LONG)
    {
        menu_exit(&oled_curser.map_graph_page);
    }
}
void map_pure_page_handler(KeySignalType keysignal)
{
    if (keysignal == KEY_UP_SHORT)
    {
        if (oled_curser.map_pure_page.selected == DISABLE)
        {
            menu_change_curser(&oled_curser.map_pure_page, -1);
        }
        else if (oled_curser.map_pure_page.selected == ENABLE)
        {
            switch (oled_curser.map_pure_page.curser)
            {
            case 0:
                map_show_level += 1;
                break;
            case 1:
                map_show_area.y -= 1;
                break;

            default:
                break;
            }
        }
    }
    else if (keysignal == KEY_DOWN_SHORT)
    {
        if (oled_curser.map_pure_page.selected == DISABLE)
        {
            menu_change_curser(&oled_curser.map_pure_page, 1);
        }
        else if (oled_curser.map_pure_page.selected == ENABLE)
        {
            switch (oled_curser.map_pure_page.curser)
            {
            case 0:
                map_show_level -= 1;
                break;
            case 1:
                map_show_area.y += 1;
                break;

            default:
                break;
            }
        }
    }
    else if (keysignal == KEY_LEFT_SHORT)
    {
        if (oled_curser.map_pure_page.selected == DISABLE)
        {
            menu_change_curser(&oled_curser.map_pure_page, -1);
        }
        else if (oled_curser.map_pure_page.selected == ENABLE)
        {
            switch (oled_curser.map_pure_page.curser)
            {
            case 1:
                map_show_area.x -= 1;
                break;

            default:
                break;
            }
        }
    }
    else if (keysignal == KEY_RIGHT_SHORT)
    {
        if (oled_curser.map_pure_page.selected == DISABLE)
        {
            menu_change_curser(&oled_curser.map_pure_page, 1);
        }
        else if (oled_curser.map_pure_page.selected == ENABLE)
        {
            switch (oled_curser.map_pure_page.curser)
            {
            case 1:
                map_show_area.x += 1;
                break;

            default:
                break;
            }
        }
    }
    else if (keysignal == KEY_CENTER_SHORT)
    {
        menu_enter(&oled_curser.map_pure_page);
    }
    else if (keysignal == KEY_CENTER_LONG)
    {
        menu_exit(&oled_curser.map_pure_page);
    }
}
void solve_data_page_handler(KeySignalType keysignal)
{
    if (keysignal == KEY_UP_SHORT)
    {
        if (oled_curser.solve_data_page.selected == DISABLE)
        {
            menu_change_curser(&oled_curser.solve_data_page, -1);
        }
        else if (oled_curser.solve_data_page.selected == ENABLE)
        {
            switch (oled_curser.solve_data_page.curser)
            {
            case 0:
                car_goal_position.x += 0.1;
                break;
            case 1:
                car_goal_position.y += 0.1;
                break;
            case 3:
                solve_data_page -= 1;
                break;
            default:
                break;
            }
        }
    }
    else if (keysignal == KEY_DOWN_SHORT)
    {
        if (oled_curser.solve_data_page.selected == DISABLE)
        {
            menu_change_curser(&oled_curser.solve_data_page, 1);
        }
        else if (oled_curser.solve_data_page.selected == ENABLE)
        {
            switch (oled_curser.solve_data_page.curser)
            {
            case 0:
                car_goal_position.x -= 0.1;
                break;
            case 1:
                car_goal_position.y -= 0.1;
                break;
            case 3:
                solve_data_page += 1;
                break;
            default:
                break;
            }
        }
    }
    else if (keysignal == KEY_LEFT_SHORT)
    {
        menu_change_curser(&oled_curser.solve_data_page, -1);
    }
    else if (keysignal == KEY_RIGHT_SHORT)
    {
        menu_change_curser(&oled_curser.solve_data_page, 1);
    }
    else if (keysignal == KEY_CENTER_SHORT)
    {
        if (oled_curser.solve_data_page.curser == 2)
        {
            PlanPath_Iterative(car_combined_processed.CombinedPosition, car_goal_position, car_track, &current_track_length);
            Tracker_Init(&tracker, car_track, current_track_length, 0.1f, 0.8f);
        }
        else
        {
            menu_enter(&oled_curser.solve_data_page);
        }
    }
    else if (keysignal == KEY_CENTER_LONG)
    {
        menu_exit(&oled_curser.solve_data_page);
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
                car_goal_position.y += 0.1;
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
                car_goal_position.y -= 0.1;
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
                car_goal_position.x -= 0.1;
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
                car_goal_position.x += 0.1;
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
    else if (keysignal == KEY_CENTER_LONG)
    {
        menu_exit(&oled_curser.solve_graph_page);
    }
}
