#include "car_map.h"
#include "system.h"

// 内部宏：计算位偏移和掩码
#define POINTS_PER_BYTE (8 / BIT_PER_POINT)
#define BIT_MASK ((1 << BIT_PER_POINT) - 1)
#define MAP_CENTER (CAR_MAP_SIZE / 2)
#define CONTOUR_DIST 3 // 轮廓线距离（1,2像素空出，第3像素填充）

// 腐蚀和膨胀需要3行缓冲区：prev(上一行), curr(当前行), next(下一行)
// 画出边线需要 7 行缓冲区来覆盖 +/- 3 像素的垂直范围
uint8_t row_buf[7][MAP_ROW_BYTES];

uint8_t result_line[MAP_ROW_BYTES]; // 存放当前计算出的结果行

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

// 坐标变换--------------------------------------------------------------
/**
 * @brief 内部私有函数：将物理坐标(m)转换为地图索引
 * @param x:物理坐标的x
 * @param y:物理坐标的y
 * @param grid_x:地图索引x
 * @param grid_y:地图索引y
 * @return 0: 转换成功且在范围内, -1: 超出地图边界
 */
static int8_t WorldToMapIdx(float x, float y, int *grid_x, int *grid_y)
{
    // 1. 计算相对于中心的偏移格数 (四舍五入)，而不是正常的向0取整
    int offset_x = (int)(x / POINT_GAP + (x >= 0 ? 0.5f : -0.5f));
    int offset_y = (int)(y / POINT_GAP + (y >= 0 ? 0.5f : -0.5f));

    // 2. 映射到数组索引 (原点在中心)
    *grid_x = MAP_CENTER + offset_x;
    *grid_y = MAP_CENTER - offset_y;

    // 3. 边界检查
    if (*grid_x < 0 || *grid_x >= CAR_MAP_SIZE || *grid_y < 0 || *grid_y >= CAR_MAP_SIZE)
    {
        return -1;
    }
    return 0;
}

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

// 地图读写操作 真实物理坐标--------------------------------------------------------------

/**
 * @brief 通过真实物理坐标，设置地图指定格点的状态
 * @param x 真实物理坐标x
 * @param y 真实物理坐标y
 * @param state 要写入的状态 (0:未知, 1:空闲, 2:轮廓, 3:障碍)
 */
void SetMapValue(float x, float y, uint8_t state)
{
    int grid_x, grid_y;
    if (WorldToMapIdx(x, y, &grid_x, &grid_y) != 0)
        return;

    /* * 假设 BIT_PER_POINT = 2, 则 POINTS_PER_BYTE = 4, BIT_MASK = 0b11 (3)
     * 目标：修改第 grid_x 行，第 grid_y 个点的数据
     */

    // 1. 定位：计算 grid_x 这个点落在第几个字节（Byte）里
    // 2. 偏移：计算点在字节内的起始比特位（Bit Position）
    // 字节内部位分布预览: [点3(6-7位) | 点2(4-5位) | 点1(2-3位) | 点0(0-1位)]
    // 3. 制作“掩码弹药”：将 BIT_MASK 移动到目标位置
    // 4. 清除旧值（Masking Out）：使用与运算（&）
    // 5. 写入新值（Setting Bits）：使用或运算（|）

    int byte_idx = grid_x / POINTS_PER_BYTE;
    int bit_pos = (grid_x % POINTS_PER_BYTE) * BIT_PER_POINT;
    car_map[grid_y][byte_idx] &= ~(BIT_MASK << bit_pos);
    car_map[grid_y][byte_idx] |= ((state & BIT_MASK) << bit_pos);
}

/**
 * @brief 通过真实物理坐标，对地图指定格点的值进行增减
 * @param x 真实物理坐标x
 * @param y 真实物理坐标y
 * @param delta 要增减的数值
 */
void UpdateMapValue(float x, float y, int8_t delta)
{
    // 1. 获取当前格子的值 (复用 GetMapValue 逻辑)
    // 注意：如果坐标越界，GetMapValue 会返回 0，这与后续逻辑自洽
    uint8_t current_val = GetMapValue(x, y);

    // 2. 计算新值并处理溢出边界 (0 ~ BIT_MASK)
    // 使用 int16_t 防止中间计算溢出，确保对比逻辑准确
    int16_t new_val = (int16_t)current_val + delta;

    if (new_val > BIT_MASK)
        new_val = BIT_MASK;
    else if (new_val < 1 && delta < 0) // 如果是减法操作，下限设为 1
        new_val = 1;
    else if (new_val < 0)
        new_val = 0;

    // 3. 写回地图 (复用 SetMapValue 逻辑)
    // 这样整个 Update 函数就变成了：读取 -> 运算 -> 写入
    SetMapValue(x, y, (uint8_t)new_val);
}

