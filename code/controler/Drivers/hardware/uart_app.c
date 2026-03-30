#include "uart_app.h"

uint8_t uart1_rxdat;               // 收到的单个数据
volatile unsigned char uart1_rx_p; // 缓冲区指针

// private start-------------------------

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
    case BREATH:
        return "BREATH";
    default:
        return "UNKNOWN";
    }
}

/**
 * @brief 把缓冲区的数据通过串口发送出去
 */
static void uart1_transmit()
{
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
// private end-----------------------------

/**
 * @brief 串口接收
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
            sprintf(uart1_send_buf, "|                 Car Data                     |\r\n"), uart1_transmit();
            sprintf(uart1_send_buf, "-----------------------------------------------\r\n"), uart1_transmit();
            sprintf(uart1_send_buf, "Moving Data\r\n"), uart1_transmit();
            sprintf(uart1_send_buf, "V:%.2f  angular:%.2f\r\n", car_feedback_velocity.v, car_feedback_velocity.angular), uart1_transmit();
            sprintf(uart1_send_buf, "-----------------------------------------------\r\n"), uart1_transmit();
            sprintf(uart1_send_buf, "CombinedProcessed \r\n"), uart1_transmit();
            sprintf(uart1_send_buf, "X:%.2f  Y:%.2f  theta:%.2f\r\n", car_combined_processed.CombinedPosition.x, car_combined_processed.CombinedPosition.y, car_combined_processed.CombinedPosition.theta), uart1_transmit();
            sprintf(uart1_send_buf, "-----------------------------------------------\r\n"), uart1_transmit();
            sprintf(uart1_send_buf, "NRF:\r\n"), uart1_transmit();
            sprintf(uart1_send_buf, "received:%s\r\n", nrf_receive_buf), uart1_transmit();
            sprintf(uart1_send_buf, "error_state:%d\r\n", nrf_error_state), uart1_transmit();
            sprintf(uart1_send_buf, "velocity_stright:%.2f   velocity_angle:%.2f   angle:%.2f\r\n", four_dirction_velocity_stright,four_dirction_velocity_turning, four_dirction_angular), uart1_transmit();
            sprintf(uart1_send_buf, "-----------------------------------------------\r\n"), uart1_transmit();
            sprintf(uart1_send_buf, "Track Data:\r\n"), uart1_transmit();
            sprintf(uart1_send_buf, "state:               %d\r\n", tracker.state_feedback), uart1_transmit();
            sprintf(uart1_send_buf, "near_index:%2d       lookahead_index:%2d\r\n", tracker.near_index, tracker.lookahead_index), uart1_transmit();
            sprintf(uart1_send_buf, "current position: % .1f % .1f\r\n", car_combined_processed.CombinedPosition.x, car_combined_processed.CombinedPosition.y), uart1_transmit();
            sprintf(uart1_send_buf, "-----------------------------------------------\r\n"), uart1_transmit();
            sprintf(uart1_send_buf, "Solve Data:\r\n"), uart1_transmit();
            sprintf(uart1_send_buf, "car_goal_position_feedback % .1f % .1f\r\n", car_goal_position_feedback.x, car_goal_position_feedback.y), uart1_transmit();
            sprintf(uart1_send_buf, "-----------------------------------------------\r\n"), uart1_transmit();

            sprintf(uart1_send_buf, "-----------------------------------------------\r\n"), uart1_transmit();
            sprintf(uart1_send_buf, "|            Controler Data                    |\r\n"), uart1_transmit();
            sprintf(uart1_send_buf, "-----------------------------------------------\r\n"), uart1_transmit();
            sprintf(uart1_send_buf, "Mpu6050Data        time:%d\r\n", controler_mpu6050_data.time), uart1_transmit();
            sprintf(uart1_send_buf, "temperature:%.2f   X     Y     Z\r\n", controler_mpu6050_data.temperature), uart1_transmit();
            sprintf(uart1_send_buf, "acceleration:     %.2f  %.2f  %.2f\r\n", controler_mpu6050_data.acceleration.x, controler_mpu6050_data.acceleration.y, controler_mpu6050_data.acceleration.z), uart1_transmit();
            sprintf(uart1_send_buf, "angular_velocity: %.2f  %.2f  %.2f\r\n", controler_mpu6050_data.angular_velocity.x, controler_mpu6050_data.angular_velocity.y, controler_mpu6050_data.angular_velocity.z), uart1_transmit();
            sprintf(uart1_send_buf, "angle:            %.2f  %.2f  %.2f\r\n", controler_mpu6050_data.angle.x, controler_mpu6050_data.angle.y, controler_mpu6050_data.angle.z), uart1_transmit();
            sprintf(uart1_send_buf, "magnet:           %.2f  %.2f  %.2f\r\n", controler_mpu6050_data.magnet.x, controler_mpu6050_data.magnet.y, controler_mpu6050_data.magnet.z), uart1_transmit();
            sprintf(uart1_send_buf, "-----------------------------------------------\r\n"), uart1_transmit();
            sprintf(uart1_send_buf, "SYS:\r\n"), uart1_transmit();
            sprintf(uart1_send_buf, "LED color:%s\r\n", led_color_to_string_uart(controler_led_color)), uart1_transmit();
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
                            i, car_track[i].x, car_track[i].y,
                            i + 1, car_track[i + 1].x, car_track[i + 1].y);
                }
                else
                {
                    // 2. 最后一项如果是单数，只显示左边一列
                    sprintf(uart1_send_buf, "%2d: %.1f, %.1f  |  --: ---, ---\r\n",
                            i, car_track[i].x, car_track[i].y);
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
