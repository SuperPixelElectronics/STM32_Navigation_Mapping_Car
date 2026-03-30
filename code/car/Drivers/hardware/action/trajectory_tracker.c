#include "trajectory_tracker.h"

// private start----------------
/**************************************************************************
函数功能：计算两点间距离的平方
入口参数：两个点的x和y坐标
返回  值：距离的平方
**************************************************************************/
static float distance_sq(float x1, float y1, float x2, float y2)
{
    float dx = x1 - x2;
    float dy = y1 - y2;
    return dx * dx + dy * dy;
}
// private end--------------------

/**************************************************************************
函数功能：初始化轨迹追踪器
入口参数：tracker            :轨迹追踪器句柄
         track              :需要走的路径数组的首地址
         len                :需要走的路径数组的长度
         lookahead_distance :预锚距离
         speed              :设定速度
返回  值：无
**************************************************************************/
void Tracker_Init(TrajectoryTracker *tracker, PointType *track, uint16_t len, float lookahead_distance, float speed)
{
    tracker->track = track;
    tracker->track_length = len;
    tracker->near_index = 0;
    tracker->lookahead_index = 0;
    tracker->lookahead_point.x = 0, tracker->lookahead_point.y = 0, tracker->lookahead_point.z = 0;
    tracker->lookahead_bias.x = 0, tracker->lookahead_bias.y = 0, tracker->lookahead_bias.z = 0;
    tracker->lookahead_distance = lookahead_distance;
    tracker->target_speed = speed;
    tracker->k_pure_pursuit = 0.2f; // 默认增益
    tracker->state = ENABLE;

    // 定義減速區間參數
    tracker->slowdown_start_dist = 0.10f; // 開始減速的距離 (米)
    tracker->stop_dist = 0.10f;           // 完全停止的距離 (米)
    
    tracker->cmd_linear = 0.0f;
    tracker->cmd_angular = 0.0f;
}

