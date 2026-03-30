#include "path_planing.h"

#define MAX_DEPTH 7 // 递归最大深度

/**
 * @brief 路径分割任务单元
 * @details 该结构体用于非递归寻径算法。算法将复杂的长路径拆解为多个
 * 简单的线段任务，通过检查每一段任务是否碰撞来决定是否继续拆分。
 */
typedef struct
{
    PointType p_start; // 路径段的起始坐标 (World Frame)
    PointType p_end;   // 路径段的终止坐标 (World Frame)
    int depth;         // 当前探测深度 (Depth Level)
} SegmentTask;
SegmentTask stack[MAX_DEPTH + 1 + 5] = {0}; // 手动模拟 路径分割任务栈

#define SAFETY_MARGIN 0.05f          // 发现空隙后额外推入
#define PATH_HISTORY_THRESHOLD 0.05f // 历史点避让阈值 (5cm)

// private start------------------
/**
 * @brief  寻找碰撞点附近的最近安全缝隙 (Gap Searching)
 * * @details 当规划器发现 A -> B 连线在 collision 处被阻断时，该函数会沿着
 * 垂直于 A-B 连线的法线方向(正交方向)，向左和向右交替探测，
 * 寻找处于 State 1 的像素点。
 * 这样找到的点既避开了障碍物，又尽可能地贴近原始路径。
 * @param  start     [In]  路径段的起始坐标 (单位: 米)
 * @param  goal      [In]  路径段的目标坐标 (单位: 米)
 * @param  collision [In]  探测到的障碍物碰撞坐标 (单位: 米)
 * @param  gap       [Out] 找到的第一个安全切点坐标 (仅在函数返回有解时有效)
 * @return SolveType 执行结果：
 * @retval NO_FIND             无解
 * @retval FIND_BACKED_SOLVE   解在历史路径上
 * @retval FIND_EDGE_SOLVE     解在障碍物边缘
 * @retval FIND_SPACIOUS_SOLVE 解在空旷位置
 */
static SolveType FindNearestGap(PointType start, PointType goal, PointType collision, PointType *gap)
{
    float dx = goal.x - start.x;
    float dy = goal.y - start.y;
    float d = sqrtf(dx * dx + dy * dy + 0.000001f); // 防止除零

    // 法线和原本的线垂直，向量相乘为0
    volatile float nx = -dy / d; // 法线方向
    volatile float ny = dx / d;

    PointType backup_solve = {0};
    int has_backup = 0;

    // 左右交替探测，步长 0.01m
    for (float offset = 0.01f; offset < 1.5f; offset += 0.01f)
    {
        for (int sign = -1; sign <= 1; sign += 2)
        {
            float test_x = collision.x + nx * offset * sign;
            float test_y = collision.y + ny * offset * sign;

            if (GetMapValue(test_x, test_y) == 1)
            {
                // 小车碰到障碍的时候会在障碍法线方向寻找空旷位置，可以如果小车刚好是急转弯绕过障碍的，可能会出现解出的障碍点在小车已经规划的路径上，造成小车来回走动。
                // 因此需要在解出结果后判断是否在solve数组的路径附近，如果是，则跳过当前解尝试寻找下一个解。除非实在找不到其他解，要返回NO_FIND时才会使用这个解，并返回FIND_BACKED_SOLVE。
                // --- 历史路径查重 ---
                int too_close_to_history = 0;
                for (int i = 0; i < solves_count; i++)
                {
                    float hdx = test_x - solves[i].point.x;
                    float hdy = test_y - solves[i].point.y;
                    // 使用平方距离判断，效率更高
                    if ((hdx * hdx + hdy * hdy) < (PATH_HISTORY_THRESHOLD * PATH_HISTORY_THRESHOLD))
                    {
                        too_close_to_history = 1;
                        break;
                    }
                }

                // 在最开始寻找空地的算法中，如果发现找到空旷区域了，算法会马上认为这个是求解结果。但是这个结果通常靠近边缘，
                // 导致小车可能撞墙。更严重的是，进行下一轮迭代时，系统会算出障碍物就在当前求解结果很近的地方，从而会使得下一个求解结果又和当前求解结果靠很近，这会严重消耗栈空间
                // 因此，需要在找到空旷区域后，让探测点继续向外多走几个像素（例如 5-10cm），确保小车位于通道中心附近
                // 计算推离点
                float safe_x = test_x + nx * SAFETY_MARGIN * sign;
                float safe_y = test_y + ny * SAFETY_MARGIN * sign;
                int is_spacious = (GetMapValue(safe_x, safe_y) == 1);

                // 如果这个点离历史路径太近，先存为备份，继续往后找
                if (too_close_to_history)
                {
                    if (!has_backup)
                    {
                        backup_solve.x = is_spacious ? safe_x : test_x;
                        backup_solve.y = is_spacious ? safe_y : test_y;
                        has_backup = 1;
                    }
                    continue; // 跳过，寻找更远的、不重叠的解
                }

                // 找到一个既安全又不重叠的解
                gap->x = is_spacious ? safe_x : test_x;
                gap->y = is_spacious ? safe_y : test_y;
                gap->z = 0.0f;
                return is_spacious ? FIND_SPACIOUS_SOLVE : FIND_EDGE_SOLVE;
            }
        }
    }
    // --- 备选机制：如果实在找不到新路，被迫使用备份解 ---
    if (has_backup)
    {
        *gap = backup_solve;
        gap->z = 0.0f;
        return FIND_BACKED_SOLVE;
    }

    return NO_FIND;
}

/**
 * @brief 探测 A->B 直线上遇到的第一个障碍点
 * @param A A点信息
 * @param B B点信息
 * @param collision 如果由碰撞，通过此参数返回障碍物信息
 * @return 1: 发现碰撞, 0: 直线安全
 */