/**
 * @brief 通过真实物理坐标，获取地图指定格点的数值
 * @param x 真实物理坐标x
 * @param y 真实物理坐标y
 * @return 该点的状态值 (0~3)，若越界返回 0
 */
uint8_t GetMapValue(float x, float y)
{
    int grid_x, grid_y;
    if (WorldToMapIdx(x, y, &grid_x, &grid_y) != 0)
        return 0; // 超出范围返回 0

    int byte_idx = grid_x / POINTS_PER_BYTE;
    int bit_pos = (grid_x % POINTS_PER_BYTE) * BIT_PER_POINT;
    return (car_map[grid_y][byte_idx] >> bit_pos) & BIT_MASK;

    // int byte_idx = grid_y / POINTS_PER_BYTE;
    // int bit_pos = (grid_y % POINTS_PER_BYTE) * BIT_PER_POINT;
    // return (car_map[grid_x][byte_idx] >> bit_pos) & BIT_MASK;
}

// 射线追踪建图任务------------------------------------------------------
/**
 * @brief 射线追踪建图任务
 */
void Update_Map(void)
{
    if (map_detect_switch == ENABLE)
    {
        // 1. 获取输入数据
        VL53L1_get_all();
        float x_c = car_combined_processed.CombinedPosition.x;
        float y_c = car_combined_processed.CombinedPosition.y;
        float theta = car_combined_processed.CombinedPosition.theta;
        float dist = vl53l1_distance;

        // 2. 传感器有效性过滤（VL53L1X 典型有效量程为 4cm - 4m）
        if (dist <= 0.04f || dist > 4.0f)
        {
            obstacle_info.status = DISABLE;
            return;
        }

        // 3. 预计算三角函数，避免在循环内重复调用（节省开销）
        float cos_t = cosf(theta);
        float sin_t = sinf(theta);

        // 4. 射线追踪：清除路径上的障碍物（Space Clearing）
        // 我们从 0 步进到 dist 之前的最后一个格子
        // 即使是空旷区域，我们也采用“减 1”策略，而不是直接暴力 Set 0，
        // 这样可以利用“复利”效应过滤掉偶发的传感器遮挡。
        float current_r = 0.0f;
        while (current_r < (dist - 3.0f * POINT_GAP))
        {
            float p_x = x_c + current_r * cos_t;
            float p_y = y_c + current_r * sin_t;

            // 调用 API：将该点的占据概率降低
            UpdateMapValue(p_x, p_y, -1);

            // 步进一个格子的距离
            current_r += POINT_GAP;
        }

        // 5. 命中目标：标记障碍物点（Obstacle Marking）
        // 终点坐标计算
        float obs_x = x_c + dist * cos_t;
        float obs_y = y_c + dist * sin_t;
        obstacle_info.obstacle_position.x = obs_x;
        obstacle_info.obstacle_position.y = obs_y;
        obstacle_info.status = ENABLE;
        // 调用 API：将该点的占据概率增加
        UpdateMapValue(obs_x, obs_y, 2);
    }
}

// 当我把map_show_level调到1时，我可以很清楚地在oled看到空闲的区域像射线状一样随着小车旋转而放射，边缘也是类似墙角的形状。可是如果map_show_level为2，障碍物边缘就只能看到零星几个点了
// 这是因为：
// 在 Level 1（空闲）能看到射线，说明 UpdateMapValue(..., -1) 工作得非常频繁且覆盖面广；
// 而 Level 2（占据）只有碎点，说明 UpdateMapValue(..., 1) 作用在同一个格子上的次数不够多，或者传感器扫过的路径重叠度太低，导致大部分格子的数值停留在 1，还没跨过 2 的门槛
// 因此需要利用Level 1部分的信息来重新进行图像处理，丢弃原本Level 2和Level 3的数据
// 图像处理-----------------------------------------------------------------------

/**
 * @brief 对car_map中state1部分进行腐蚀操作，其余部分清除为state0
 */
