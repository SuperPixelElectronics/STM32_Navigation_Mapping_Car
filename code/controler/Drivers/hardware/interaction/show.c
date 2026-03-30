#include "show.h"

char oled_buf[30];
float arrow_length = 0.2; // 单位：m
typedef enum
{
    OLED_X_TYPE = 0U,
    OLED_Y_TYPE
} OLED_CoordinateType;
// private start--------------------------
int oled_transform_axis(float rectangular_axis, int return_type)
{
    int k_x = METER_PIXEL, b_x = SHOW_CENTER_X, k_y = -METER_PIXEL, b_y = SHOW_CENTER_Y;

    if (return_type == OLED_X_TYPE)
    {
        // 使用四舍五入，而不是截断
        float pixel_value = k_x * rectangular_axis + b_x;
        int cartesian_x = (int)(pixel_value + 0.5f); // 四舍五入
        return cartesian_x;
    }
    else if (return_type == OLED_Y_TYPE)
    {
        float pixel_value = k_y * rectangular_axis + b_y;
        int cartesian_y = (int)(pixel_value + 0.5f); // 四舍五入
        return cartesian_y;
    }
    return 0;
}

const char *led_color_to_string(Led_Color color)
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

const char *nrf_state_to_string(NRF_statusType nrf_state)
{
    switch (nrf_state)
    {
    case NRF_IDLE:
        return "IDLE";
    case NRF_WAIT_FOR_PARAM:
        return "WAIT_FOR_PARAM";
    case NRF_WAIT_FOR_MAP:
        return "WAIT_FOR_MAP";
    case NRF_WAIT_FOR_TRACK:
        return "WAIT_FOR_TRACK";
    default:
        return "UNKNOWN";
    }
}

/**
 * @brief 将序号转换为2位紧凑字符 (00-99, 100+变为A0, A1...)
 * @param buf 至少3字节的缓冲区
 * @param index 原始序号 (0-255)
 */
void get_compact_index_str(char *buf, uint16_t index)
{
    if (index < 100)
    {
        sprintf(buf, "%02d", index); // 0-99 正常显示
    }
    else
    {
        // 100-109 -> A0-A9
        // 110-119 -> B0-B9 ...
        char major = (index / 10) - 10 + 'A'; // 计算字母 A, B, C...
        int minor = index % 10;               // 计算个位数

        // 兜底保护，防止超过 Z
        if (major > 'Z')
            major = 'Z';

        buf[0] = major;
        buf[1] = minor + '0';
        buf[2] = '\0';
    }
}

void oled_show_block(uint8_t X, uint8_t Y, uint8_t Width, uint8_t Height, Single_Curser_Tpye curser)
{
    if (curser.selected == DISABLE)
    {
        OLED_DrawRectangle(X, Y, Width, Height, 0);
    }
    else if (curser.selected == ENABLE)
    {
        OLED_ReverseArea(X, Y, Width, Height);
    }
}
void oled_draw_grid()
{
    for (int i = 0; i < OLED_MAX_HIGHT; i++)
    {
        for (int j = 0; j < OLED_MAX_WIDTH; j++)
        {
            if (abs(i - SHOW_CENTER_Y) % METER_PIXEL == 0 && abs(j - SHOW_CENTER_X) % METER_PIXEL == 0)
            {
                OLED_DrawPoint(j, i);
            }
        }
    }
    sprintf(oled_buf, "1grid = 1m");
    OLED_ShowString(0, 56, oled_buf, OLED_6X8);
}
void oled_draw_car_arrow(float x, float y, float theta)
{
    OLED_DrawCircle(oled_transform_axis(x, OLED_X_TYPE), oled_transform_axis(y, OLED_Y_TYPE), 2, 1);
    OLED_DrawLine(oled_transform_axis(x, OLED_X_TYPE),
                  oled_transform_axis(y, OLED_Y_TYPE),
                  oled_transform_axis((x + arrow_length * cosf(theta)), OLED_X_TYPE),
                  oled_transform_axis((y + arrow_length * sinf(theta)), OLED_Y_TYPE));
}
/**
 * @brief 全图渲染模式：将200x200地图完整投影至OLED
 * @note  此函数会遍历整个地图数组（4万个点），对执行效率有一定要求
 */