/**************************************************************************
函数功能：轨迹追踪器更新函数
入口参数：tracker            :轨迹追踪器句柄
         current_pose       :小车现在的位置
返回  值：无，但是函数会修改tracker的cmd_linear和cmd_angular
**************************************************************************/
void Tracker_Update(TrajectoryTracker *tracker, VectorType current_pose)
{
    if (tracker->track_length == 0)
        return;

    // **步骤1: 寻找最近点**-------------------------------------------------------------------------------------
    uint16_t nearest_index = tracker->near_index;
    float min_dist_sq = distance_sq(current_pose.x, current_pose.y,
                                    tracker->track[nearest_index].x,
                                    tracker->track[nearest_index].y);
    // 从当前索引开始，向前搜索一段范围，避免每次都从头搜索
    // 问题：如果当路径出现锐角或回圈，小车在走后面节点的时候可能会很靠近之前走过的前面的节点，造成小车认为自己“进度倒退了”，结果小车开始走前面的节点去了
    // 解决方法：search_start 不再向前回溯（不再执行 -5 操作）,使得
    uint16_t search_start = nearest_index;
    uint16_t search_end = (nearest_index + 20 < tracker->track_length) ? (nearest_index + 20) : tracker->track_length;
    for (uint16_t i = search_start; i < search_end; ++i)
    {
        float dist_sq = distance_sq(current_pose.x, current_pose.y,
                                    tracker->track[i].x, tracker->track[i].y);
        if (dist_sq < min_dist_sq)
        {
            min_dist_sq = dist_sq;
            nearest_index = i;
        }
    }
    tracker->near_index = nearest_index;

    // **步骤2: 寻找预瞄点**-------------------------------------------------------------------------------------
    // “预瞄点”（Lookahead Point）一定是超过当前车辆位置对应的最近路径点，而位于车辆前方一段距离的某个路径点
    uint16_t lookahead_index = nearest_index;
    float accumulated_dist = 0.0f;

    // 从最近点开始，沿路径累加距离，直到超过前瞻距离
    for (uint16_t i = nearest_index; i < tracker->track_length - 1; ++i)
    {
        float seg_dx = tracker->track[i + 1].x - tracker->track[i].x;
        float seg_dy = tracker->track[i + 1].y - tracker->track[i].y;
        accumulated_dist += sqrtf(seg_dx * seg_dx + seg_dy * seg_dy);

        // 问题：我目前的路径点总数是PATH_LEN=128，定义如下
        // PointType car_track[TRACK_LEN_MAX] = {
        // {0.0f, 0.0f},{0.0f, 0.5f},{0.0f, 1.0f},{0.5f, 1.0f},{1.0f, 1.0f},
        // {1.5f, 1.0f},{2.0f, 1.0f},{2.5f, 1.0f},{3.0f, 1.0f}
        // };
        // Tracker_Init时定义len也为128
        // 可是当near index和lookaheead index都为9的时候，当运行到if(accumulated_dist >= tracker->lookahead_distance) 时，
        // 发现accumulated_dist怎么加都为0(因为后面的点都为(0, 0))。此时tracker->lookahead_distance = 0.1。
        // 但是代码却跳入这个循环了。更诡异的是，运行lookahead_index = i + 1;时发现lookahead_index 永远为9，像是被焊死一样
        // 原因解析：
        /* * 底层逻辑拆解 (基于 ARM Cortex-M3 指令集):
         * -------------------------------------------------------------------------
         * 寄存器分配参考:
         * r4: tracker 结构体基地址
         * r0, r1: 浮点运算的操作数 (用于比较)
         * r6: 循环变量 i (uint16_t)
         * r7: 预瞄点索引 lookahead_index (uint16_t)
         * sp: 栈指针 (存储局部变量 accumulated_dist)
         * -------------------------------------------------------------------------
         */

        if (accumulated_dist >= tracker->lookahead_distance)
        // -------------------------------------------------------------------------
        // [0x08005D96] LDR r1, [r4, #0x24]  --> 从结构体偏移 0x24 处取出 lookahead_distance
        // [0x08005D98] LDR r0, [sp, #0x18]  --> 从栈中取出你累加好的 accumulated_dist
        // [0x08005D9A] BL.W __aeabi_cfrcmple --> 调用浮点比较函数 (F103没有硬件浮点，这步很重!)
        // [0x08005D9E] BHI 0x08005DA6       --> 【关键跳跃】: 如果 dist < Ld (条件不满足),
        //                                       直接“瞬移”跳过 if 内部，去执行 [0x5DA6]
        // -------------------------------------------------------------------------
        {
            lookahead_index = i + 1;
            break;
            // ---------------------------------------------------------------------
            // 注意：如果上面 [0x5D9E] 发生了跳转，这两行代码在物理上就会被 CPU “无视”
            // ---------------------------------------------------------------------
        }
        // -------------------------------------------------------------------------
        // [0x08005DA0] ADDS r6, r6, #1      --> i = i + 1 (循环自增)
        // [0x08005DA2] UXTH r7, r6           --> 确保 i 是 16 位无符号数，并同步给 lookahead_index
        // [0x08005DA4] B 0x08005DB2         --> 无条件跳转回循环顶部的条件检查 (Check: i < len-1)
        // -------------------------------------------------------------------------

        /*
         * 总结：为什么条件没满足仍然跳入if语句，而且if内的 index 不动仿佛会“焊死”：
         * 1. 因为你的后段路径全是 (0,0)，accumulated_dist 永远加不动。
         * 2. 编译器通过编译优化，把if (accumulated_dist >= tracker->lookahead_distance)变成if (accumulated_dist < tracker->lookahead_distance),如果满足则跳过if分支，否则继续执行
         * 3. 于是 [0x5D9E] 处的 BHI (跳跃) 永远成立，导致 if 内部的赋值语句被永久跳过。
         * 4. 由于编译优化，造成实际代码和原本C语言代码无法直接对应，造成了Keil的显示和实际代码无法对应
         * 5. lookahead_index (r7) 只能跟着循环变量 i (r6) 的自增逻辑默默运行到结束。
         */

        // 解决方法：在路径规划完成填充数组时，记录下真实点的个数。然后在Tracker_Init时定义len为真实点的个数，而不是128.
        // 这样for (uint16_t i = nearest_index; i < tracker->track_length - 1; ++i)就可以直接正常结束，不必处理后面的无效点
    }
    // 如果路径已到尽头，则取最后一个点
    if (lookahead_index >= tracker->track_length)
    {
        lookahead_index = tracker->track_length - 1;
    }
    tracker->lookahead_index = lookahead_index;
    tracker->lookahead_point = tracker->track[tracker->lookahead_index];

    // **步骤3: 将预瞄点转换到车辆坐标系**-------------------------------------------------------------------------------------
    // 车辆坐标系：车头朝向为X轴正方向，左侧为Y轴正方向
    // 小车离下一个点的位置偏差
    tracker->lookahead_bias.x = tracker->lookahead_point.x - current_pose.x;
    tracker->lookahead_bias.y = tracker->lookahead_point.y - current_pose.y;

    // 旋转矩阵：将全局坐标转换到车辆坐标系
    float sin_theta = sinf(current_pose.theta);
    float cos_theta = cosf(current_pose.theta);
    float lookahead_x_v = tracker->lookahead_bias.x * cos_theta + tracker->lookahead_bias.y * sin_theta;  // 车辆坐标系下的下一个点坐标
    float lookahead_y_v = -tracker->lookahead_bias.x * sin_theta + tracker->lookahead_bias.y * cos_theta; // 车辆坐标系下的下一个点Y坐标

    // **步骤4: 计算曲率和角速度 (Pure Pursuit 核心公式)**-------------------------------------------------------------------------------------
    // 曲率 κ = 2 * y_v / (Ld²)
    // 角速度 ω = v * κ
    float curvature = 2.0f * lookahead_y_v / (tracker->lookahead_distance * tracker->lookahead_distance);

    // 计算预瞄点的方位角 (Relative Heading) atan2f 返回范围是 (-PI, PI)
    float angle_to_lookahead = atan2f(lookahead_y_v, lookahead_x_v);

    // **步驟5: 处理终点减速逻辑**-------------------------------------------------------------------------------------

    // 計算小車當前位置到路徑終點的直線距離
    PointType end_point = tracker->track[tracker->track_length - 1];
    float dx_to_end = end_point.x - current_pose.x;
    float dy_to_end = end_point.y - current_pose.y;
    float dist_to_end_sq = dx_to_end * dx_to_end + dy_to_end * dy_to_end; // 使用平方距離避免開方
    float dist_to_end = sqrtf(dist_to_end_sq);
    float desired_speed = tracker->target_speed; // 期望線速度

    // 1.最开始小车到达路径终点后，电机仍然在转动，后续行驶轨迹随机了。因此需要加入终点判定，让小车到终点后停止
    // 2.如果我规划的路径是一个圈又回到起点，会出现一开始就识别到到终点进入if (dist_to_end < tracker->slowdown_start_dist)，造成追踪器直接被关闭
    // 解决办法：
    // 加入终点减速逻辑，并且只有当最近点索引超过路径总长度的一半时，才允许进入终点减速逻辑
    // 这样在一开始即使离终点物理距离很近，但因为索引还很小，不会触发停止
    volatile uint8_t is_near_end_of_path = (tracker->near_index >= (tracker->track_length / 2));
    if (is_near_end_of_path)
    {
        // 此时再计算到终点的物理距离
        PointType end_point = tracker->track[tracker->track_length - 1];
        float dx_to_end = end_point.x - current_pose.x;
        float dy_to_end = end_point.y - current_pose.y;
        float dist_to_end = sqrtf(dx_to_end * dx_to_end + dy_to_end * dy_to_end);

        // 根據到終點的距離，平滑地減少線速度，直至為零。
        if (dist_to_end < tracker->slowdown_start_dist)
        {
            if (dist_to_end > tracker->stop_dist)
            {
                // 線性減速：距離終點越近，速度越慢
                desired_speed = tracker->target_speed * ((dist_to_end - tracker->stop_dist) / (tracker->slowdown_start_dist - tracker->stop_dist));
            }
            else
            {
                // 已進入停止範圍，速度設為0，並將追蹤器狀態置為 DISABLE
                desired_speed = 0.0f;
                if (nrf_control_mode == CAR_CONTROL)
                {
                    tracker->state = DISABLE;
                }
                // 可選：重置下一個追蹤點索引，便於後續重新啟用
                // tracker->near_index = 0;
            }
        }
    }

    // **步驟6: 生成控制指令**-------------------------------------------------------------------------------------
    // 如果下一个点正好在小车的后面，按照 Pure Pursuit 公式，算出的角度会很小，导致小车直接直着走，离下一个点原来越远
    // 应该加入判断,如果在后面则需要让小车先原地掉头
    if (fabsf(angle_to_lookahead) > 1.047f) // 1.047rad ≈ 60度，可以根据你的舵机/电机性能调整
    {
        // --- 原地转向模式 ---
        // 目标点角度偏差太大，先原地转正车头
        tracker->cmd_linear = 0.4f;

        // 使用一个简单的比例控制来实现转向
        float k_rotate = 20.0f; // 转向增益
        tracker->cmd_angular = angle_to_lookahead * k_rotate;

        // 限制最大转向速度，防止摆动过猛
        if (tracker->cmd_angular > 17.0f)
            tracker->cmd_angular = 17.0f;
        if (tracker->cmd_angular < -17.0f)
            tracker->cmd_angular = -17.0f;
    }
    else
    {
        // --- 正常 Pure Pursuit 模式 ---
        tracker->cmd_linear = desired_speed;

        // 传统的 Pure Pursuit 公式
        // 最开始小车进行轨迹行驶的时候，当小车到达轨迹的转弯点的时候，这个轨迹中的模块会立刻立刻输出一个让小车能够偏转很大角速度，
        // 然后小车开始剧烈偏转，偏转之后小车角度超过了轨迹模块，又超过了他原本应该正确行驶路径所这角度一些轨迹模块要输出一个很大负的角速度导致小车不停在原地摇摆
        // 后面通过调整k_pure_pursuit大小后解决
        tracker->cmd_angular = desired_speed * curvature * tracker->k_pure_pursuit;
    }

    // 如果追蹤器已被禁用（例如在停止區間內被設置），確保指令為零。
    if (tracker->state == DISABLE)
    {
        tracker->cmd_linear = 0.0f;
        tracker->cmd_angular = 0.0f;
    }

    // 可选：根据曲率限制最大速度，弯道减速
    // if (fabs(curvature) > 0.5f) {
    //     tracker->cmd_linear = tracker->target_speed * 0.7f;
    // }
}