static CrackType GetFirstCollision(PointType A, PointType B, PointType *collision)
{
    float dx = B.x - A.x;
    float dy = B.y - A.y;
    float dist = sqrtf(dx * dx + dy * dy);

    // 每 1cm 探测一个点，确保不会漏掉细小的 State 3
    int steps = (int)(dist / 0.01f) + 1;

    for (int i = 1; i < steps; i++)
    {
        float t = (float)i / steps;
        float cx = A.x + t * dx;
        float cy = A.y + t * dy;

        // 只要不是空闲(1)，都视为障碍
        if (GetMapValue(cx, cy) != 1)
        {
            collision->x = cx;
            collision->y = cy;
            collision->z = 0;
            return FIND_CRACK;
        }
    }
    return FIND_SAFE;
}

// private end------------------

/**
 * @brief 非递归版蛇形寻径
 * @note 直接先连接start和end，然后看中间是否穿过障碍物，如果有，则寻找新的点，需要反复迭代，直到找到可行解为止
 * @param vstart 起始位置
 * @param goal 目标位置
 * @param path 求解路径结果，通过这个数组返回
 * @param path_len 求解路径结果的长度，通过这个指针返回
 */
void PlanPath_Iterative(VectorType vstart, PointType goal, PointType *path, uint16_t *path_len)
{
    int stack_ptr = 0;
    solves_count = 0;

    // 初始任务：从起点到终点
    PointType start = (PointType){vstart.x, vstart.y, 0.0f};
    stack[stack_ptr] = (SegmentTask){start, goal, 0}; // 入栈
    stack_ptr++;

    while (stack_ptr > 0)
    {
        // 出栈一个任务
        stack_ptr--;
        SegmentTask current = stack[stack_ptr];

        /**
         * @brief 碰撞点坐标 (Collision Point)
         * @details 当算法使用 GetFirstCollision 函数探测 p_start 到 p_end 的直线路径时，
         * 如果发现中间有障碍物（State 0, 3），会将该障碍物的精确世界坐标存入 Collision。
         * 它是后续 FindNearestGap 函数寻找“避障出口”的参考基准点。
         */
        PointType Collision;
        SolveType current_solve_state = FIND_SPACIOUS_SOLVE;
        // 1. 检查当前段
        if (GetFirstCollision(current.p_start, current.p_end, &Collision) == FIND_CRACK)
        {
            // 如果有碰撞且未达深度上限，进行切分
            if (current.depth < MAX_DEPTH)
            {
                PointType solve = {0}; // 当前解
                current_solve_state = FindNearestGap(current.p_start, current.p_end, Collision, &solve);
                if (current_solve_state == FIND_SPACIOUS_SOLVE || current_solve_state == FIND_EDGE_SOLVE || current_solve_state == FIND_BACKED_SOLVE)
                {
                    // 从目标位置到当前解的路径还未知，需要再次入栈，后面去处理
                    // 【顺序保障】：先压后半段，再压前半段
                    // 这样栈会先弹出前半段进行处理，保证了路径从 start 到 goal 的顺序
                    stack[stack_ptr++] = (SegmentTask){solve, current.p_end, current.depth + 1};
                    stack[stack_ptr++] = (SegmentTask){current.p_start, solve, current.depth + 1};
                    continue;
                }
                else
                {
                    // 如果无法避障或深度达到上限，该段失败
                    path_solve_status = NO_SPACE_POINT;
                }
            }
            else
            {
                // 如果无法避障或深度达到上限，该段失败
                path_solve_status = STACK_OVERFLOW;
            }
        }
        else
        {
            // 最开始的求解算法中，一旦求解出新的点，就马上放入求解结果中，可是万一算出的当前求解结果和上一个点之间有障碍，会造成需要在当前求解结果之前需要插入新的点。
            // 而原本做法似乎会造成求解结果顺序混乱。正确方法是应该要确认前一个点到当前求解结果之间已经连通无障碍才可以存储求解结果

            // 【核心逻辑】：只有当前段完全安全(FIND_SAFE)时，才记录该段的终点
            // 这保证了 solves 数组里的点是按行驶顺序依次排列的
            if (solves_count < MAX_SOLVE_NODES)
            {
                solves[solves_count].point = current.p_end;
                solves[solves_count].solvestate = current_solve_state; // 确保每个存入的点都有明确的状态归属
                solves_count++; // 补上分号
            }
            path_solve_status = SOLVE_OK;
        }
    }
    // --- 路径数组填充逻辑 ---

    // 1. 起点固定放在 path[0]
    path[0] = start;

    // 2. 将避障过程中产生的中继点拷入 path
    // 注意：solves 里的最后一个点通常就是 goal，
    // 我们将其按顺序放入 path[1] 开始的位置
    // 算出的解中，最后一个解就是终点坐标
    for (int i = 0; i < solves_count; i++)
    {
        if (i + 1 < TRACK_LEN_MAX - 1) // 留一个位置给最后的 goal 强制覆盖
        {
            path[i + 1] = solves[i].point;
        }
    }

    // 3. 【关键：复利思维】计算真实有效长度
    // 真实长度 = 起点(1) + 中继点总数(solves_count) + 终点(1)
    uint16_t actual_valid_len = solves_count + 2;

    // 4. 数据清洗：将数组剩余部分填充为 goal (可选，但建议保留)
    // 这样即便后面逻辑出错扫到了后面，也是在终点打转
    for (int j = actual_valid_len -1 ; j < TRACK_LEN_MAX; j++)
    {
        path[j] = goal;
    }

    *path_len = actual_valid_len;
}