void erode_map()
{
    // --- 第一阶段：全图腐蚀 (Erosion) ---
    // 逻辑：只有 3x3 范围内全是 1，中心才保持 1；2/3状态跳过不处理
    // 初始化：先拷贝第0行和第1行
    memcpy(row_buf[0], car_map[0], MAP_ROW_BYTES);
    memcpy(row_buf[1], car_map[1], MAP_ROW_BYTES);

    for (int y = 0; y < CAR_MAP_SIZE; y++)
    {
        // 准备下一行数据 (如果到了最后一行，next设为全0)
        if (y < CAR_MAP_SIZE - 1)
        {
            memcpy(row_buf[2], car_map[y + 1], MAP_ROW_BYTES);
        }
        else
        {
            memset(row_buf[2], 0, MAP_ROW_BYTES);
        }

        // 对当前行 y 的每一个点进行腐蚀计算
        memset(result_line, 0, MAP_ROW_BYTES);
        for (int x = 0; x < CAR_MAP_SIZE; x++)
        {
            uint8_t state = GetStateFromBuf(row_buf[1], x);

            if (state == 1)
            { // 只对空闲区进行腐蚀
                // 检查 4 邻域（上下左右）
                if (GetStateFromBuf(row_buf[0], x) == 1 &&
                    GetStateFromBuf(row_buf[2], x) == 1 &&
                    (x > 0 ? GetStateFromBuf(row_buf[1], x - 1) == 1 : 1) &&
                    (x < CAR_MAP_SIZE - 1 ? GetStateFromBuf(row_buf[1], x + 1) == 1 : 1))
                {
                    SetStateToBuf(result_line, x, 1);
                }
                else
                {
                    SetStateToBuf(result_line, x, 0); // 被腐蚀
                }
            }
            else
            {
                // SetStateToBuf(result_line, x, state); // 障碍物保留
                SetStateToBuf(result_line, x, 0); // 清除障碍物，后续根据复原的空白区域来重建障碍物
            }
        }

        // 将结果写回地图：注意为了下一行的计算，我们只能写回 y-1 行？
        // 优化：写回当前行是安全的，因为下一行 y+1 的计算使用的是 row_buf 里的备份
        memcpy(car_map[y], result_line, MAP_ROW_BYTES);

        // 滚动缓冲区：上一行 = 当前行，当前行 = 下一行
        memcpy(row_buf[0], row_buf[1], MAP_ROW_BYTES);
        memcpy(row_buf[1], row_buf[2], MAP_ROW_BYTES);
    }
}

/**
 * @brief 对car_map的state1部分进行膨胀操作，其余部分清除为state0
 */
void dilate_map()
{
    // --- 第二阶段：全图膨胀 (Dilation) ---
    // 逻辑：如果 3x3 范围内有 1，中心就变为 1
    // (代码结构与腐蚀完全一致，仅判断逻辑变为 || )
    memcpy(row_buf[0], car_map[0], MAP_ROW_BYTES);
    memcpy(row_buf[1], car_map[1], MAP_ROW_BYTES);

    for (int y = 0; y < CAR_MAP_SIZE; y++)
    {
        if (y < CAR_MAP_SIZE - 1)
            memcpy(row_buf[2], car_map[y + 1], MAP_ROW_BYTES);
        else
            memset(row_buf[2], 0, MAP_ROW_BYTES);

        memset(result_line, 0, MAP_ROW_BYTES);
        for (int x = 0; x < CAR_MAP_SIZE; x++)
        {
            uint8_t state = GetStateFromBuf(row_buf[1], x);
            if (state <= 1)
            { // 对空闲/探测区膨胀
                if (GetStateFromBuf(row_buf[1], x) == 1 ||
                    GetStateFromBuf(row_buf[0], x) == 1 ||
                    GetStateFromBuf(row_buf[2], x) == 1 ||
                    (x > 0 ? GetStateFromBuf(row_buf[1], x - 1) == 1 : 0) ||
                    (x < CAR_MAP_SIZE - 1 ? GetStateFromBuf(row_buf[1], x + 1) == 1 : 0))
                {
                    SetStateToBuf(result_line, x, 1);
                }
            }
            else
            {
                // SetStateToBuf(result_line, x, state); // 障碍物保留
                SetStateToBuf(result_line, x, 0); // 清除障碍物，后续根据复原的空白区域来重建障碍物
            }
        }
        memcpy(car_map[y], result_line, MAP_ROW_BYTES);
        memcpy(row_buf[0], row_buf[1], MAP_ROW_BYTES);
        memcpy(row_buf[1], row_buf[2], MAP_ROW_BYTES);
    }
}



/**
 * @brief 将地图最外圈 3 像素区域强制清零 (State 0)
 * @details 这样做是为了给 Map_Generate_Boundary_Contour 提供 3 像素的缓冲带，
 * 确保即使空闲区 (State 1) 贴着边缘，算法也能在距离边缘 3 像素处激发出 State 3。
 */
