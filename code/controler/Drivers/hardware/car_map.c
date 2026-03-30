#include "car_map.h"
#include "system.h"

// 内部宏
#define MAP_CENTER (CAR_MAP_SIZE / 2)

// private start------------------
// 缓冲区操作--------------------------------------------------------------
/**
 * @brief 从指定的行缓冲区中读取第 x 个点的 2bit 状态
 * @param row 指向一行地图数据的指针 (uint8_t*)
 * @param x   点的列索引 (0 ~ 199)
 * @return    该点的状态值 (0, 1, 2, 或 3)
 */
static inline uint8_t GetStateFromBuf(uint8_t *row, int x)
{
    /* * 1. 寻找字节偏移 (Byte Offset): x >> 2
     * 因为每个字节存 4 个点，所以 x 除以 4 (右移2位) 得到该点在数组中的第几个字节。
     * 例如: x=5, 5>>2 = 1, 说明在 row[1] 里面。
     */

    /* * 2. 寻找位偏移 (Bit Offset): (x & 0x03) << 1
     * x & 0x03 取 x 的低两位 (即 x % 4)，得到点在字节内的序号 (0, 1, 2, 3)。
     * 每个点占 2 bit，所以序号乘以 2 (左移1位) 得到起始位位置。
     * 对应关系: 序号0->0位, 1->2位, 2->4位, 3->6位。
     */

    /*
     * 3. 提取并屏蔽: (row[...] >> offset) & 0x03
     * 将目标字节右移，使目标的 2 bit 对齐到最低位，然后通过 & 0x03 (二进制 11)
     * 把其他无关的高位全部抹去。
     */
    return (row[x >> 2] >> ((x & 0x03) << 1)) & 0x03;
}

/**
 * @brief 在指定的行缓冲区中修改第 x 个点的 2bit 状态
 * @param row   指向一行地图数据的指针
 * @param x     点的列索引
 * @param state 要写入的新状态值 (0~3)
 */
static inline void SetStateToBuf(uint8_t *row, int x, uint8_t state)
{
    /* * 1. 定位逻辑与读取一致
     */
    int byte_idx = x >> 2;
    int bit_offset = (x & 0x03) << 1;

    /* * 2. 制作“橡皮擦”并擦除旧值: row &= ~(0x03 << bit_offset)
     * 0x03 是二进制 11。左移 bit_offset 位后，对准了目标点。
     * 取反 (~) 后变成：目标位是 00，其他位全是 1。
     * 与运算 (&) 后：目标位的旧数据被清零，其他 3 个点的数据原封不动。
     */
    row[byte_idx] &= ~(0x03 << bit_offset);

    /* * 3. 填入新值: row |= (state << bit_offset)
     * 将新状态(state)左移到目标位置，通过或运算 (|) 填入刚才清零的坑里。
     * 这里的 state & 0x03 是为了防止输入值超过 2bit 导致破坏邻居。
     */
    row[byte_idx] |= ((state & 0x03) << bit_offset);
}
// private end------------------

// 地图读写操作 像素级读取--------------------------------------------------------------
/**
 * @brief 在全局地图 car_map 中修改指定 (x, y) 坐标点的状态
 * @param x 横坐标 (像素索引)
 * @param y 纵坐标 (行索引)
 * @param state 要写入的状态 (0:未知, 1:空闲, 2:轮廓, 3:障碍)
 */
void SetMapValue_ByPixel(int x, int y, uint8_t state)
{
    // 1. 鲁棒性检查：防止坐标越界导致 OOB (Out of Bounds) 错误
    if (x < 0 || x >= CAR_MAP_SIZE || y < 0 || y >= CAR_MAP_SIZE)
    {
        return;
    }

    // 2. 调用你提供的行缓冲区操作函数
    // car_map[y] 获取的是指向第 y 行数据的 uint8_t 指针
    SetStateToBuf(car_map[y], x, state);
}

/**
 * @brief 直接通过像素坐标(Grid Index)获取地图状态
 * @param x 像素列索引 (0 ~ CAR_MAP_SIZE-1)
 * @param y 像素行索引 (0 ~ CAR_MAP_SIZE-1)
 * @return 该点的状态值 (0~3)，若越界返回 0
 */
uint8_t GetMapValue_ByPixel(int x, int y)
{
    // 1. 边界检查，防止内存非法访问
    if (x < 0 || x >= CAR_MAP_SIZE || y < 0 || y >= CAR_MAP_SIZE)
    {
        return 0;
    }

    // 2. 调用你提供的行缓冲区操作函数
    // car_map[y] 获取的是指向第 y 行数据的 uint8_t 指针
    return GetStateFromBuf(car_map[y], x);
}

/**
 * @brief 将 200x200 的原图按照meter_pixel压缩，然后取左上角64*64放到压缩后的图像
 * @note  建议在小车停止运动或每隔 500ms 执行一次，不要放在高频控制循环里
 * @note  只有当METER_PIXEL=32，SHOW_CENTER_X=64，SHOW_CENTER_Y=32时，也就是设定为显示整张地图时，压缩地图相关代码才能正常工作
 */
void Map_Compress_Process(int meter_pixel)
{
    // 清空压缩地图
    memset(car_map_compressed, 0, sizeof(car_map_compressed));

    // 缩放比例约为 3.125 (200 / 64)
    // 为了简单且不丢失像素，我们计算每个压缩点对应的原图 3x3 区域
    for (int ty = 0; ty < COMPRESSED_SIZE; ty++)
    {
        int sy_start = (ty * 100) / meter_pixel;
        int sy_end = ((ty + 1) * 100) / meter_pixel;

        for (int tx = 0; tx < COMPRESSED_SIZE; tx++)
        {
            int sx_start = (tx * 100) / meter_pixel;
            int sx_end = ((tx + 1) * 100) / meter_pixel;

            uint8_t max_state = 0;

            // 在原图中搜索该区域的最大状态值
            for (int sy = sy_start; sy < sy_end && sy < 200; sy++)
            {
                for (int sx = sx_start; sx < sx_end && sx < 200; sx++)
                {
                    int s_byte_idx = sx / 4;
                    int s_bit_pos = (sx % 4) * 2;
                    uint8_t s_state = (car_map[sy][s_byte_idx] >> s_bit_pos) & 0x03;

                    if (s_state > max_state)
                        max_state = s_state;
                    if (max_state == 3)
                        break; // 已达到最大障碍等级，跳出
                }
                if (max_state == 3)
                    break;
            }

            // 将计算出的 max_state 存入压缩数组
            if (max_state > 0)
            {
                int t_byte_idx = tx / 4;
                int t_bit_pos = (tx % 4) * 2;
                car_map_compressed[ty][t_byte_idx] |= (max_state << t_bit_pos);
            }
        }
    }
    car_map_compressed_complete = ENABLE;
}