void OLED_ShowFullMap(void)
{
    // 遍历地图所有的格点 (gx, gy)
    for (int gx = 0; gx < CAR_MAP_SIZE; gx++)
    {
        for (int gy = 0; gy < CAR_MAP_SIZE; gy++)
        {
            // // 1. 提取当前格点状态 (2bit)
            // int byte_idx = gy / (8 / BIT_PER_POINT);
            // int bit_pos = (gy % (8 / BIT_PER_POINT)) * BIT_PER_POINT;
            // uint8_t state = (car_map[gx][byte_idx] >> bit_pos) & 0x03;

            // 1. 提取当前格点状态 (2bit)
            int byte_idx = gx / (8 / BIT_PER_POINT);
            int bit_pos = (gx % (8 / BIT_PER_POINT)) * BIT_PER_POINT;
            uint8_t state = (car_map[gy][byte_idx] >> bit_pos) & 0x03;

            // 2. 只有占据(2)或静态障碍(3)才画点
            if (state == map_show_level)
            {
                // 3. 将索引转回物理坐标 (单位: m)
                // 这里的偏移量 CAR_MAP_SIZE/2 对应地图的中心点(0,0)
                float world_x = (gx - CAR_MAP_SIZE / 2) * POINT_GAP;
                float world_y = -(gy - CAR_MAP_SIZE / 2) * POINT_GAP;

                // 4. 调用你的坐标转换函数映射到 OLED 像素点
                // 只要 METER_PIXEL 比例设置得当，200个点会被压缩进屏幕范围内
                int screen_x = oled_transform_axis(world_x, OLED_X_TYPE);
                int screen_y = oled_transform_axis(world_y, OLED_Y_TYPE);

                // 5. 画点（OLED_DrawPoint 内部已有 0~127, 0~63 的边界检查）
                OLED_DrawPoint((uint8_t)screen_x, (uint8_t)screen_y);
            }
        }
    }
}

#define MAP_CEN (CAR_MAP_SIZE / 2)
#define Q_FACTOR 12 // 定点数放大倍数 2^12 = 4096
void OLED_ShowFullMap_FixPoint(void)
{
    // 1. 预计算缩放因子 K
    float K_f = POINT_GAP * (float)METER_PIXEL;
    int32_t K_fixed = (int32_t)(K_f * (1 << Q_FACTOR));

    // 2. 修正平移系数 B
    // Bx = 64 - 100 * K
    int32_t Bx_fixed = (int32_t)((SHOW_CENTER_X - MAP_CEN * K_f) * (1 << Q_FACTOR));
    // By = 32 - 100 * K  (修正点：让 gy 增大时 sy 也增大)
    int32_t By_fixed = (int32_t)((SHOW_CENTER_Y - MAP_CEN * K_f) * (1 << Q_FACTOR));

    for (int gy = 0; gy < CAR_MAP_SIZE; gy++)
    {
        // 3. 计算 sy：(gy * K + By) >> Q_FACTOR
        int sy = (gy * K_fixed + By_fixed) >> Q_FACTOR;

        // 垂直边界检查
        if (sy < 0 || sy >= OLED_MAX_HIGHT)
            continue;

        for (int gx = 0; gx < CAR_MAP_SIZE; gx++)
        {
            // 提取 2bit 状态
            int byte_idx = gx / 4;
            int bit_pos = (gx % 4) * 2;
            uint8_t state = (car_map[gy][byte_idx] >> bit_pos) & 0x03;

            if (state == map_show_level)
            {
                // 4. 计算 sx：(gx * K + Bx) >> Q_FACTOR
                int sx = (gx * K_fixed + Bx_fixed) >> Q_FACTOR;

                if (sx >= 0 && sx < OLED_MAX_WIDTH)
                {
                    OLED_DrawPoint((uint8_t)sx, (uint8_t)sy);
                }
            }
        }
    }
}

/**
 * @brief 使用压缩地图进行渲染
 * @param x_offset 屏幕显示的起始偏移，例如设为 32，使 64 宽的地图居中显示 (128屏幕)
 * @note  只有当METER_PIXEL=32，SHOW_CENTER_X=64，SHOW_CENTER_Y=32时，也就是设定为显示整张地图时，压缩地图相关代码才能正常工作
 */
