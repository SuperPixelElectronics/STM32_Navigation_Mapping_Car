#include "show.h"

char oled_buf[30];        // oled显示缓冲区
float arrow_length = 0.2; // 小车在地图上的箭头图标长度单位：m

typedef enum
{
    OLED_X_TYPE = 0U,
    OLED_Y_TYPE
} OLED_CoordinateType;

// private start--------------------------
/**
 * @brief 把真实物理坐标转换为OLED应该显示的位置
 * @param rectangular_axis 在真实物理坐标系(平面直角坐标系)中的x或y坐标
 * @param return_type      告诉函数传入的坐标类型，便于调用对应的转换公式
 * @return 转换后的对应坐标
 */
static int oled_transform_axis(float rectangular_axis, int return_type)
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

/**
 * @brief 将LED状态枚举转化为易读的字符串
 * @param color LED颜色
 * @return const char* 对应的描述字符串
 * @note 最开始尝试使用X-Macro(X 宏)来实现枚举进行字符化，但是实现较为复杂，因此放弃，使用switch case函数完成
 */
static const char *led_color_to_string(Led_Color color)
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
static const char *get_solve_state_string(SolveType state)
{
    switch (state)
    {
    case NO_FIND:
        return "F"; // 寻径失败
    case FIND_BACKED_SOLVE:
        return "B"; // 历史避让
    case FIND_EDGE_SOLVE:
        return "E"; // 边缘通过
    case FIND_SPACIOUS_SOLVE:
        return "S"; // 空旷通过
    default:
        return "?"; // 未知状态
    }
}

/**
 * @brief 将序号转换为2位紧凑字符 (00-99, 100+变为A0, A1...)
 * @param buf 至少3字节的缓冲区
 * @param index 原始序号 (0-255)
 */
static void get_compact_index_str(char *buf, uint16_t index)
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

/**
 * @brief 在菜单的指定位置显示光标
 * @param X 起始显示X坐标
 * @param Y 起始显示Y坐标
 * @param Width  显示宽度
 * @param Height 显示高度
 * @param curser 需要显示的光标
 * @note 函数会根据光标是否被选中来绘制不同的效果
 */
static void oled_show_block(uint8_t X, uint8_t Y, uint8_t Width, uint8_t Height, Single_Curser_Tpye curser)
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

/**
 * @brief 在屏幕上显示地图格点
 */
static void oled_draw_grid()
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

/**
 * @brief 在屏幕上显示小车的导航指针
 * @param x 小车的真实物理坐标x
 * @param y 小车的真实物理坐标y
 * @param theta 小车的航向角
 */
static void oled_draw_car_arrow(float x, float y, float theta)
{
    OLED_DrawCircle(oled_transform_axis(x, OLED_X_TYPE), oled_transform_axis(y, OLED_Y_TYPE), 2, 1);
    OLED_DrawLine(oled_transform_axis(x, OLED_X_TYPE),
                  oled_transform_axis(y, OLED_Y_TYPE),
                  oled_transform_axis((x + arrow_length * cosf(theta)), OLED_X_TYPE),
                  oled_transform_axis((y + arrow_length * sinf(theta)), OLED_Y_TYPE));
}

// /**
//  * @brief 全图渲染模式：将200x200地图完整投影至OLED
//  * @note  此函数会遍历整个地图数组（4万个点），对执行效率有一定要求
//  */
// static void OLED_ShowFullMap(void)
// {
//     // 遍历地图所有的格点 (gx, gy)
//     for (int gx = 0; gx < CAR_MAP_SIZE; gx++)
//     {
//         for (int gy = 0; gy < CAR_MAP_SIZE; gy++)
//         {
//             // // 1. 提取当前格点状态 (2bit)
//             // int byte_idx = gy / (8 / BIT_PER_POINT);
//             // int bit_pos = (gy % (8 / BIT_PER_POINT)) * BIT_PER_POINT;
//             // uint8_t state = (car_map[gx][byte_idx] >> bit_pos) & 0x03;

//             // 1. 提取当前格点状态 (2bit)
//             int byte_idx = gx / (8 / BIT_PER_POINT);
//             int bit_pos = (gx % (8 / BIT_PER_POINT)) * BIT_PER_POINT;
//             uint8_t state = (car_map[gy][byte_idx] >> bit_pos) & 0x03;

//             // 2. 只有占据(2)或静态障碍(3)才画点
//             if (state == map_show_level)
//             {
//                 // 3. 将索引转回物理坐标 (单位: m)
//                 // 这里的偏移量 CAR_MAP_SIZE/2 对应地图的中心点(0,0)
//                 float world_x = (gx - CAR_MAP_SIZE / 2) * POINT_GAP;
//                 float world_y = -(gy - CAR_MAP_SIZE / 2) * POINT_GAP;

//                 // 4. 调用你的坐标转换函数映射到 OLED 像素点
//                 // 只要 METER_PIXEL 比例设置得当，200个点会被压缩进屏幕范围内
//                 int screen_x = oled_transform_axis(world_x, OLED_X_TYPE);
//                 int screen_y = oled_transform_axis(world_y, OLED_Y_TYPE);

//                 // 5. 画点（OLED_DrawPoint 内部已有 0~127, 0~63 的边界检查）
//                 OLED_DrawPoint((uint8_t)screen_x, (uint8_t)screen_y);
//             }
//         }
//     }
// }

#define MAP_CEN (CAR_MAP_SIZE / 2)
#define Q_FACTOR 12 // 定点数放大倍数 2^12 = 4096
/**
 * @brief 定点模式全图渲染
 * @note  和普通全图渲染模式相比，速度提升
 */