void Map_Deep_Boundary_Clear(void)
{
    // 1. 强制清空顶部 3 行和底部 3 行 (State 0)
    // 每行占用 MAP_ROW_BYTES 字节
    for (int i = 0; i < 3; i++)
    {
        memset(car_map[i], 0, MAP_ROW_BYTES);                    // 顶部 3 行
        memset(car_map[CAR_MAP_SIZE - 1 - i], 0, MAP_ROW_BYTES); // 底部 3 行
    }

    // 2. 强制清空左右两侧各 3 像素 (State 0)
    for (int y = 0; y < CAR_MAP_SIZE; y++)
    {
        // 左侧 3 像素：位于每行的第 0 个字节
        // 我们通过与运算掩码 0x3F (00111111) 来清除最高 2 位 (第0个点)
        // 但为了清除 3 个点，我们需要更精准的操作：
        // 假设一个字节存 [P0, P1, P2, P3]，我们要清掉前三个
        for (int x = 0; x < 3; x++)
        {
            SetMapValue_ByPixel(x, y, 0);                    // 左侧 3 像素列
            SetMapValue_ByPixel(CAR_MAP_SIZE - 1 - x, y, 0); // 右侧 3 像素列
        }
    }
}

/**
 * @brief 在空闲区域外围 2 像素处生成一圈 State 3 的轮廓线
 * 内存消耗：约 400 字节 (8行缓冲区)
 */
void Map_Generate_Boundary_Contour(void)
{
    Map_Deep_Boundary_Clear();

    // 初始化：填充前几行数据（处理边界溢出）
    for (int i = 0; i < 7; i++)
    {
        int target_y = i - CONTOUR_DIST; // 从 -3 行开始
        if (target_y >= 0 && target_y < CAR_MAP_SIZE)
            memcpy(row_buf[i], car_map[target_y], MAP_ROW_BYTES);
        else
            memset(row_buf[i], 0, MAP_ROW_BYTES);
    }

    for (int gy = 0; gy < CAR_MAP_SIZE; gy++)
    {
        // 1. 准备当前行的结果（先复制原行，避免破坏已有的障碍物）
        memcpy(result_line, car_map[gy], MAP_ROW_BYTES);

        for (int gx = 0; gx < CAR_MAP_SIZE; gx++)
        {
            // 如果当前点已经是 1 (空闲)，我们不改动它
            uint8_t current_s = GetStateFromBuf(row_buf[CONTOUR_DIST], gx);
            if (current_s == 1)
                continue;

            // 2. 核心检测：扫描 7x7 窗口内是否有 State 1
            uint8_t dist12_has_free = 0; // 半径 1-2 范围内是否有空闲
            uint8_t dist3_has_free = 0;  // 半径 3 边界上是否有空闲

            // 开始“雷达扫描”：以 (gx, gy) 为中心，检查周围 7x7 的区域
            // 这个 7 * 7 窗口的中心点 (gx, gy) 就是我们当前正在考察的、准备画轮廓线的点
            // dy 和 dx 是相对于中心点的偏移量，范围从 -3 到 +3
            for (int dy = -CONTOUR_DIST; dy <= CONTOUR_DIST; dy++)
            {
                for (int dx = -CONTOUR_DIST; dx <= CONTOUR_DIST; dx++)
                {
                    // 1. 计算邻居点的横坐标 nx
                    int nx = gx + dx;

                    // 2. 边界检查：如果邻居超出了地图左右边界，直接跳过，防止数组越界
                    if (nx < 0 || nx >= CAR_MAP_SIZE)
                        continue;

                    /* 3. 从 7 行缓冲区中提取邻居的状态
                     * row_buf 存储了 [gy-3, gy-2, gy-1, gy, gy+1, gy+2, gy+3] 这 7 行数据
                     * 所以 dy + CONTOUR_DIST (即 dy + 3) 正好将偏移量映射到索引 0~6
                     */
                    if (GetStateFromBuf(row_buf[dy + CONTOUR_DIST], nx) == 1)
                    // 这个函数并不是“只判断 x 邻居”，而是先通过 row_buf 的下标选好了 y 邻居所在的整行数据，再从这行数据里挑出对应的 x 邻居。
                    {
                        /* 4. 计算当前点与探测到的“空闲点”之间的距离
                         * 这里使用【切比雪夫距离】: d = max(|dx|, |dy|)
                         * 它的特点是：距离中心为 1 的点围成一个 3x3 的方块，距离为 2 的围成 5x5...
                         */
                        int d = abs(dx) > abs(dy) ? abs(dx) : abs(dy);

                        // 5. 分层标记
                        // 如果空闲点出现在半径 1 或 2 的范围内
                        if (d < CONTOUR_DIST)
                            dist12_has_free = 1;

                        // 如果空闲点恰好出现在半径为 3 的最外环上
                        if (d == CONTOUR_DIST)
                            dist3_has_free = 1;
                    }
                }
            }

            // 3. 判定准则：
            // 如果半径 3 处有空闲，且半径 1-2 处没有空闲（保证是第3格）
            if (dist3_has_free && !dist12_has_free)
            {
                SetStateToBuf(result_line, gx, 3); // 标记为占据/轮廓
            }
        }

        // 写回原地图
        memcpy(car_map[gy], result_line, MAP_ROW_BYTES);

        // 4. 滚动缓冲区：丢弃最老行，加载下一行
        for (int i = 0; i < 6; i++)
        {
            memcpy(row_buf[i], row_buf[i + 1], MAP_ROW_BYTES);
        }
        int next_y = gy + CONTOUR_DIST + 1;
        if (next_y < CAR_MAP_SIZE)
            memcpy(row_buf[6], car_map[next_y], MAP_ROW_BYTES);
        else
            memset(row_buf[6], 0, MAP_ROW_BYTES);
    }
}