void OLED_ShowCompressedMap(uint8_t x_offset)
{
    for (uint8_t y = 0; y < COMPRESSED_SIZE; y++)
    {
        for (uint8_t x = 0; x < COMPRESSED_SIZE; x++)
        {
            // 1. 定位压缩地图中的 2bit 数据
            int byte_idx = x / 4;
            int bit_pos = (x % 4) * 2;

            // 2. 提取状态值
            uint8_t state = (car_map_compressed[y][byte_idx] >> bit_pos) & 0x03;

            // 3. 根据指定的层级画点
            if (state == map_show_level)
            {
                // 这里加 x_offset 是为了让地图显示在屏幕中央或指定位置
                OLED_DrawPoint(x + x_offset, y);
            }

            // 可选：在此处加入小车图标或中心点的逻辑
            // if (x == 32 && y == 32) OLED_DrawCircle(x + x_offset, y, 2, 1);
        }
    }
    OLED_ShowString(0, 8, "Cache", OLED_6X8);
}

// privare end------------------------
void show_init()
{
    OLED_Init();
    oled_page = MENU_PAGE;
    oled_curser.menu_page.maxnum = OLED_PAGE_COUNT_TWOMORE - 2;
    oled_curser.control_page.maxnum = 12;
    oled_curser.solve_graph_page.maxnum = 2;
    oled_curser.track_data_page.maxnum = 1;
    oled_curser.led_buzzer_page.maxnum = 4;
}