static void OLED_ShowFullMap_FixPoint(void)
{
    // 1. 预计算缩放因子 K
    float K_f = POINT_GAP * (float)METER_PIXEL;
    int32_t K_fixed = (int32_t)(K_f * (1 << Q_FACTOR));

    // 2. 修正平移系数 B
    // Bx = SHOW_CENTER_X - MAP_CEN * K
    int32_t Bx_fixed = (int32_t)((SHOW_CENTER_X - MAP_CEN * K_f) * (1 << Q_FACTOR));
    // By = SHOW_CENTER_Y - MAP_CEN * K
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
static void OLED_ShowCompressedMap(uint8_t x_offset)
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

/**
 * @brief 在OLED上显示1:1地图
 */
static void OLED_ShowPureMap(void)
{
    // 1. 动态计算区块最大值（用于摇杆边界限制）
    // 向上取整计算：如果200点/OLED_MAX_WIDTH像素，需要2个区块才能看全
    map_show_area_max.x = (CAR_MAP_SIZE + 127) / OLED_MAX_WIDTH;
    map_show_area_max.y = (CAR_MAP_SIZE + 63) / OLED_MAX_HIGHT;

    // 2. 计算当前区块在地图数组中的起始坐标 (Top-Left)
    int start_gx = map_show_area.x * OLED_MAX_WIDTH;
    int start_gy = map_show_area.y * OLED_MAX_HIGHT;

    // 3. 遍历当前屏幕能容纳的 OLED_MAX_WIDTH * OLED_MAX_HIGHT 个像素点
    for (uint8_t x = 0; x < OLED_MAX_WIDTH; x++)
    {
        int current_gx = start_gx + x;

        // 越界检查：如果地图不是OLED_MAX_WIDTH的整数倍，防止读到数组外
        if (current_gx >= CAR_MAP_SIZE)
            break;

        for (uint8_t y = 0; y < OLED_MAX_HIGHT; y++)
        {
            int current_gy = start_gy + y;
            if (current_gy >= CAR_MAP_SIZE)
                break;

            // 4. 定位 2bit 数据在 car_map 中的位置
            int byte_idx = current_gx / (8 / BIT_PER_POINT);
            int bit_pos = (current_gx % (8 / BIT_PER_POINT)) * BIT_PER_POINT;

            // 提取状态值
            uint8_t state = (car_map[current_gy][byte_idx] >> bit_pos) & 0x03;

            // 5. 根据 map_show_level 决定是否画点
            // 建议这里 map_show_level 设为变量，方便调试你之前的“碎点”问题
            if (state == map_show_level)
            {
                OLED_DrawPoint(x, y);
            }

            if (current_gx == 100 && current_gy == 100)
            {
                OLED_DrawCircle(x, y, 3, 0);
            }
        }
    }
}
// privare end------------------------

/**
 * @brief OLED初始化函数
 */
void show_init()
{
    OLED_Init();
    oled_page = MENU_PAGE;
    oled_curser.menu_page.maxnum = OLED_PAGE_COUNT_TWOMORE - 2;

    oled_curser.encoder_data_page.maxnum = 4;
    oled_curser.combined_data_page.maxnum = 1;
    oled_curser.motor_control_page.maxnum = 14;
    oled_curser.led_buzzer_page.maxnum = 5;
    oled_curser.track_data_page.maxnum = 25;
    oled_curser.map_data_page.maxnum = 6;
    oled_curser.map_graph_page.maxnum = 1;
    oled_curser.map_pure_page.maxnum = 2;
    oled_curser.solve_data_page.maxnum = 4;
    oled_curser.solve_graph_page.maxnum = 2;
    map_show_area.x = 0;
    map_show_area.y = 1;
}

/**
 * @brief OLED主处理函数
 */
void oled_process()
{
    static uint16_t counter = 0; // 静态计数器，记录函数执行次数
    counter++; // 每次进入函数，计数自增
    if (counter >= oled_refresh_gap && oled_refresh_gap != 0) // 0：关闭OLED刷新
    {
        counter = 0;
        OLED_Clear();
        // 菜单--------------------------------------------------------------------------
        if (oled_page == MENU_PAGE)
        {
            sprintf(oled_buf, "Code Data");
            OLED_ShowString(0, 0, oled_buf, OLED_6X8);
            sprintf(oled_buf, "Code Graph");
            OLED_ShowString(0, 8, oled_buf, OLED_6X8);
            sprintf(oled_buf, "MPU Data");
            OLED_ShowString(0, 16, oled_buf, OLED_6X8);
            sprintf(oled_buf, "MPU Graph");
            OLED_ShowString(0, 24, oled_buf, OLED_6X8);
            sprintf(oled_buf, "Sum Data");
            OLED_ShowString(0, 32, oled_buf, OLED_6X8);
            sprintf(oled_buf, "Sum Graph");
            OLED_ShowString(0, 40, oled_buf, OLED_6X8);
            sprintf(oled_buf, "Motor   ");
            OLED_ShowString(0, 48, oled_buf, OLED_6X8);
            sprintf(oled_buf, "NRF     ");
            OLED_ShowString(0, 56, oled_buf, OLED_6X8);
            sprintf(oled_buf, "LED/Bz/sys");
            OLED_ShowString(64, 0, oled_buf, OLED_6X8);
            sprintf(oled_buf, "Track Data");
            OLED_ShowString(64, 8, oled_buf, OLED_6X8);
            sprintf(oled_buf, "TrackGraph");
            OLED_ShowString(64, 16, oled_buf, OLED_6X8);
            sprintf(oled_buf, "Map Data");
            OLED_ShowString(64, 24, oled_buf, OLED_6X8);
            sprintf(oled_buf, "Map Graph");
            OLED_ShowString(64, 32, oled_buf, OLED_6X8);
            sprintf(oled_buf, "Map Pure");
            OLED_ShowString(64, 40, oled_buf, OLED_6X8);
            sprintf(oled_buf, "SolveData");
            OLED_ShowString(64, 48, oled_buf, OLED_6X8);
            sprintf(oled_buf, "SolveGraph");
            OLED_ShowString(64, 56, oled_buf, OLED_6X8);
            if (oled_curser.menu_page.curser < 8)
            {
                OLED_ReverseArea(0, oled_curser.menu_page.curser * 8, 64, 8);
            }
            else
            {
                OLED_ReverseArea(64, (oled_curser.menu_page.curser - 8) * 8, 64, 8);
            }
        }
        // 电机编码器数据--------------------------------------------------------------------------
        else if (oled_page == ENCODER_DATA_PAGE)
        {
            sprintf(oled_buf, "Encoder");
            OLED_ShowString(0, 0, oled_buf, OLED_6X8);

            sprintf(oled_buf, "E:A% .2f B% .2f", MOTOR_A.Encoder, MOTOR_B.Encoder);
            OLED_ShowString(0, 8, oled_buf, OLED_6X8);
            sprintf(oled_buf, "V:X% .2f Z% .2f", car_encoder_processed.Encoder_Velocity.motor_speed, car_encoder_processed.Encoder_Velocity.motor_delta);
            OLED_ShowString(0, 16, oled_buf, OLED_6X8);
            sprintf(oled_buf, "P:X% .2fY% .2fT% .2f", car_encoder_processed.Encoder_Position.x, car_encoder_processed.Encoder_Position.y, car_encoder_processed.Encoder_Position.theta);
            OLED_ShowString(0, 24, oled_buf, OLED_6X8);
            sprintf(oled_buf, "speed:k% .2f b% .2f", k_motor_speed, b_motor_speed);
            OLED_ShowString(0, 32, oled_buf, OLED_6X8);
            sprintf(oled_buf, "delta:k% .2f b% .2f", k_motor_delta, b_motor_delta);
            OLED_ShowString(0, 40, oled_buf, OLED_6X8);
            switch (oled_curser.encoder_data_page.curser)
            {
            case 0:
                oled_show_block(8 * 6, 4 * 8, 4 * 6, 8, oled_curser.encoder_data_page);
                break;
            case 1:
                oled_show_block(15 * 6, 4 * 8, 4 * 6, 8, oled_curser.encoder_data_page);
                break;
            case 2:
                oled_show_block(8 * 6, 5 * 8, 4 * 6, 8, oled_curser.encoder_data_page);
                break;
            case 3:
                oled_show_block(15 * 6, 5 * 8, 4 * 6, 8, oled_curser.encoder_data_page);
                break;
            default:
                break;
            }
        }
        // 电机编码器绘图--------------------------------------------------------------------------
        else if (oled_page == ENCODER_GRAPH_PAGE)
        {
            sprintf(oled_buf, "Encoder");
            OLED_ShowString(0, 0, oled_buf, OLED_6X8);
            oled_draw_grid();
            oled_draw_car_arrow(car_encoder_processed.Encoder_Position.x, car_encoder_processed.Encoder_Position.y, car_encoder_processed.Encoder_Position.theta);
        }
        // MPU6050+磁力计数据--------------------------------------------------------------------------
        else if (oled_page == MPU6050_DATA_PAGE)
        {
            sprintf(oled_buf, "MPU6050");
            OLED_ShowString(0, 0, oled_buf, OLED_6X8);
            sprintf(oled_buf, "A:X% .1f Y% .1f Z% .1f", car_mpu6050_data.acceleration.x, car_mpu6050_data.acceleration.y, car_mpu6050_data.acceleration.z);
            OLED_ShowString(0, 8, oled_buf, OLED_6X8);
            sprintf(oled_buf, "AV:X%4.0f Y%4.0f Z%4.0f", car_mpu6050_data.angular_velocity.x, car_mpu6050_data.angular_velocity.y, car_mpu6050_data.angular_velocity.z);
            OLED_ShowString(0, 16, oled_buf, OLED_6X8);
            sprintf(oled_buf, "AN:X%4.0f Y%4.0f Z%4.0f", car_mpu6050_data.angle.x, car_mpu6050_data.angle.y, car_mpu6050_data.angle.z);
            OLED_ShowString(0, 24, oled_buf, OLED_6X8);
            // sprintf(oled_buf, "M:X %5.0f Y %5.0f ", car_mpu6050_data.magnet.x, car_mpu6050_data.magnet.y);
            // OLED_ShowString(0, 32, oled_buf, OLED_6X8);
            // sprintf(oled_buf, "  Z %5.0f T:% .1f", car_mpu6050_data.magnet.z, car_mpu6050_data.temperature);
            // OLED_ShowString(0, 40, oled_buf, OLED_6X8);

            sprintf(oled_buf, "ANG:X% .1f Y% .1f Z% .1f", car_mpu6050_processed.acceleration_no_gravity.x, car_mpu6050_processed.acceleration_no_gravity.y, car_mpu6050_processed.acceleration_no_gravity.z);
            OLED_ShowString(0, 32, oled_buf, OLED_6X8);
            sprintf(oled_buf, "V:X% .1f Y% .1f Z% .1f", car_mpu6050_processed.vel.x, car_mpu6050_processed.vel.y, car_mpu6050_processed.vel.z);
            OLED_ShowString(0, 40, oled_buf, OLED_6X8);
            sprintf(oled_buf, "P:X% .2fY% .2fZ% .2f", car_mpu6050_processed.pos.x, car_mpu6050_processed.pos.y, car_mpu6050_processed.pos.z);
            OLED_ShowString(0, 48, oled_buf, OLED_6X8);
            sprintf(oled_buf, "ANF:X% .1f Y% .1f Z% .1f", car_mpu6050_processed.angle_remove_first.x, car_mpu6050_processed.angle_remove_first.y, car_mpu6050_processed.angle_remove_first.z);
            OLED_ShowString(0, 56, oled_buf, OLED_6X8);
        }
        // MPU6050+磁力计绘图--------------------------------------------------------------------------
        else if (oled_page == MPU6050_GRAPH_PAGE)
        {
            sprintf(oled_buf, "MPU6050");
            OLED_ShowString(0, 0, oled_buf, OLED_6X8);
            oled_draw_grid();
            oled_draw_car_arrow(car_mpu6050_processed.pos.x, car_mpu6050_processed.pos.y, car_mpu6050_processed.angle_remove_first.z);
        }
        // 融合后的里程计数据--------------------------------------------------------------------------
        else if (oled_page == COMBINED_DATA_PAGE)
        {
            sprintf(oled_buf, "COMBINED");
            OLED_ShowString(0, 0, oled_buf, OLED_6X8);
            sprintf(oled_buf, "E:V% .1f A% .1f T% .1f", car_combined_processed.Encoder_Velocity.motor_speed, car_combined_processed.Encoder_Velocity.motor_delta, car_combined_processed.Encoder_Position.theta);
            OLED_ShowString(0, 8, oled_buf, OLED_6X8);
            sprintf(oled_buf, "ANF:Z% .1f", car_combined_processed.Mpu6050DataProcessed.angle_remove_first.z);
            OLED_ShowString(0, 16, oled_buf, OLED_6X8);
            sprintf(oled_buf, "P:X% .2fY% .2fT% .2f", car_combined_processed.CombinedPosition.x, car_combined_processed.CombinedPosition.y, car_combined_processed.CombinedPosition.theta);
            OLED_ShowString(0, 24, oled_buf, OLED_6X8);
            sprintf(oled_buf, "combined_k:% .2f", position_angle_combined_k);
            OLED_ShowString(0, 32, oled_buf, OLED_6X8);
            oled_show_block(12 * 6, 4 * 8, 4 * 6, 8, oled_curser.combined_data_page);
        }
        // 融合后的里程计绘图--------------------------------------------------------------------------
        else if (oled_page == COMBINED_GRAPH_PAGE)
        {
            sprintf(oled_buf, "COMBINED");
            OLED_ShowString(0, 0, oled_buf, OLED_6X8);
            oled_draw_grid();
            oled_draw_car_arrow(car_combined_processed.CombinedPosition.x, car_combined_processed.CombinedPosition.y, car_combined_processed.CombinedPosition.theta);
        }
        // 电机控制页面--------------------------------------------------------------------------
        else if (oled_page == MOTOR_CONTROL_PAGE)
        {
            if (control_mode == HAND_PWM)
            {
                sprintf(oled_buf, "Control    M=HAND_PWM");
                OLED_ShowString(0, 0, oled_buf, OLED_6X8);
            }
            else if (control_mode == HAND_LR)
            {
                sprintf(oled_buf, "Control    M=HAND_L/R");
                OLED_ShowString(0, 0, oled_buf, OLED_6X8);
            }
            else if (control_mode == HAND_XYZ)
            {
                sprintf(oled_buf, "Control    M=HAND_XYZ");
                OLED_ShowString(0, 0, oled_buf, OLED_6X8);
            }
            else if (control_mode == NRF_CTRL)
            {
                if (nrf_control_mode == CAR_CONTROL)
                {
                    sprintf(oled_buf, "Control     M=NRF_CAR");
                    OLED_ShowString(0, 0, oled_buf, OLED_6X8);
                }
                else if (nrf_control_mode == REMOTE_CONTROL)
                {
                    sprintf(oled_buf, "Control     M=NRF_REM");
                    OLED_ShowString(0, 0, oled_buf, OLED_6X8);
                }
            }
            else if (control_mode == JOY_XYZ)
            {
                sprintf(oled_buf, "Control     M=JOY_XYZ");
                OLED_ShowString(0, 0, oled_buf, OLED_6X8);
            }
            else if (control_mode == HAND_TRACK)
            {
                sprintf(oled_buf, "Control  M=HAND_TRACK");
                OLED_ShowString(0, 0, oled_buf, OLED_6X8);
            }
            sprintf(oled_buf, "TAR V% 4.2f A% 4.2f JOY", car_set_velocity.v, car_set_velocity.angular);
            OLED_ShowString(0, 8, oled_buf, OLED_6X8);
            sprintf(oled_buf, "JOY V:% .2f, A:% .2f", four_dirction_velocity, four_dirction_angular);
            OLED_ShowString(0, 16, oled_buf, OLED_6X8);
            OLED_DrawLine(0, 23, 128, 23);
            sprintf(oled_buf, "    MOTORA:  MOTORB:");
            OLED_ShowString(0, 24, oled_buf, OLED_6X8);
            sprintf(oled_buf, "PWM %4.0f     %4.0f", MOTOR_A.Motor_Pwm, MOTOR_B.Motor_Pwm);
            OLED_ShowString(0, 32, oled_buf, OLED_6X8);
            sprintf(oled_buf, "TAR %4.2f     %4.2f", MOTOR_A.Target, MOTOR_B.Target);
            OLED_ShowString(0, 40, oled_buf, OLED_6X8);
            sprintf(oled_buf, "KP  %4.0f     %4.0f", MOTOR_A.Velocity_KP, MOTOR_B.Velocity_KP);
            OLED_ShowString(0, 48, oled_buf, OLED_6X8);
            sprintf(oled_buf, "KI  %4.0f     %4.0f", MOTOR_A.Velocity_KI, MOTOR_B.Velocity_KI);
            OLED_ShowString(0, 56, oled_buf, OLED_6X8);

            switch (oled_curser.motor_control_page.curser)
            {
            case 0:
                oled_show_block(9 * 6, 0, 12 * 6, 8, oled_curser.motor_control_page);
                break;
            case 1:
                oled_show_block(6 * 6, 1 * 8, 4 * 6, 8, oled_curser.motor_control_page);
                break;
            case 2:
                oled_show_block(13 * 6, 1 * 8, 4 * 6, 8, oled_curser.motor_control_page);
                break;
            case 3:
                oled_show_block(18 * 6, 1 * 8, 3 * 6, 8, oled_curser.motor_control_page);
                break;
            case 4:
                oled_show_block(7 * 6, 2 * 8, 4 * 6, 8, oled_curser.motor_control_page);
                break;
            case 5:
                oled_show_block(16 * 6, 2 * 8, 4 * 6, 8, oled_curser.motor_control_page);
                break;
            case 6:
                oled_show_block(4 * 6, 4 * 8, 6 * 6, 8, oled_curser.motor_control_page);
                break;
            case 7:
                oled_show_block(13 * 6, 4 * 8, 6 * 6, 8, oled_curser.motor_control_page);
                break;
            case 8:
                oled_show_block(4 * 6, 5 * 8, 6 * 6, 8, oled_curser.motor_control_page);
                break;
            case 9:
                oled_show_block(13 * 6, 5 * 8, 6 * 6, 8, oled_curser.motor_control_page);
                break;
            case 10:
                oled_show_block(4 * 6, 6 * 8, 6 * 6, 8, oled_curser.motor_control_page);
                break;
            case 11:
                oled_show_block(13 * 6, 6 * 8, 6 * 6, 8, oled_curser.motor_control_page);
                break;
            case 12:
                oled_show_block(4 * 6, 7 * 8, 6 * 6, 8, oled_curser.motor_control_page);
                break;
            case 13:
                oled_show_block(13 * 6, 7 * 8, 6 * 6, 8, oled_curser.motor_control_page);
                break;
            
            default:
                break;
            }
        }
        // NRF数据--------------------------------------------------------------------------
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
        // LED 蜂鸣器 系统相关数据--------------------------------------------------------------------------
        else if (oled_page == LED_BUZZER_SYS_PAGE)
        {
            sprintf(oled_buf, "LED Color: %s", led_color_to_string(car_led_color));
            OLED_ShowString(0, 0, oled_buf, OLED_6X8);
            if (car_buzzer_param.status == ENABLE)
            {
                sprintf(oled_buf, "Buzzer: ON  Set:%4d", car_buzzer_param.set_freq);
                OLED_ShowString(0, 8, oled_buf, OLED_6X8);
            }
            else if (car_buzzer_param.status == DISABLE)
            {
                sprintf(oled_buf, "Buzzer: OFF Set:%4d", car_buzzer_param.set_freq);
                OLED_ShowString(0, 8, oled_buf, OLED_6X8);
            }

            sprintf(oled_buf, "            Real:%4d", car_buzzer_param.real_freq);
            OLED_ShowString(0, 16, oled_buf, OLED_6X8);

            sprintf(oled_buf, "disp gap:%2d", oled_refresh_gap);
            OLED_ShowString(0, 24, oled_buf, OLED_6X8);

            if (display_max_time == DISABLE)
            {
                sprintf(oled_buf, "current");
                OLED_ShowString(16 * 5, 24, oled_buf, OLED_6X8);

                sprintf(oled_buf, "mai%4d", main_all_tick);
                OLED_ShowString(0, 32, oled_buf, OLED_6X8);
                sprintf(oled_buf, "ker%4d", main_during_tick[0]);
                OLED_ShowString(0, 40, oled_buf, OLED_6X8);
                sprintf(oled_buf, "kep%4d", main_during_tick[1]);
                OLED_ShowString(0, 48, oled_buf, OLED_6X8);
                sprintf(oled_buf, "olp%4d", main_during_tick[2]);
                OLED_ShowString(0, 56, oled_buf, OLED_6X8);
                sprintf(oled_buf, "old%4d", main_during_tick[3]);
                OLED_ShowString(42, 32, oled_buf, OLED_6X8);
                sprintf(oled_buf, "nrf%4d", main_during_tick[4]);
                OLED_ShowString(42, 40, oled_buf, OLED_6X8);
                sprintf(oled_buf, "led%4d", main_during_tick[5]);
                OLED_ShowString(42, 48, oled_buf, OLED_6X8);
                sprintf(oled_buf, "uar%4d", main_during_tick[6]);
                OLED_ShowString(42, 56, oled_buf, OLED_6X8);
                sprintf(oled_buf, "pos%4d", main_during_tick[7]);
                OLED_ShowString(84, 32, oled_buf, OLED_6X8);
                sprintf(oled_buf, "dri%4d", main_during_tick[8]);
                OLED_ShowString(84, 40, oled_buf, OLED_6X8);
                sprintf(oled_buf, "map%4d", main_during_tick[9]);
                OLED_ShowString(84, 48, oled_buf, OLED_6X8);
            }
            else if (display_max_time == ENABLE)
            {
                sprintf(oled_buf, "max    ");
                OLED_ShowString(16 * 5, 24, oled_buf, OLED_6X8);

                sprintf(oled_buf, "mai%4d", main_all_tick_max);
                OLED_ShowString(0, 32, oled_buf, OLED_6X8);
                sprintf(oled_buf, "ker%4d", main_during_tick_max[0]);
                OLED_ShowString(0, 40, oled_buf, OLED_6X8);
                sprintf(oled_buf, "kep%4d", main_during_tick_max[1]);
                OLED_ShowString(0, 48, oled_buf, OLED_6X8);
                sprintf(oled_buf, "olp%4d", main_during_tick_max[2]);
                OLED_ShowString(0, 56, oled_buf, OLED_6X8);
                sprintf(oled_buf, "old%4d", main_during_tick_max[3]);
                OLED_ShowString(42, 32, oled_buf, OLED_6X8);
                sprintf(oled_buf, "nrf%4d", main_during_tick_max[4]);
                OLED_ShowString(42, 40, oled_buf, OLED_6X8);
                sprintf(oled_buf, "led%4d", main_during_tick_max[5]);
                OLED_ShowString(42, 48, oled_buf, OLED_6X8);
                sprintf(oled_buf, "uar%4d", main_during_tick_max[6]);
                OLED_ShowString(42, 56, oled_buf, OLED_6X8);
                sprintf(oled_buf, "pos%4d", main_during_tick_max[7]);
                OLED_ShowString(84, 32, oled_buf, OLED_6X8);
                sprintf(oled_buf, "dri%4d", main_during_tick_max[8]);
                OLED_ShowString(84, 40, oled_buf, OLED_6X8);
                sprintf(oled_buf, "map%4d", main_during_tick_max[9]);
                OLED_ShowString(84, 48, oled_buf, OLED_6X8);
            }
            switch (oled_curser.led_buzzer_page.curser)
            {
            case 0:
                oled_show_block(11 * 6, 0 * 8, 6 * 6, 8, oled_curser.led_buzzer_page);
                break;
            case 1:
                oled_show_block(8 * 6, 1 * 8, 3 * 6, 8, oled_curser.led_buzzer_page);
                break;
            case 2:
                oled_show_block(16 * 6, 1 * 8, 4 * 6, 8, oled_curser.led_buzzer_page);
                break;
            case 3:
                oled_show_block(9 * 6, 3 * 8, 2 * 6, 8, oled_curser.led_buzzer_page);
                break;
            case 4:
                oled_show_block(16 * 5, 3 * 8, 7 * 6, 8, oled_curser.led_buzzer_page);
                break;
            default:
                break;
            }
        }
        // 轨迹追踪器数据--------------------------------------------------------------------------
        else if (oled_page == TRACK_DATA_PAGE)
        {
            uint16_t start_idx = track_data_page * 8; // 计算当前页的起始数据索引

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

            sprintf(oled_buf, "p%2d l%3d", track_data_page, current_track_length);
            OLED_ShowString(64 + 4, 0, oled_buf, OLED_6X8);
            sprintf(oled_buf, "n%2dl%2dtest", tracker.near_index, tracker.lookahead_index);
            OLED_ShowString(64 + 4, 8, oled_buf, OLED_6X8);
            sprintf(oled_buf, "c% .1f % .1f", car_combined_processed.CombinedPosition.x, car_combined_processed.CombinedPosition.y);
            OLED_ShowString(64 + 4, 16, oled_buf, OLED_6X8);
            sprintf(oled_buf, "b% .1f % .1f", tracker.lookahead_bias.x, tracker.lookahead_bias.y);
            OLED_ShowString(64 + 4, 24, oled_buf, OLED_6X8);
            sprintf(oled_buf, "o% .1f % .1f", tracker.cmd_linear, tracker.cmd_angular);
            OLED_ShowString(64 + 4, 32, oled_buf, OLED_6X8);
            sprintf(oled_buf, "v% .1fk% .1f", tracker.target_speed, tracker.k_pure_pursuit);
            OLED_ShowString(64 + 4, 40, oled_buf, OLED_6X8);
            sprintf(oled_buf, "s% .1fs% .1f", tracker.slowdown_start_dist, tracker.stop_dist);
            OLED_ShowString(64 + 4, 48, oled_buf, OLED_6X8);
            if (tracker.state == DISABLE)
            {
                sprintf(oled_buf, "OF");
                OLED_ShowString(64 + 4, 56, oled_buf, OLED_6X8);
            }
            else if (tracker.state == ENABLE)
            {
                sprintf(oled_buf, "ON");
                OLED_ShowString(64 + 4, 56, oled_buf, OLED_6X8);
            }
            sprintf(oled_buf, "R");
            OLED_ShowString(64 + 4 + 3 * 6, 56, oled_buf, OLED_6X8);
            sprintf(oled_buf, "l% .1f", tracker.lookahead_distance);
            OLED_ShowString(64 + 4 + 5 * 6, 56, oled_buf, OLED_6X8);

            OLED_DrawLine(67, 0, 67, 64);

            switch (oled_curser.track_data_page.curser)
            {
            case 0:
                oled_show_block(4 * 6, 0 * 8, 3 * 6, 8, oled_curser.track_data_page);
                break;
            case 1:
                oled_show_block(8 * 6, 0 * 8, 3 * 6, 8, oled_curser.track_data_page);
                break;
            case 2:
                oled_show_block(4 * 6, 1 * 8, 3 * 6, 8, oled_curser.track_data_page);
                break;
            case 3:
                oled_show_block(8 * 6, 1 * 8, 3 * 6, 8, oled_curser.track_data_page);
                break;
            case 4:
                oled_show_block(4 * 6, 2 * 8, 3 * 6, 8, oled_curser.track_data_page);
                break;
            case 5:
                oled_show_block(8 * 6, 2 * 8, 3 * 6, 8, oled_curser.track_data_page);
                break;
            case 6:
                oled_show_block(4 * 6, 3 * 8, 3 * 6, 8, oled_curser.track_data_page);
                break;
            case 7:
                oled_show_block(8 * 6, 3 * 8, 3 * 6, 8, oled_curser.track_data_page);
                break;
            case 8:
                oled_show_block(4 * 6, 4 * 8, 3 * 6, 8, oled_curser.track_data_page);
                break;
            case 9:
                oled_show_block(8 * 6, 4 * 8, 3 * 6, 8, oled_curser.track_data_page);
                break;
            case 10:
                oled_show_block(4 * 6, 5 * 8, 3 * 6, 8, oled_curser.track_data_page);
                break;
            case 11:
                oled_show_block(8 * 6, 5 * 8, 3 * 6, 8, oled_curser.track_data_page);
                break;
            case 12:
                oled_show_block(4 * 6, 6 * 8, 3 * 6, 8, oled_curser.track_data_page);
                break;
            case 13:
                oled_show_block(8 * 6, 6 * 8, 3 * 6, 8, oled_curser.track_data_page);
                break;
            case 14:
                oled_show_block(4 * 6, 7 * 8, 3 * 6, 8, oled_curser.track_data_page);
                break;
            case 15:
                oled_show_block(8 * 6, 7 * 8, 3 * 6, 8, oled_curser.track_data_page);
                break;
            case 16:
                oled_show_block(68 + 1 * 6, 0 * 8, 2 * 6, 8, oled_curser.track_data_page);
                break;
            case 17:
                oled_show_block(68 + 5 * 6, 0 * 8, 3 * 6, 8, oled_curser.track_data_page);
                break;
            case 18:
                oled_show_block(68 + 6 * 6, 1 * 8, 4 * 6, 8, oled_curser.track_data_page);
                break;

            case 19:
                oled_show_block(68, 5 * 8, 5 * 6, 8, oled_curser.track_data_page);
                break;
            case 20:
                oled_show_block(68 + 5 * 6, 5 * 8, 5 * 6, 8, oled_curser.track_data_page);
                break;
            case 21:
                oled_show_block(68, 6 * 8, 5 * 6, 8, oled_curser.track_data_page);
                break;
            case 22:
                oled_show_block(68 + 5 * 6, 6 * 8, 5 * 6, 8, oled_curser.track_data_page);
                break;
            case 23:
                oled_show_block(68, 7 * 8, 2 * 6, 8, oled_curser.track_data_page);
                break;
            case 24:
                oled_show_block(68 + 3 * 6, 7 * 8, 1 * 6, 8, oled_curser.track_data_page);
                break;
            case 25:
                oled_show_block(68 + 5 * 6, 7 * 8, 5 * 6, 8, oled_curser.track_data_page);
                break;
            default:
                break;
            }
        }
        // 轨迹追踪器绘图--------------------------------------------------------------------------
        else if (oled_page == TRACK_GRAPH_PAGE)
        {
            sprintf(oled_buf, "TRACK");
            OLED_ShowString(0, 0, oled_buf, OLED_6X8);
            oled_draw_grid();
            oled_draw_car_arrow(car_combined_processed.CombinedPosition.x, car_combined_processed.CombinedPosition.y, car_combined_processed.CombinedPosition.theta);
            for (int i = 0; i < current_track_length; i++)
            {
                OLED_DrawCircle(oled_transform_axis(tracker.track[i].x, OLED_X_TYPE), oled_transform_axis(tracker.track[i].y, OLED_Y_TYPE), 1, 1);
            }
            OLED_DrawRectangle(oled_transform_axis(tracker.track[tracker.near_index].x, OLED_X_TYPE) - 2, oled_transform_axis(tracker.track[tracker.near_index].y, OLED_Y_TYPE) - 2, 5, 5, 0);
            OLED_DrawCircle(oled_transform_axis(tracker.track[tracker.lookahead_index].x, OLED_X_TYPE), oled_transform_axis(tracker.track[tracker.lookahead_index].y, OLED_Y_TYPE), 3, 0);
            OLED_DrawLine(oled_transform_axis(car_combined_processed.CombinedPosition.x, OLED_X_TYPE),
                          oled_transform_axis(car_combined_processed.CombinedPosition.y, OLED_Y_TYPE),
                          oled_transform_axis(tracker.track[tracker.lookahead_index].x, OLED_X_TYPE),
                          oled_transform_axis(tracker.track[tracker.lookahead_index].y, OLED_Y_TYPE));
        }
        // 地图数据--------------------------------------------------------------------------
        else if (oled_page == MAP_DATA_PAGE)
        {
            sprintf(oled_buf, "MAP               ");
            OLED_ShowString(0, 0, oled_buf, OLED_6X8);
            sprintf(oled_buf, "pos % .1f % .1f % .1f", car_combined_processed.CombinedPosition.x, car_combined_processed.CombinedPosition.y, car_combined_processed.CombinedPosition.theta);
            OLED_ShowString(0, 8, oled_buf, OLED_6X8);
            sprintf(oled_buf, "distance % .1f", vl53l1_distance);
            OLED_ShowString(0, 16, oled_buf, OLED_6X8);
            sprintf(oled_buf, "obstacle status: %d", obstacle_info.status);
            OLED_ShowString(0, 24, oled_buf, OLED_6X8);
            sprintf(oled_buf, "obstacle % .1f % .1f", obstacle_info.obstacle_position.x, obstacle_info.obstacle_position.y);
            OLED_ShowString(0, 32, oled_buf, OLED_6X8);
            sprintf(oled_buf, "process");
            OLED_ShowString(0, 40, oled_buf, OLED_6X8);
            sprintf(oled_buf, "erode");
            OLED_ShowString(0, 48, oled_buf, OLED_6X8);
            sprintf(oled_buf, "dilate");
            OLED_ShowString(0, 56, oled_buf, OLED_6X8);
            sprintf(oled_buf, "line");
            OLED_ShowString(64, 40, oled_buf, OLED_6X8);
            if (map_detect_switch == DISABLE)
            {
                sprintf(oled_buf, "OFF");
                OLED_ShowString(64, 48, oled_buf, OLED_6X8);
            }
            else if (map_detect_switch == ENABLE)
            {
                sprintf(oled_buf, "ON");
                OLED_ShowString(64, 48, oled_buf, OLED_6X8);
            }
            sprintf(oled_buf, "TestCShape");
            OLED_ShowString(64, 56, oled_buf, OLED_6X8);

            switch (oled_curser.map_data_page.curser)
            {
            case 0:
                oled_show_block(0, 40, 7 * 6, 8, oled_curser.map_data_page);
                break;
            case 1:
                oled_show_block(0, 48, 5 * 6, 8, oled_curser.map_data_page);
                break;
            case 2:
                oled_show_block(0, 56, 6 * 6, 8, oled_curser.map_data_page);
                break;
            case 3:
                oled_show_block(64, 40, 4 * 6, 8, oled_curser.map_data_page);
                break;
            case 4:
                oled_show_block(64, 48, 3 * 6, 8, oled_curser.map_data_page);
                break;
            case 5:
                oled_show_block(64, 56, 10 * 6, 8, oled_curser.map_data_page);
                break;
            default:
                break;
            }
        }
        // 地图绘图--------------------------------------------------------------------------
        else if (oled_page == MAP_GRAPH_PAGE)
        {
            sprintf(oled_buf, "MAP");
            OLED_ShowString(0, 0, oled_buf, OLED_6X8);
            sprintf(oled_buf, "level=%d", map_show_level);
            OLED_ShowString(64, 56, oled_buf, OLED_6X8);
            oled_show_block(64, 56, 7 * 6, 8, oled_curser.map_graph_page);

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
            
        }
        // 地图1:1绘图--------------------------------------------------------------------------
        else if (oled_page == MAP_PURE_PAGE)
        {
            sprintf(oled_buf, "MAP");
            OLED_ShowString(0, 0, oled_buf, OLED_6X8);
            sprintf(oled_buf, "l%d x%d y%d", map_show_level, (int)map_show_area.x, (int)map_show_area.y);
            OLED_ShowString(64, 56, oled_buf, OLED_6X8);

            OLED_ShowPureMap();

            switch (oled_curser.map_pure_page.curser)
            {
            case 0:
                oled_show_block(64, 56, 3 * 6, 8, oled_curser.map_pure_page);
                break;
            case 1:
                oled_show_block(64 + 3 * 6, 56, 5 * 6, 8, oled_curser.map_pure_page);
                break;

            default:
                break;
            }
        }
        // 路径求解器数据--------------------------------------------------------------------------
        else if (oled_page == SOLVE_DATA_PAGE)
        {
            // 计算当前页的起始数据索引
            uint16_t start_idx = solve_data_page * 8;

            // 1. 循环绘制每一行数据
            for (uint8_t i = 0; i < 8; i++)
            {
                uint16_t current_data_idx = start_idx + i;

                // 如果数据超出数组大小，清除该行并跳过
                if (current_data_idx >= MAX_SOLVE_NODES)
                {
                    sprintf(oled_buf, "END HERE");
                    OLED_ShowString(0, 56, oled_buf, OLED_6X8);
                    continue;
                }

                // 格式化字符串：[序号] [X] [Y] [状态短码]
                // 序号使用 current_data_idx + 1，方便人类阅读
                // 1. 获取紧凑序号 (例如 105 -> "A5")
                char idx_str[3];
                get_compact_index_str(idx_str, current_data_idx + 1);

                // 2. 格式化输出
                // 去掉冒号后的空格，进一步压缩空间
                sprintf(oled_buf, " %s% .1f% .1f%s",
                        idx_str,
                        solves[current_data_idx].point.x,
                        solves[current_data_idx].point.y,
                        get_solve_state_string(solves[current_data_idx].solvestate));

                OLED_ShowString(0, i * 8, oled_buf, OLED_6X8);

                if (current_data_idx == solves_count - 1)
                {
                    OLED_DrawLine(0, i * 8 + 7, 72, i * 8 + 7);
                }
            }

            // 2. 绘制当前追踪进度标识 (逻辑偏移处理)
            // 逻辑：path[0]是起点，path[1...]对应solves[0...]
            // 所以 solves 的指示器索引 = tracker.near_index - 1
            int16_t solve_near_idx = (int16_t)tracker.near_index - 1;
            int16_t solve_look_idx = (int16_t)tracker.lookahead_index - 1;

            // 只有当追踪点在当前页面的显示范围内时，才绘制标识
            if (solve_near_idx >= start_idx && solve_near_idx < (start_idx + 8))
            {
                uint8_t relative_row = solve_near_idx - start_idx;
                // 在该行最左侧或指定位置绘制标识，此处复用你的矩形/圆圈逻辑
                OLED_DrawRectangle(0, relative_row * 8, 6, 6, 0);
            }
            // 只有当追踪点在当前页面的显示范围内时，才绘制标识
            if (solve_look_idx >= start_idx && solve_look_idx < (start_idx + 8))
            {
                uint8_t relative_row = solve_look_idx - start_idx;
                // 在该行最左侧或指定位置绘制标识，此处复用你的矩形/圆圈逻辑
                OLED_DrawCircle(3, relative_row * 8 + 3, 3, 0); // 1表示实心或高亮
            }

            OLED_DrawLine(73, 0, 73, 64);

            sprintf(oled_buf, "goal");
            OLED_ShowString(74, 0, oled_buf, OLED_6X8);
            sprintf(oled_buf, "% .1f % .1f", car_goal_position.x, car_goal_position.y);
            OLED_ShowString(74, 8, oled_buf, OLED_6X8);
            sprintf(oled_buf, "solve");
            OLED_ShowString(74, 16, oled_buf, OLED_6X8);
            sprintf(oled_buf, "page:%d", solve_data_page);
            OLED_ShowString(74, 24, oled_buf, OLED_6X8);
            sprintf(oled_buf, "len=%2d", solves_count);
            OLED_ShowString(74, 32, oled_buf, OLED_6X8);
            if (path_solve_status == SOLVE_OK)
            {
                sprintf(oled_buf, "SOLVE_OK");
                OLED_ShowString(74, 40, oled_buf, OLED_6X8);
            }
            else if (path_solve_status == STACK_OVERFLOW)
            {
                sprintf(oled_buf, "OVERFLOW");
                OLED_ShowString(74, 40, oled_buf, OLED_6X8);
            }
            else if (path_solve_status == NO_SPACE_POINT)
            {
                sprintf(oled_buf, "NO_SPACE");
                OLED_ShowString(74, 40, oled_buf, OLED_6X8);
            }

            switch (oled_curser.solve_data_page.curser)
            {
            case 0:
                oled_show_block(74 + 1 * 6, 8, 3 * 6, 8, oled_curser.solve_data_page);
                break;
            case 1:
                oled_show_block(74 + 6 * 6, 8, 3 * 6, 8, oled_curser.solve_data_page);
                break;
            case 2:
                oled_show_block(74, 16, 5 * 6, 8, oled_curser.solve_data_page);
                break;
            case 3:
                oled_show_block(74, 24, 7 * 6, 8, oled_curser.solve_data_page);
                break;
            default:
                break;
            }
        }
        // 路径求解器绘图--------------------------------------------------------------------------
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
            
            // if (car_map_compressed_complete == ENABLE)
            // {
            //     OLED_ShowCompressedMap(32);
            // }
            // else
            // {
                OLED_ShowFullMap_FixPoint();
            // }

            oled_draw_car_arrow(car_combined_processed.CombinedPosition.x, car_combined_processed.CombinedPosition.y, car_combined_processed.CombinedPosition.theta);

            OLED_DrawTriangle(oled_transform_axis(car_goal_position.x, OLED_X_TYPE), oled_transform_axis(car_goal_position.y, OLED_Y_TYPE) - 4,
                              oled_transform_axis(car_goal_position.x, OLED_X_TYPE) - 3, oled_transform_axis(car_goal_position.y, OLED_Y_TYPE) + 2,
                              oled_transform_axis(car_goal_position.x, OLED_X_TYPE) + 3, oled_transform_axis(car_goal_position.y, OLED_Y_TYPE) + 2,
                              0);

            for (int i = 0; i < current_track_length; i++)
            {
                OLED_DrawCircle(oled_transform_axis(tracker.track[i].x, OLED_X_TYPE), oled_transform_axis(tracker.track[i].y, OLED_Y_TYPE), 1, 1);
            }
            OLED_DrawRectangle(oled_transform_axis(tracker.track[tracker.near_index].x, OLED_X_TYPE) - 2, oled_transform_axis(tracker.track[tracker.near_index].y, OLED_Y_TYPE) - 2, 5, 5, 0);
            OLED_DrawCircle(oled_transform_axis(tracker.track[tracker.lookahead_index].x, OLED_X_TYPE), oled_transform_axis(tracker.track[tracker.lookahead_index].y, OLED_Y_TYPE), 3, 0);
            OLED_DrawLine(oled_transform_axis(car_combined_processed.CombinedPosition.x, OLED_X_TYPE),
                          oled_transform_axis(car_combined_processed.CombinedPosition.y, OLED_Y_TYPE),
                          oled_transform_axis(tracker.track[tracker.lookahead_index].x, OLED_X_TYPE),
                          oled_transform_axis(tracker.track[tracker.lookahead_index].y, OLED_Y_TYPE));
        }

        if (car_key[4].press_time != 0)
        {
            OLED_DrawLine(0, 63, (car_key[4].press_time * OLED_MAX_WIDTH) / KEY_LONG_CNT, 63);
        }
    }
}

/**
 * @brief OLED显示函数
 */
void oled_display()
{
    static uint16_t counter = 0; // 静态计数器，记录函数执行次数

    counter++; // 每次进入函数，计数自增

    // 逻辑判定
    if (counter >= oled_refresh_gap && oled_refresh_gap != 0) // 0：关闭OLED刷新
    {
        OLED_Update();
        counter = 0; // 完成一个周期(8+2)，计数器清零重新开始
    }
}