/**
 * @brief car_map图像处理函数
 */
void Map_Global_Opening_InPlace(void)
{
    // 最开始运行开运算后，噪声变成一个一个圆点，像是椒噪声
    // 因此可能需要根据实际情况增加膨胀和腐蚀次数
    dilate_map();
    erode_map();
    dilate_map();
    Map_Generate_Boundary_Contour();
}

// 图像压缩和测试图像的生成-----------------------------------------------------------------------
/**
 * @brief 将 200x200 的原图按照meter_pixel压缩，然后取左上角64*64放到压缩后的图像
 * @note
 * 主要作用时在建图完成后在显示OLED的时候可以直接使用car_map_compressed缓存内容，增快main函数执行速度，减小卡顿
 * 原本在弄路径规划那边的时候，为了减少栈溢出导致运行时死机，因此更改了startup_stm32f103xb.s的EQU大小从0x400变成0x600
 * 但是加了car_map_compressed后RAM又爆了，因此把EQU大小改回默认值0x400
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

/**
 * @brief 生成一个带空心路径的 'C' 型障碍物
 * @details
 * - 内部空心区域填充 State 1
 * - 墙体填充 State 3
 * - 右下角末端位于地图中心 (100, 100)
 */
void Map_Generate_Test_C_Shape_Advanced(void)
{
    for (int y = 0; y < CAR_MAP_SIZE; y++)
        memset(car_map[y], 0x00, MAP_ROW_BYTES);

    int cX = 100, cY = 100;
    int length = 80;
    int lane_w = 15;
    int wall_t = 2;

    // --- 核心平移逻辑 ---
    // Y方向：让 (cY) 处于通道上下墙的中点
    int bottom = cY + (lane_w / 2);
    // X方向：让 (cX) 处于通道开口末端的中心。原本 right = cX，现在向右平移 lane_w / 2
    int right = cX + (lane_w / 2);

    int top = bottom - length;
    int left = right - length;

    // 1. 铺设走廊 (State 1)
    for (int y = top; y <= bottom; y++)
    {
        for (int x = left; x <= right; x++)
        {
            if ((y >= top && y < top + lane_w) ||
                (x >= left && x < left + lane_w) ||
                (y > bottom - lane_w && y <= bottom))
                SetMapValue_ByPixel(x, y, 1);
        }
    }

    // 2. 绘制墙壁 (State 3)
    // 外墙
    for (int t = 0; t < wall_t; t++)
    {
        for (int i = left - t; i <= right; i++)
        {
            SetMapValue_ByPixel(i, top - t, 3);
            SetMapValue_ByPixel(i, bottom + t, 3);
        }
        for (int i = top - t; i <= bottom + t; i++)
            SetMapValue_ByPixel(left - t, i, 3);

        // 内墙
        for (int x = left + lane_w; x <= right; x++)
        {
            SetMapValue_ByPixel(x, top + lane_w + t, 3);
            SetMapValue_ByPixel(x, bottom - lane_w - t, 3);
        }
        for (int y = top + lane_w; y <= bottom - lane_w; y++)
            SetMapValue_ByPixel(left + lane_w + t, y, 3);

        // 封口（向右平移后的断面）
        for (int y = top; y <= top + lane_w; y++)
            SetMapValue_ByPixel(right + t, y, 3);
        for (int y = bottom - lane_w; y <= bottom; y++)
            SetMapValue_ByPixel(right + t, y, 3);
    }
}
