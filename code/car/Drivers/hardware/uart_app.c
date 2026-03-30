#include "uart_app.h"

uint8_t uart1_rxdat;                // 收到的单个数据
volatile unsigned char uart1_rx_p;  // 缓冲区指针  
// private start-------------------------

/**
 * @brief 将控制状态枚举转化为易读的字符串
 * @param mode 控制状态
 * @return const char* 对应的描述字符串
 */
static const char *ControlModeToString_uart(Control_Mode mode)
{
    switch (mode)
    {
    case HAND_PWM:
        return "HAND_PWM";
    case HAND_LR:
        return "HAND_LR";
    case HAND_XYZ:
        return "HAND_XYZ";
    case NRF_CTRL:
        return "NRF_CTRL";
    case JOY_XYZ:
        return "JOY_XYZ";
    case HAND_TRACK:
        return "HAND_TRACK";
    case HAND_MODE_NUM:
        return "HAND_MODE_NUM";
    default:
        return "UNKNOWN_CONTROL_MODE";
    }
}

/**
 * @brief 将LED状态枚举转化为易读的字符串
 * @param mode LED状态
 * @return const char* 对应的描述字符串
 */
static const char *led_color_to_string_uart(Led_Color color)
{
    switch (color)
    {
    case OFF:
        return "OFF";
    case RED:
        return "RED";
    case GREEN:
        return "GREEN";
    case BLUE:
        return "BLUE";
    case YELLOW:
        return "YELLOW";
    case PURPLE:
        return "PURPLE";
    case CYAN:
        return "CYAN";
    case WHITE:
        return "WHITE";
    case BREATH:
        return "BREATH";
    default:
        return "UNKNOWN";
    }
}

/**
 * @brief 将寻径状态枚举转化为易读的字符串
 * @param state 寻径状态
 * @return const char* 对应的描述字符串
 */
static const char *get_solve_state_string_uart(SolveType state)
{
    switch (state)
    {
    case NO_FIND:
        return "NO      "; // 寻径失败
    case FIND_BACKED_SOLVE:
        return "BACKED  "; // 历史避让
    case FIND_EDGE_SOLVE:
        return "EDGE    "; // 边缘通过
    case FIND_SPACIOUS_SOLVE:
        return "SPACIOUS"; // 空旷通过
    default:
        return "UNKNOW  "; // 未知状态
    }
}

/**
 * @brief 把缓冲区的数据通过串口发送出去
 */
static void uart1_transmit()
{
    // 最早没有计算字符串实际长度，导致每条消息都只能发送一半就没了
    // 计算字符串实际长度
    uint16_t len = 0;

    // 安全地计算字符串长度
    while (len < UART1_TX_SIZE && uart1_send_buf[len] != '\0')
    {
        len++;
    }

    if (len > 0)
    {
        // 发送实际长度，增加超时时间
        HAL_UART_Transmit(&huart1, (uint8_t *)uart1_send_buf, len, 100);
    }

    // // 可选：添加小延迟，避免连续发送冲突
    // HAL_Delay(10);
}

/**
 * @brief 以16进制的方式打印nrf接收到的内容
 */
static void nrf_print(){
    for (int i = 0; i < 32; i++)
    {
        // 每次格式化一个字节到缓冲区
        // %02X 表示大写十六进制，不足两位补0
        sprintf(uart1_send_buf, "%02X ", (uint8_t)nrf_receive_buf[i]);
        uart1_transmit();
    }
}
// private end-----------------------------

/**
 * @brief 初始化串口
 */
void uart1_init()
{
    HAL_UART_Receive_IT(&huart1, &uart1_rxdat, 1);
}

/**
 * @brief 串口接收
 */
void uart1_receivce()
{
    uart1_rxdata[uart1_rx_p] = uart1_rxdat;
    HAL_UART_Receive_IT(&huart1, &uart1_rxdat, 1);
    uart1_rx_p++;
    last_rx_time = HAL_GetTick();
}

/**
 * @brief 串口处理主函数
 * @note 编写这段代码的时候，falsh又爆了，通过提升编译优化等级到O2，并且启用MicroLib来减小程序体积
 *       缺点就是调试时可能碰到编译优化导致汇编和C语言语义不对应问题,此时需要分析汇编得出bug发生的原因
 */