void oled_process()
{
    OLED_Clear();
    if (oled_page == MENU_PAGE)
    {
        sprintf(oled_buf, "Control");
        OLED_ShowString(0, 0, oled_buf, OLED_6X8);
        sprintf(oled_buf, "Solve Graph");
        OLED_ShowString(0, 8, oled_buf, OLED_6X8);
        sprintf(oled_buf, "Track Data");
        OLED_ShowString(0, 16, oled_buf, OLED_6X8);
        sprintf(oled_buf, "MPU Data");
        OLED_ShowString(0, 24, oled_buf, OLED_6X8);
        sprintf(oled_buf, "LED/Buzzer");
        OLED_ShowString(0, 32, oled_buf, OLED_6X8);
        sprintf(oled_buf, "NRF     ");
        OLED_ShowString(0, 40, oled_buf, OLED_6X8);
        if (oled_curser.menu_page.curser < 8)
        {
            OLED_ReverseArea(0, oled_curser.menu_page.curser * 8, 64, 8);
        }
        else
        {
            OLED_ReverseArea(64, (oled_curser.menu_page.curser - 8) * 8, 64, 8);
        }
    }
    else if (oled_page == CONTROL_PAGE)
    {
        sprintf(oled_buf, "pos:% .2f % .2f % .2f", car_combined_processed.CombinedPosition.x, car_combined_processed.CombinedPosition.y, car_combined_processed.CombinedPosition.theta);
        OLED_ShowString(0, 0, oled_buf, OLED_6X8);
        sprintf(oled_buf, "v_set:% .2f % .2f", car_feedback_velocity.v, car_feedback_velocity.angular);
        OLED_ShowString(0, 8, oled_buf, OLED_6X8);
        sprintf(oled_buf, "goal:% .2f % .2f", car_goal_position_feedback.x, car_goal_position_feedback.y);
        OLED_ShowString(0, 16, oled_buf, OLED_6X8);
        sprintf(oled_buf, "JOY");
        OLED_ShowString(0, 24, oled_buf, OLED_6X8);
        if (map_detect_switch_feedback == DISABLE)
        {
            sprintf(oled_buf, "OFF");
            OLED_ShowString(4 * 6, 24, oled_buf, OLED_6X8);
        }
        else if (map_detect_switch_feedback == ENABLE)
        {
            sprintf(oled_buf, "ON");
            OLED_ShowString(4 * 6, 24, oled_buf, OLED_6X8);
        }
        if (tracker.state_feedback == DISABLE)
        {
            sprintf(oled_buf, "OFF");
            OLED_ShowString(8 * 6, 24, oled_buf, OLED_6X8);
        }
        else if (tracker.state_feedback == ENABLE)
        {
            sprintf(oled_buf, "ON");
            OLED_ShowString(8 * 6, 24, oled_buf, OLED_6X8);
        }
        sprintf(oled_buf, "Process  Solve");
        OLED_ShowString(0, 32, oled_buf, OLED_6X8);
        sprintf(oled_buf, "refres info map track");
        OLED_ShowString(0, 40, oled_buf, OLED_6X8);
        sprintf(oled_buf, "NRF:%s", nrf_state_to_string(nrf_running_status));
        OLED_ShowString(0, 48, oled_buf, OLED_6X8);
        switch (oled_curser.control_page.curser)
        {
        case 0:
            oled_show_block(6 * 6, 1 * 8, 5 * 6, 1 * 8, oled_curser.control_page);
            break;
        case 1:
            oled_show_block(12 * 6, 1 * 8, 5 * 6, 1 * 8, oled_curser.control_page);
            break;
        case 2:
            oled_show_block(5 * 6, 2 * 8, 5 * 6, 1 * 8, oled_curser.control_page);
            break;
        case 3:
            oled_show_block(11 * 6, 2 * 8, 5 * 6, 1 * 8, oled_curser.control_page);
            break;
        case 4:
            oled_show_block(0 * 6, 3 * 8, 3 * 6, 1 * 8, oled_curser.control_page);
            break;
        case 5:
            oled_show_block(4 * 6, 3 * 8, 3 * 6, 1 * 8, oled_curser.control_page);
            break;
        case 6:
            oled_show_block(8 * 6, 3 * 8, 3 * 6, 1 * 8, oled_curser.control_page);
            break;
        case 7:
            oled_show_block(0 * 6, 4 * 8, 7 * 6, 1 * 8, oled_curser.control_page);
            break;
        case 8:
            oled_show_block(9 * 6, 4 * 8, 5 * 6, 1 * 8, oled_curser.control_page);
            break;
        case 9:
            oled_show_block(7 * 6, 5 * 8, 4 * 6, 1 * 8, oled_curser.control_page);
            break;
        case 10:
            oled_show_block(12 * 6, 5 * 8, 3 * 6, 1 * 8, oled_curser.control_page);
            break;
        case 11:
            oled_show_block(16 * 6, 5 * 8, 5 * 6, 1 * 8, oled_curser.control_page);
            break;
        default:
            break;
        }
    }
    else if (oled_page == SOLVE_GRAPH_PAGE)
    {
        sprintf(oled_buf, "PATH");
        OLED_ShowString(0, 0, oled_buf, OLED_6X8);

        sprintf(oled_buf, "lev=%d", map_show_level);
        OLED_ShowString(64, 56, oled_buf, OLED_6X8);
        sprintf(oled_buf, "EndP");
        OLED_ShowString(64 + 4 * 8, 56, oled_buf, OLED_6X8);

        switch (oled_curser.solve_graph_page.curser)
        {
        case 0:
            oled_show_block(64, 56, 5 * 6, 8, oled_curser.solve_graph_page);
            break;
        case 1:
            oled_show_block(64 + 4 * 8, 56, 4 * 6, 8, oled_curser.solve_graph_page);
            break;
        default:
            break;
        }

        oled_draw_grid();
        
        if (car_map_compressed_complete == ENABLE)
        {
            OLED_ShowCompressedMap(32);
        }
        else
        {
            OLED_ShowFullMap_FixPoint();
        }

        oled_draw_car_arrow(car_combined_processed.CombinedPosition.x, car_combined_processed.CombinedPosition.y, car_combined_processed.CombinedPosition.theta);

        OLED_DrawTriangle(oled_transform_axis(car_goal_position_feedback.x, OLED_X_TYPE), oled_transform_axis(car_goal_position_feedback.y, OLED_Y_TYPE) - 4,
                          oled_transform_axis(car_goal_position_feedback.x, OLED_X_TYPE) - 3, oled_transform_axis(car_goal_position_feedback.y, OLED_Y_TYPE) + 2,
                          oled_transform_axis(car_goal_position_feedback.x, OLED_X_TYPE) + 3, oled_transform_axis(car_goal_position_feedback.y, OLED_Y_TYPE) + 2,
                          0);

        for (int i = 0; i < current_track_length; i++)
        {
            OLED_DrawCircle(oled_transform_axis(car_track[i].x, OLED_X_TYPE), oled_transform_axis(car_track[i].y, OLED_Y_TYPE), 1, 1);
        }
        OLED_DrawRectangle(oled_transform_axis(car_track[tracker.near_index].x, OLED_X_TYPE) - 2, oled_transform_axis(car_track[tracker.near_index].y, OLED_Y_TYPE) - 2, 5, 5, 0);
        OLED_DrawCircle(oled_transform_axis(car_track[tracker.lookahead_index].x, OLED_X_TYPE), oled_transform_axis(car_track[tracker.lookahead_index].y, OLED_Y_TYPE), 3, 0);
        OLED_DrawLine(oled_transform_axis(car_combined_processed.CombinedPosition.x, OLED_X_TYPE),
                      oled_transform_axis(car_combined_processed.CombinedPosition.y, OLED_Y_TYPE),
                      oled_transform_axis(car_track[tracker.lookahead_index].x, OLED_X_TYPE),
                      oled_transform_axis(car_track[tracker.lookahead_index].y, OLED_Y_TYPE));

        
    }
    else if (oled_page == TRACK_DATA_PAGE)
    {
        // 计算当前页的起始数据索引
        uint16_t start_idx = track_data_page * 8;

        // 1. 循环绘制每一行数据
        for (uint8_t i = 0; i < 8; i++)
        {
            uint16_t current_data_idx = start_idx + i;

            // 如果数据超出数组大小，清除该行并跳过
            if (current_data_idx >= TRACK_LEN_MAX)
            {
                sprintf(oled_buf, "END HERE");
                OLED_ShowString(0, 56, oled_buf, OLED_6X8);
                continue;
            }

            // 格式化字符串：[序号] [X] [Y]
            // 序号使用 current_data_idx + 1，方便人类阅读
            // 1. 获取紧凑序号 (例如 105 -> "A5")
            char idx_str[3];
            get_compact_index_str(idx_str, current_data_idx + 1);

            // 2. 格式化输出
            // 去掉冒号后的空格，进一步压缩空间
            sprintf(oled_buf, " %s% .1f% .1f",
                    idx_str,
                    car_track[current_data_idx].x,
                    car_track[current_data_idx].y);

            OLED_ShowString(0, i * 8, oled_buf, OLED_6X8);

            if (current_data_idx == current_track_length - 1)
            {
                OLED_DrawLine(0, i * 8 + 7, 68, i * 8 + 7);
            }
        }

        // 2. 绘制当前追踪进度标识
        int16_t track_near_idx = (int16_t)tracker.near_index;
        int16_t track_look_idx = (int16_t)tracker.lookahead_index;

        // 只有当追踪点在当前页面的显示范围内时，才绘制标识
        if (track_near_idx >= start_idx && track_near_idx < (start_idx + 8))
        {
            uint8_t relative_row = track_near_idx - start_idx;
            // 在该行最左侧或指定位置绘制标识，此处复用你的矩形/圆圈逻辑
            OLED_DrawRectangle(0, relative_row * 8, 6, 6, 0);
        }
        // 只有当追踪点在当前页面的显示范围内时，才绘制标识
        if (track_look_idx >= start_idx && track_look_idx < (start_idx + 8))
        {
            uint8_t relative_row = track_look_idx - start_idx;
            // 在该行最左侧或指定位置绘制标识，此处复用你的矩形/圆圈逻辑
            OLED_DrawCircle(3, relative_row * 8 + 3, 3, 0); // 1表示实心或高亮
        }

        sprintf(oled_buf, "page %2d", track_data_page);
        OLED_ShowString(64 + 4, 0, oled_buf, OLED_6X8);
        sprintf(oled_buf, "len %3d", current_track_length);
        OLED_ShowString(64 + 4, 8, oled_buf, OLED_6X8);
        sprintf(oled_buf, "near %2d", tracker.near_index);
        OLED_ShowString(64 + 4, 16, oled_buf, OLED_6X8);
        sprintf(oled_buf, "look %2d", tracker.lookahead_index);
        OLED_ShowString(64 + 4, 24, oled_buf, OLED_6X8);
        sprintf(oled_buf, "c% .1f % .1f", car_combined_processed.CombinedPosition.x, car_combined_processed.CombinedPosition.y);
        OLED_ShowString(64 + 4, 32, oled_buf, OLED_6X8);

        if (tracker.state_feedback == DISABLE)
        {
            sprintf(oled_buf, "OF");
            OLED_ShowString(64 + 4, 40, oled_buf, OLED_6X8);
        }
        else if (tracker.state_feedback == ENABLE)
        {
            sprintf(oled_buf, "ON");
            OLED_ShowString(64 + 4, 40, oled_buf, OLED_6X8);
        }

        OLED_DrawLine(67, 0, 67, 64);

        oled_show_block(68, 0 * 8, 7 * 6, 8, oled_curser.track_data_page);
    }
    else if (oled_page == MPU6050_DATA_PAGE)
    {
        sprintf(oled_buf, "MPU6050");
        OLED_ShowString(0, 0, oled_buf, OLED_6X8);
        sprintf(oled_buf, "A:X% .1f Y% .1f Z% .1f", controler_mpu6050_data.acceleration.x, controler_mpu6050_data.acceleration.y, controler_mpu6050_data.acceleration.z);
        OLED_ShowString(0, 8, oled_buf, OLED_6X8);
        sprintf(oled_buf, "AV:X%4.0f Y%4.0f Z%4.0f", controler_mpu6050_data.angular_velocity.x, controler_mpu6050_data.angular_velocity.y, controler_mpu6050_data.angular_velocity.z);
        OLED_ShowString(0, 16, oled_buf, OLED_6X8);
        sprintf(oled_buf, "AN:X%4.0f Y%4.0f Z%4.0f", controler_mpu6050_data.angle.x, controler_mpu6050_data.angle.y, controler_mpu6050_data.angle.z);
        OLED_ShowString(0, 24, oled_buf, OLED_6X8);
        sprintf(oled_buf, "M:X %5.0f Y %5.0f ", controler_mpu6050_data.magnet.x, controler_mpu6050_data.magnet.y);
        OLED_ShowString(0, 32, oled_buf, OLED_6X8);
        sprintf(oled_buf, "  Z %5.0f T:% .1f", controler_mpu6050_data.magnet.z, controler_mpu6050_data.temperature);
        OLED_ShowString(0, 40, oled_buf, OLED_6X8);
        sprintf(oled_buf, "ANF:X% .1f Y% .1f Z% .1f", controler_mpu6050_processed.angle_remove_first.x, controler_mpu6050_processed.angle_remove_first.y, controler_mpu6050_processed.angle_remove_first.z);
        OLED_ShowString(0, 48, oled_buf, OLED_6X8);
    }
    else if (oled_page == LED_BUZZER_PAGE)
    {
        sprintf(oled_buf, "LED Color: %s", led_color_to_string(controler_led_color));
        OLED_ShowString(0, 0, oled_buf, OLED_6X8);
        if (controler_buzzer_param.status == ENABLE)
        {
            sprintf(oled_buf, "Buzzer: ON");
            OLED_ShowString(0, 8, oled_buf, OLED_6X8);
        }
        else if (controler_buzzer_param.status == DISABLE)
        {
            sprintf(oled_buf, "Buzzer: OFF");
            OLED_ShowString(0, 8, oled_buf, OLED_6X8);
        }
        sprintf(oled_buf, "freq:%4d Vol:%3d", controler_buzzer_param.set_freq, controler_buzzer_param.set_volume);
        OLED_ShowString(0, 16, oled_buf, OLED_6X8);
        sprintf(oled_buf, "TrackKey %2.f %2.f", controler_track_key.track_key_value.x, controler_track_key.track_key_value.y);
        OLED_ShowString(0, 24, oled_buf, OLED_6X8);

        switch (oled_curser.led_buzzer_page.curser)
        {
        case 0:
            oled_show_block(11 * 6, 0 * 8, 7 * 6, 8, oled_curser.led_buzzer_page);
            break;
        case 1:
            oled_show_block(8 * 6, 1 * 8, 3 * 6, 8, oled_curser.led_buzzer_page);
            break;
        case 2:
            oled_show_block(5 * 6, 2 * 8, 4 * 6, 8, oled_curser.led_buzzer_page);
            break;
        case 3:
            oled_show_block(14 * 6, 2 * 8, 3 * 6, 8, oled_curser.led_buzzer_page);
            break;
        default:
            break;
        }
    }
    else if (oled_page == NRF_PAGE)
    {
        if (nrf_error_state == RESET)
        {
            sprintf(oled_buf, "NRF received:");
            OLED_ShowString(0, 0, oled_buf, OLED_6X8);
        }
        else
        {
            sprintf(oled_buf, "NRF received:   ERROR");
            OLED_ShowString(0, 0, oled_buf, OLED_6X8);
        }

        uint8_t i, j;
        for (i = 0; i < 32; i += 7) // 步长为7，因为你一行显示7个
        {
            // 清空缓冲区，保证安全
            memset(oled_buf, 0, sizeof(oled_buf));

            // 格式化当前行的 7 个数据
            // 注意：如果剩余数据不足7个，需要特殊处理防止越界
            int len = 0;
            for (j = 0; j < 7 && (i + j) < 32; j++)
            {
                len += sprintf(oled_buf + len, "%02X ", nrf_receive_buf[i + j]);
            }

            // 在 OLED 不同行显示 (第一行坐标0, 第二行8...)
            // (i / 7) 代表当前是第几行
            OLED_ShowString(0, (i / 7) * 8 + 8, oled_buf, OLED_6X8);
        }
    }

    if (controler_key[4].press_time != 0)
    {
        OLED_DrawLine(0, 63, (controler_key[4].press_time * OLED_MAX_WIDTH) / KEY_LONG_CNT, 63);
    }
}

void oled_display()
{
    OLED_Update();
}