void uart1_process()
{
    if (uart1_rx_p >= 3)
    {
        if (strcmp(uart1_rxdata, "print") == 0)
        {
            memset(uart1_rxdata, 0, UART1_RX_SIZE);
            uart1_rx_p = 0;
            sprintf(uart1_send_buf, "-----------------------------------------------\r\n"), uart1_transmit();
            sprintf(uart1_send_buf, "Moving Data\r\n"), uart1_transmit();
            sprintf(uart1_send_buf, "V:%.2f  angular:%.2f\r\n", car_set_velocity.v, car_set_velocity.angular), uart1_transmit();
            sprintf(uart1_send_buf, "Control_Mode:%s\r\n", ControlModeToString_uart(control_mode)), uart1_transmit();
            sprintf(uart1_send_buf, "-----------------------------------------------\r\n"), uart1_transmit();
            sprintf(uart1_send_buf, "Motor Data\r\n"), uart1_transmit();
            sprintf(uart1_send_buf, "            MotorA:  MotorB:\r\n"), uart1_transmit();
            sprintf(uart1_send_buf, "Encoder:     %.2f     %.2f :\r\n", MOTOR_A.Encoder, MOTOR_B.Encoder), uart1_transmit();
            sprintf(uart1_send_buf, "Motor_Pwm:   %.2f     %.2f :\r\n", MOTOR_A.Motor_Pwm, MOTOR_B.Motor_Pwm), uart1_transmit();
            sprintf(uart1_send_buf, "Target:      %.2f     %.2f :\r\n", MOTOR_A.Target, MOTOR_B.Target), uart1_transmit();
            sprintf(uart1_send_buf, "Velocity_KP: %.2f     %.2f :\r\n", MOTOR_A.Velocity_KP, MOTOR_B.Velocity_KP), uart1_transmit();
            sprintf(uart1_send_buf, "Velocity_KI: %.2f     %.2f :\r\n", MOTOR_A.Velocity_KI, MOTOR_B.Velocity_KI), uart1_transmit();
            sprintf(uart1_send_buf, "Bias:        %.2f     %.2f :\r\n", MOTOR_A.Bias, MOTOR_B.Bias), uart1_transmit();
            sprintf(uart1_send_buf, "Last_bias:   %.2f     %.2f :\r\n", MOTOR_A.Last_bias, MOTOR_B.Last_bias), uart1_transmit();
            sprintf(uart1_send_buf, "-----------------------------------------------\r\n"), uart1_transmit();
            sprintf(uart1_send_buf, "Encoder Data       time:%d\r\n", car_encoder_processed.Encoder_Velocity.time), uart1_transmit();
            sprintf(uart1_send_buf, "motor_speed: %.2f  motor_delta: %.2f\r\n", car_encoder_processed.Encoder_Velocity.motor_speed, car_encoder_processed.Encoder_Velocity.motor_delta), uart1_transmit();
            sprintf(uart1_send_buf, "x:           %.2f  y:           %.2f v:%.2f\r\n", car_encoder_processed.Encoder_Position.x, car_encoder_processed.Encoder_Position.y, car_encoder_processed.Encoder_Position.theta), uart1_transmit();
            sprintf(uart1_send_buf, "-----------------------------------------------\r\n"), uart1_transmit();
            sprintf(uart1_send_buf, "Mpu6050Data        time:%d\r\n", car_mpu6050_data.time), uart1_transmit();
            sprintf(uart1_send_buf, "temperature:%.2f   X     Y     Z\r\n", car_mpu6050_data.temperature), uart1_transmit();
            sprintf(uart1_send_buf, "acceleration:     %.2f  %.2f  %.2f\r\n", car_mpu6050_data.acceleration.x, car_mpu6050_data.acceleration.y, car_mpu6050_data.acceleration.z), uart1_transmit();
            sprintf(uart1_send_buf, "angular_velocity: %.2f  %.2f  %.2f\r\n", car_mpu6050_data.angular_velocity.x, car_mpu6050_data.angular_velocity.y, car_mpu6050_data.angular_velocity.z), uart1_transmit();
            sprintf(uart1_send_buf, "angle:            %.2f  %.2f  %.2f\r\n", car_mpu6050_data.angle.x, car_mpu6050_data.angle.y, car_mpu6050_data.angle.z), uart1_transmit();
            sprintf(uart1_send_buf, "magnet:           %.2f  %.2f  %.2f\r\n", car_mpu6050_data.magnet.x, car_mpu6050_data.magnet.y, car_mpu6050_data.magnet.z), uart1_transmit();
            sprintf(uart1_send_buf, "-----------------------------------------------\r\n"), uart1_transmit();
            sprintf(uart1_send_buf, "Mpu6050Processed   time:%d\r\n", car_mpu6050_processed.time), uart1_transmit();
            sprintf(uart1_send_buf, "                    X     Y     Z\r\n"), uart1_transmit();
            sprintf(uart1_send_buf, "Velocity:          %.2f  %.2f  %.2f\r\n", car_mpu6050_processed.vel.x, car_mpu6050_processed.vel.y, car_mpu6050_processed.vel.z), uart1_transmit();
            sprintf(uart1_send_buf, "Position:          %.2f  %.2f  %.2f\r\n", car_mpu6050_processed.pos.x, car_mpu6050_processed.pos.y, car_mpu6050_processed.pos.z), uart1_transmit();
            sprintf(uart1_send_buf, "acceleration_no_g: %.2f  %.2f  %.2f\r\n", car_mpu6050_processed.acceleration_no_gravity.x, car_mpu6050_processed.acceleration_no_gravity.y, car_mpu6050_processed.acceleration_no_gravity.z), uart1_transmit();
            sprintf(uart1_send_buf, "angle_remove_first:%.2f  %.2f  %.2f\r\n", car_mpu6050_processed.angle_remove_first.x, car_mpu6050_processed.angle_remove_first.y, car_mpu6050_processed.angle_remove_first.z), uart1_transmit();
            sprintf(uart1_send_buf, "-----------------------------------------------\r\n"), uart1_transmit();
            sprintf(uart1_send_buf, "CombinedProcessed \r\n"), uart1_transmit();
            sprintf(uart1_send_buf, "X:%.2f  Y:%.2f  theta:%.2f\r\n", car_combined_processed.CombinedPosition.x, car_combined_processed.CombinedPosition.y, car_combined_processed.CombinedPosition.theta), uart1_transmit();
            sprintf(uart1_send_buf, "-----------------------------------------------\r\n"), uart1_transmit();
            sprintf(uart1_send_buf, "NRF:\r\n"), uart1_transmit();
            sprintf(uart1_send_buf, "received hex:"),uart1_transmit();
            nrf_print();
            sprintf(uart1_send_buf, "\r\n"),uart1_transmit();
            sprintf(uart1_send_buf, "error_state:%d\r\n", nrf_error_state), uart1_transmit();
            sprintf(uart1_send_buf, "velocity:%.2f   angle:%.2f\r\n", four_dirction_velocity, four_dirction_angular), uart1_transmit();
            sprintf(uart1_send_buf, "-----------------------------------------------\r\n"), uart1_transmit();
            sprintf(uart1_send_buf, "SYS:\r\n"), uart1_transmit();
            sprintf(uart1_send_buf, "LED color:%s\r\n", led_color_to_string_uart(car_led_color)), uart1_transmit();
            sprintf(uart1_send_buf, "Buzzer State:%d  set_freq:%d  real_freq:%d\r\n", car_buzzer_param.status, car_buzzer_param.set_freq, car_buzzer_param.real_freq), uart1_transmit();
            sprintf(uart1_send_buf, "current time:\r\n"), uart1_transmit();
            sprintf(uart1_send_buf, "main:%4dms \r\n", main_all_tick), uart1_transmit();
            sprintf(uart1_send_buf, "key_read:%4dms      key_process:%4dms\r\n", main_during_tick[0], main_during_tick[1]), uart1_transmit();
            sprintf(uart1_send_buf, "oled_process:%4dms  oled_display:%4dms\r\n", main_during_tick[2], main_during_tick[3]), uart1_transmit();
            sprintf(uart1_send_buf, "nrf_process:%4dms   led_process:%4dms\r\n", main_during_tick[4], main_during_tick[5]), uart1_transmit();
            sprintf(uart1_send_buf, "uart1_process:%4dms position_main_process:%4dms\r\n", main_during_tick[6], main_during_tick[7]), uart1_transmit();
            sprintf(uart1_send_buf, "drive:%4dms         Update_Map:%4dms\r\n", main_during_tick[8], main_during_tick[9]), uart1_transmit();
            sprintf(uart1_send_buf, "max time:\r\n"), uart1_transmit();
            sprintf(uart1_send_buf, "main:%4dms \r\n", main_all_tick_max), uart1_transmit();
            sprintf(uart1_send_buf, "key_read:%4dms      key_process:%4dms\r\n", main_during_tick_max[0], main_during_tick_max[1]), uart1_transmit();
            sprintf(uart1_send_buf, "oled_process:%4dms  oled_display:%4dms\r\n", main_during_tick_max[2], main_during_tick_max[3]), uart1_transmit();
            sprintf(uart1_send_buf, "nrf_process:%4dms   led_process:%4dms\r\n", main_during_tick_max[4], main_during_tick_max[5]), uart1_transmit();
            sprintf(uart1_send_buf, "uart1_process:%4dms position_main_process:%4dms\r\n", main_during_tick_max[6], main_during_tick_max[7]), uart1_transmit();
            sprintf(uart1_send_buf, "drive:%4dms         Update_Map:%4dms\r\n", main_during_tick_max[8], main_during_tick_max[9]), uart1_transmit();
            sprintf(uart1_send_buf, "-----------------------------------------------\r\n"), uart1_transmit();
            sprintf(uart1_send_buf, "Track Data:\r\n"), uart1_transmit();
            sprintf(uart1_send_buf, "state:               %d\r\n", tracker.state), uart1_transmit();
            sprintf(uart1_send_buf, "near_index:%2d       lookahead_index:%2d\r\n", tracker.near_index, tracker.lookahead_index), uart1_transmit();
            sprintf(uart1_send_buf, "current position: % .1f % .1f\r\n", car_combined_processed.CombinedPosition.x, car_combined_processed.CombinedPosition.y), uart1_transmit();
            sprintf(uart1_send_buf, "bias    position: % .1f % .1f\r\n", tracker.lookahead_bias.x, tracker.lookahead_bias.y), uart1_transmit();
            sprintf(uart1_send_buf, "target_speed:   % .1f\r\n", tracker.target_speed), uart1_transmit();
            sprintf(uart1_send_buf, "k_pure_pursuit: % .1f\r\n", tracker.k_pure_pursuit), uart1_transmit();
            sprintf(uart1_send_buf, "slowdown_start_dist: % .1f\r\n", tracker.slowdown_start_dist), uart1_transmit();
            sprintf(uart1_send_buf, "lookahead_distance:  % .1f\r\n", tracker.lookahead_distance), uart1_transmit();
            sprintf(uart1_send_buf, "stop_dist:           % .1f\r\n", tracker.stop_dist), uart1_transmit();
            sprintf(uart1_send_buf, "output  cmd:      % .1f % .1f\r\n", tracker.cmd_linear, tracker.cmd_angular), uart1_transmit();
            sprintf(uart1_send_buf, "-----------------------------------------------\r\n"), uart1_transmit();
            sprintf(uart1_send_buf, "Map Data:\r\n"), uart1_transmit();
            sprintf(uart1_send_buf, "current position: % .1f % .1f\r\n", car_combined_processed.CombinedPosition.x, car_combined_processed.CombinedPosition.y), uart1_transmit();
            sprintf(uart1_send_buf, "distance:         % .1f\r\n", vl53l1_distance), uart1_transmit();
            sprintf(uart1_send_buf, "obstacle status:  %d\r\n", obstacle_info.status), uart1_transmit();
            sprintf(uart1_send_buf, "obstacle position % .1f % .1f\r\n", obstacle_info.obstacle_position.x, obstacle_info.obstacle_position.y), uart1_transmit();
            sprintf(uart1_send_buf, "-----------------------------------------------\r\n"), uart1_transmit();
            sprintf(uart1_send_buf, "Solve Data:\r\n"), uart1_transmit();
            sprintf(uart1_send_buf, "car_goal_position % .1f % .1f\r\n", car_goal_position.x, car_goal_position.y), uart1_transmit();
            sprintf(uart1_send_buf, "solves_count      %d\r\n", solves_count), uart1_transmit();
            sprintf(uart1_send_buf, "-----------------------------------------------\r\n"), uart1_transmit();
        }
        else if (strcmp(uart1_rxdata, "track") == 0)
        {
            memset(uart1_rxdata, 0, UART1_RX_SIZE);
            uart1_rx_p = 0;
            sprintf(uart1_send_buf, "-----------------------------------------------\r\n"), uart1_transmit();
            sprintf(uart1_send_buf, "current_track_length %d\r\n", current_track_length), uart1_transmit();
            for (int i = 0; i < TRACK_LEN_MAX; i += 2)
            {
                // 逻辑检查：如果第二列的索引超出了数组范围，需要做特殊处理
                if (i + 1 < TRACK_LEN_MAX)
                {
                    // 1. 正常显示两列数据
                    sprintf(uart1_send_buf, "%2d: %.1f, %.1f  |  %2d: %.1f, %.1f\r\n",
                            i, tracker.track[i].x, tracker.track[i].y,
                            i + 1, tracker.track[i + 1].x, tracker.track[i + 1].y);
                }
                else
                {
                    // 2. 最后一项如果是单数，只显示左边一列
                    sprintf(uart1_send_buf, "%2d: %.1f, %.1f  |  --: ---, ---\r\n",
                            i, tracker.track[i].x, tracker.track[i].y);
                }

                // 3. 正确执行串口发送
                uart1_transmit();
            }
            sprintf(uart1_send_buf, "-----------------------------------------------\r\n"), uart1_transmit();
        }
        else if (strcmp(uart1_rxdata, "solve") == 0)
        {
            memset(uart1_rxdata, 0, UART1_RX_SIZE);
            uart1_rx_p = 0;
            sprintf(uart1_send_buf, "-----------------------------------------------\r\n"), uart1_transmit();
            sprintf(uart1_send_buf, "solves_count      %d\r\n", solves_count), uart1_transmit();
            for (int i = 0; i < MAX_SOLVE_NODES; i += 2)
            {
                // 逻辑检查：如果第二列的索引超出了数组范围，需要做特殊处理
                if (i + 1 < MAX_SOLVE_NODES)
                {
                    // 1. 正常显示两列数据
                    sprintf(uart1_send_buf, "%2d: %.1f, %.1f, %s|  %2d: %.1f, %.1f, %s\r\n",
                            i, solves[i].point.x, solves[i].point.y, get_solve_state_string_uart(solves[i].solvestate),
                            i + 1, solves[i + 1].point.x, solves[i + 1].point.y, get_solve_state_string_uart(solves[i + 1].solvestate));
                }
                else
                {
                    // 2. 最后一项如果是单数，只显示左边一列
                    sprintf(uart1_send_buf, "%2d: %.1f, %.1f, %s|  --: ---, ---, ---\r\n",
                            i, solves[i].point.x, solves[i].point.y, get_solve_state_string_uart(solves[i].solvestate));
                }

                // 3. 正确执行串口发送
                uart1_transmit();
            }
            sprintf(uart1_send_buf, "-----------------------------------------------\r\n"), uart1_transmit();
        }
        else if (strcmp(uart1_rxdata, "map") == 0)
        {
            memset(uart1_rxdata, 0, UART1_RX_SIZE);
            uart1_rx_p = 0;
            // 提醒：200*200 = 40,000 字节，UART 传输压力极大
            sprintf(uart1_send_buf, "-----------------------------------------------\r\n"), uart1_transmit();
            sprintf(uart1_send_buf, "--- Map Start ---\r\n"), uart1_transmit();

            for (int y = 0; y < 200; y++)
            {
                for (int x = 0; x < 200; x++)
                {
                    // 1. 获取地图数值 (假设 1 是空闲，0/3 是障碍)
                    uint8_t val = GetMapValue_ByPixel(x, y);

                    // 2. 将数值映射为紧凑字符，节省带宽
                    // 空闲用 '.'，障碍用 '#'，其他用数字
                    char c;
                    if (val == 1)
                        c = '.';
                    else if (val == 0)
                        c = '#';
                    else
                        c = val + '0';

                    // 3. 填充并发送。
                    // 注意：不要在这里每次都 sprintf，太慢！直接操作缓冲区。
                    uart1_send_buf[0] = c;
                    uart1_send_buf[1] = '\0';
                    uart1_transmit();
                }

                // 4. 每一行打印完，换行
                sprintf(uart1_send_buf, "\r\n");
                uart1_transmit();
            }

            sprintf(uart1_send_buf, "--- Map End ---\r\n");
            uart1_transmit();
            sprintf(uart1_send_buf, "-----------------------------------------------\r\n"), uart1_transmit();
        }
    }
    if (HAL_GetTick() - last_rx_time > 50)
    {
        memset(uart1_rxdata, 0, UART1_RX_SIZE);
        uart1_rx_p = 0;
    }
}
