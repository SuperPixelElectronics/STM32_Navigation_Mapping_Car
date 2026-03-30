#ifndef __SYSTEM_H
#define __SYSTEM_H

// 这里采用了轮趣小车的方案，使用system.h进行循环包含
// 1.main.c开头写#include "system.h"
// 2.system.h包含各种外设头文件
// 3.system.c里面定义各种公共变量，然后通过system.h的extern来扩大变量共享空间
// 4.外设头文件开头写#include "system.h"
// 5.这样写好处是可以很方便地共享变量，坏处是需要处理循环包含问题

// “外设头文件用到的类型定义”处的类型定义可以被外设头文件识别到，而“其他共享变量和定义”不能被外设头文件识别到的原因：
// 当你（或者某个外设头文件）包含 system.h 时，编译器的处理流程如下：
//  1.定义宏：执行 #define __SYSTEM_H。
//  2.定义类型：扫描并记录 PointType、VectorType 等结构体。此时，编译器已经记住了这些类型。
//  3.嵌套包含：执行 #include "tim.h"。
//  4.进入外设头文件：编译器跳到 tim.h 内部。
//  5.递归包含（关键点）：tim.h 内部写了 #include "system.h"。
//    a.编译器再次尝试进入 system.h。
//    b.此时 #ifndef __SYSTEM_H 判断失败（因为第一步已经定义过了）。
//    c.结果：这次嵌套包含会直接跳过 system.h 的所有内容。
//  6.回到外设逻辑：编译器继续往下读 tim.h 的剩余代码。
// 能识别类型的原因：因为在进入第 3 步之前，PointType 等定义已经在第 2 步处理完了。
// 不能识别宏/共享变量的原因：因为你的 NRF_RECEIVE_CHAR_NUM 等宏和 extern 变量写在 #include "tim.h" 的下方。编译器还没读到那里，就先钻进 tim.h 里面去了。
// 每个 .c 文件在编译时，其宏定义都是独立的内存空间。system.c 里的 __SYSTEM_H 和 tim.c 里的 __SYSTEM_H 互不相干

// 标准头文件
#include "stdio.h"
#include "string.h"
#include "math.h"
#include "stdlib.h"
#include "main.h"

// 外设头文件用到的类型定义

//    感知        |              决策              |          控制
// 电机编码器-----> 里程计融合信息--->地图-->路径规划-->轨迹追踪-->电机/执行器
//                   ^               ^
// MPU6050+磁力计  ---┘               |
// 建图信息---------------------------┘

// 基础信息------------------------------------------------------------
// 基础的点类型
typedef struct
{
    float x;
    float y;
    float z;
} PointType;

// 基础的向量类型
typedef struct
{
    float x;        // X坐标，单位：m
    float y;        // Y坐标，单位：m
    float theta;    // 朝向角，单位：rad，x轴正方向为0，逆时针为正
    float v;        // 线速度 m/s
    float angular;  //角速度 rad/s
} VectorType;

// 电机编码器------------------------------------------------------------
// 电机编码器处理后的信息
typedef struct
{
    // 同一个编译单元中不能重复定义了同名的结构体标签，因此在其他地方不能再定义_Encoder_Velocity_
    struct _Encoder_Velocity_
    {
        float motor_speed; // 小车的线速度，单位：m/s
        float motor_delta; // 小车的角速度，向前为0，逆时针为正，单位：rad/s
        uint32_t time;     // 本次时间戳
        uint32_t last_time;// 上一次的时间戳
    } Encoder_Velocity;
    VectorType Encoder_Position;    //由电机编码器计算出来的小车位置
} EncoderProcessed;

// MPU6050+磁力计信息------------------------------------------------------------
// MPU6050+磁力计读取到的原始信息
typedef struct
{
    PointType acceleration;     // 加速度
    float temperature;          // 温度
    PointType angular_velocity; // 角速度
    PointType angle;            // 角度
    PointType magnet;           // 磁场
    uint32_t time;              // 时间戳
} Mpu6050Data;

// 由MPU6050+磁力计数据计算出来的数据
typedef struct
{
    PointType vel;                      // 速度
    PointType pos;                      // 位置
    PointType last_accel;               // 上次加速度
    uint32_t last_time;                 // 上次时间
    FunctionalState is_first_sample;    // 是否是第一个样本
    PointType acceleration_no_gravity;  // 移除重力分类后的加速度
    PointType angle_remove_first;       // 移除上电后初始朝向角的数据
    PointType first_angle;              // 上电后的初始朝向角
    uint32_t time;                      // 时间戳
} Mpu6050DataProcessed;

// 里程计融合信息------------------------------------------------------------
// 编码器和MPU6050+磁力计融合后的小车里程计数据
typedef struct
{
    // 由编码器获取到的数据
    struct _Combined_Encoder_Velocity_
    {
        float motor_speed; // 小车的线速度，单位：m/s
        float motor_delta; // 小车的角速度，向前为0，逆时针为正，单位：rad/s
        uint32_t time;     // 本次时间戳
        uint32_t last_time;// 上一次时间戳
    } Encoder_Velocity;
    struct _Combined_EncoderPosition_
    {
        float theta; // 朝向角，单位：弧度，x轴正方向为0，逆时针为正
    } Encoder_Position;

    // 由MPU6050获取的数据
    struct _Mpu6050Data_
    {
        PointType angular_velocity; // 角速度
    } Mpu6050_Data;
    struct _Combined_Mpu6050DataProcessed_
    {
        PointType angle_remove_first;   // 移除了初始角度后的角度信息
        uint32_t time;                  // 时间戳
    } Mpu6050DataProcessed;

    //最后的处理结果
    VectorType CombinedPosition;
} CombinedProcessed;

// 建图信息------------------------------------------------------------
// 激光传感器检测到的障碍物相关信息
typedef struct
{
    PointType obstacle_position;    // 障碍点的位置
    FunctionalState status;         // 检测状态
} ObstacleType;

// 路径规划信息------------------------------------------------------------
// 路径规划求解器使用的障碍物类型
typedef enum CrackType
{
    FIND_SAFE = 0U, // 没有障碍物
    FIND_CRACK      // 有障碍物
} CrackType;

// 路径规划求解器使用的路径点解类型
typedef enum SolveType
{
    NO_FIND = 0U,       // 无解
    FIND_BACKED_SOLVE,  // 解在历史路径上
    FIND_EDGE_SOLVE,    // 解在障碍物边缘
    FIND_SPACIOUS_SOLVE // 解在空旷位置
} SolveType;

// 路径规划求解器得到的路径解
typedef struct
{
    PointType point;      // 当前解的数值
    SolveType solvestate; // 当前解的状态
} TrackPointType;

// 路径规划的状态信息
typedef enum
{
    SOLVE_OK,       // 求解成功
    STACK_OVERFLOW, // 求解堆栈溢出
    NO_SPACE_POINT, // 求解某个路径点时发现没有解(地图中的空旷区域)
    IDLE            // 求解器未运行
} SolveErrorType;

// 轨迹追踪器信息------------------------------------------------------------
// 轨迹追踪器结构
typedef struct
{
    PointType *track;          // 路径点数组指针
    uint16_t track_length;     // 路径点总数
    uint16_t near_index;       // 当前追踪的路径点索引（最近点）
    uint16_t lookahead_index;  // 预瞄点索引
    PointType lookahead_point; // 预瞄点
    PointType lookahead_bias;  // 预瞄点和当前位置的偏差
    float lookahead_distance;  // 前瞻距离 Ld (米)
    float target_speed;        // 目标线速度 v (米/秒)
    float k_pure_pursuit;      // 纯追踪增益系数
    FunctionalState state;     // 追踪器状态
    float slowdown_start_dist; // 開始減速的距離 (米)
    float stop_dist;           // 完全停止的距離 (米)
    // 输出
    float cmd_linear;  // 计算出的线速度指令
    float cmd_angular; // 计算出的角速度指令 (弧度/秒)
} TrajectoryTracker;

// 执行器信息------------------------------------------------------------
// NRF控制模式
typedef enum
{
    CAR_CONTROL,   // 相关参数由车子自己控制
    REMOTE_CONTROL // 相关参数完全由NRF控制
} NRFControlModeType;

// 控制模式
typedef enum
{
    HAND_PWM = 0U, // 玩家通过Motor菜单直接控制电机PWM
    HAND_LR,       // 玩家通过Motor菜单直接控制电机的目标速度
    HAND_XYZ,      // 玩家通过Motor菜单直接控制小车的线速度和角速度
    JOY_XYZ,       // 玩家通过Motor菜单的JOY选项控制小车
    HAND_TRACK,    // 玩家直接使用轨迹追踪器来控制小车
    NRF_CTRL,      // 玩家通过遥控器遥控小车
    HAND_MODE_NUM  // 控制模式的数量
} Control_Mode;

// 电机速度控制相关参数结构体
typedef struct
{
    float Encoder;     // 编码器数值，读取电机实时速度
    float Motor_Pwm;   // 电机PWM数值，控制电机实时速度
    float Target;      // 电机目标速度值，控制电机目标速度
    float Velocity_KP; // 速度控制PID参数
    float Velocity_KI; // 速度控制PID参数
    float Bias;        // 本次偏差
    float Last_bias;   // 上一次偏差
} Motor_parameter;

// 其他外设------------------------------------------------------------
// OLED页面
typedef enum
{
    MENU_PAGE = 1U,         // 菜单
    ENCODER_DATA_PAGE,      // 电机编码器数据
    ENCODER_GRAPH_PAGE,     // 电机编码器绘图
    MPU6050_DATA_PAGE,      // MPU6050+磁力计数据
    MPU6050_GRAPH_PAGE,     // MPU6050+磁力计绘图
    COMBINED_DATA_PAGE,     // 融合后的里程计数据
    COMBINED_GRAPH_PAGE,    // 融合后的里程计绘图
    MOTOR_CONTROL_PAGE,     // 电机控制页面
    NRF_PAGE,               // NRF数据
    LED_BUZZER_SYS_PAGE,    // LED 蜂鸣器 系统相关数据
    TRACK_DATA_PAGE,        // 轨迹追踪器数据
    TRACK_GRAPH_PAGE,       // 轨迹追踪器绘图
    MAP_DATA_PAGE,          // 地图数据
    MAP_GRAPH_PAGE,         // 地图绘图
    MAP_PURE_PAGE,          // 地图1:1绘图
    SOLVE_DATA_PAGE,        // 路径求解器数据
    SOLVE_GRAPH_PAGE,       // 路径求解器绘图
    OLED_PAGE_COUNT_TWOMORE // 用于获取这个枚举的元素个数，在光标越界检测中用到
} OLED_Page_Tpye;

// OLED光标
typedef struct
{
    int curser;                 // 当前光标位置
    int maxnum;                 // 当前页面中光标可移动的位置的数量
    FunctionalState selected;   // 光标是否处于选中状态
} Single_Curser_Tpye;
typedef struct
{
    Single_Curser_Tpye menu_page;          // 菜单页面的光标
    Single_Curser_Tpye encoder_data_page;  // 电机编码器数据的光标
    Single_Curser_Tpye combined_data_page; // MPU6050+磁力计数据的光标
    Single_Curser_Tpye motor_control_page; // 电机控制页面的光标
    Single_Curser_Tpye led_buzzer_page;    // LED 蜂鸣器 系统相关数据的光标
    Single_Curser_Tpye track_data_page;    // 轨迹追踪器数据的光标
    Single_Curser_Tpye map_data_page;      // 地图数据的光标
    Single_Curser_Tpye map_graph_page;     // 地图绘图的光标
    Single_Curser_Tpye map_pure_page;      // 地图1:1绘图的光标
    Single_Curser_Tpye solve_data_page;    // 路径求解器数据的光标
    Single_Curser_Tpye solve_graph_page;   // 路径求解器绘图的光标
} OLED_Curser_Tpye;

// LED灯颜色
typedef enum
{
    OFF = 0U,
    RED,
    GREEN,
    BLUE,
    YELLOW,
    PURPLE,
    CYAN,
    WHITE,
    BREATH
} Led_Color;

// 蜂鸣器信息
typedef struct
{
    uint32_t set_freq;      // 设置频率
    uint32_t set_delay;     // 设置占空比
    uint32_t real_freq;     //实际频率
    uint32_t real_delay;    //实际周期
    uint32_t time;          //时间戳
    uint32_t previous_time; //上一个时间戳
    FunctionalState status; //工作状态
} BuzzerParamType;

// 按键信息
typedef enum
{
    KEY_RELEASING = 0U, //正在被按下
    KEY_PRESSING        //松开中
} KeyNowType;
typedef enum
{
    KEY_ENABLED = 0U,
    KEY_DISABLED
} KeyStateType;
typedef enum
{
    KEY_UP = 0U,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_CENTER,
    KEY_NO
} KeyNumType;
typedef enum
{
    KEY_UP_SHORT = 0U,
    KEY_DOWN_SHORT,
    KEY_LEFT_SHORT,
    KEY_RIGHT_SHORT,
    KEY_CENTER_SHORT,
    KEY_UP_LONG ,
    KEY_DOWN_LONG,
    KEY_LEFT_LONG,
    KEY_RIGHT_LONG,
    KEY_CENTER_LONG,
    KEY_NO_HAPPEN
} KeySignalType;
typedef struct
{
    KeyNowType state;           // 状态
    KeyStateType short_pressed; // 短按信号
    KeyStateType long_pressed;  // 长按信号
    uint32_t press_time;        // 按下时间
    uint32_t start_tick;        // 消抖一瞬间的开始计时时间
    uint32_t press_start_tick;  // 确认按下后开始计时时间
} KeyType;

// 外设头文件
#include "tim.h"
#include "encoder.h"
#include "OLED.h"
#include "usart.h"
#include "mpu6050.h"
#include "key.h"
#include "user_interaction.h"
#include "adc.h"
#include "show.h"
#include "position.h"
#include "motor.h"
#include "nRF24L01_API.h"
#include "led.h"
#include "buzzer.h"
#include "uart_app.h"
#include "trajectory_tracker.h"
#include "car_map.h"
#include "path_planing.h"

// 最开始我看重了vl53l5cx的多区域检测功能，尝试使用，但是这个传感器需要在每次上电的时候由MCU来加载传输固件。固件在ST固件包的buffer.h中，体积非常大(150-200kb),导致无法放入stm32f103c8t6
// 先后想过 1.使用电脑压缩固件，在stm32移植解压算法，每次stm32启动的时候固件在里面在线解压  太复杂了！！！
//         2.每次上电的时候使用串口在线传输固件                                         每次上电的时候都要连一次电脑，非常麻烦，而且违背了“独立运行”的原则
// 最后使用vl53l5x，这个传感器只能检测1个点，但也已经够用
#include "vl53l1.h"

// 最开始这类宏定义被被我写成了“#define PI 3.14; ”导致编译时在C文件出现了分号报错，但是实际上是h文件写错了
#ifndef PI
#define PI 3.14159265358979f
#endif

// 其他共享变量和定义
// 传感器相关
extern float Encoder_precision, Wheel_perimeter, Wheel_spacing;
extern float k_motor_speed, b_motor_speed, k_motor_delta, b_motor_delta;
extern EncoderProcessed car_encoder_processed;
extern Mpu6050Data car_mpu6050_data;
extern Mpu6050DataProcessed car_mpu6050_processed;

extern float position_angle_combined_k;
extern CombinedProcessed car_combined_processed;

extern float vl53l1_distance; 
extern ObstacleType obstacle_info;

// 地图信息
extern FunctionalState map_detect_switch;
#define CAR_MAP_SIZE 200                                                       // 地图大小
#define BIT_PER_POINT 2                                                        // 每个点占的比特数 在2bit下：0:未探测, 1:空闲, 2:占据, 3:静态障碍
#define POINT_GAP 0.01                                                         // 地图中2个点格子之间的距离，单位为m
#define MAP_ROW_BYTES ((CAR_MAP_SIZE * BIT_PER_POINT) / (sizeof(uint8_t) * 8)) // 每行50字节
extern uint8_t car_map[CAR_MAP_SIZE][MAP_ROW_BYTES];

// 路径规划求解器信息
extern PointType car_goal_position;
#define MAX_SOLVE_NODES 128                    // 路径骨架最大节点数
extern TrackPointType solves[MAX_SOLVE_NODES];
extern SolveErrorType path_solve_status;
extern int solves_count;

// 轨迹追踪器信息
#define TRACK_LEN_MAX 129                  // 轨迹追踪器路径的最大长度，这里比MAX_SOLVE_NODES多了1，是为起点预留的
extern TrajectoryTracker tracker;
extern PointType car_track[TRACK_LEN_MAX]; // 这里最早未使用TRACK_LEN_MAX宏而是使用固定数字，结果反复修改的时候这里的数字和system.c对不上，导致报错
extern uint16_t current_track_length;

// 执行器信息
extern Control_Mode control_mode;
extern NRFControlModeType nrf_control_mode;
extern VectorType car_set_velocity;
extern float four_dirction_velocity, four_dirction_angular;
extern Motor_parameter MOTOR_A, MOTOR_B;

// NRF信息
// 最开始NRF_RECEIVE_CHAR_NUM的数量小于RX_PLOAD_WIDTH，导致在接收的时候，在nrf接收函数中，指向nrf_receive_buf的指针越界覆写了其他变量
#define NRF_RECEIVE_CHAR_NUM 32 // 接收缓冲区大小
#define NRF_SEND_CHAR_NUM 32    // 发送缓冲区大小
extern char nrf_receive_buf[NRF_RECEIVE_CHAR_NUM];
extern char nrf_send_buf[NRF_SEND_CHAR_NUM];
extern FlagStatus nrf_error_state;

// OLED信息
extern OLED_Page_Tpye oled_page;
extern int solve_data_page;
extern int track_data_page;
extern char oled_refresh_gap;
extern OLED_Curser_Tpye oled_curser;
extern FunctionalState display_max_time;

// OLED显示地图相关信息
#define METER_PIXEL 32 // 几个像素点代表1m (50)
// #define 仅仅是文本替换，它不是变量，也不会自动帮你加括号，因此需要手动加括号确保进行文本替换后在替换后的表达式里面运算顺序仍然正确
#define SHOW_CENTER_X (64) // 地图显示原点X坐标 (+25)
#define SHOW_CENTER_Y (32) // 地图显示原点Y坐标 (+20)
#define OLED_MAX_WIDTH 128 // OLED宽
#define OLED_MAX_HIGHT 64  // OLED高
extern int map_show_level;
extern PointType map_show_area, map_show_area_max;

// OLED显示的压缩地图缓存信息
#define COMPRESSED_SIZE 64                                          // 压缩后地图的尺寸，正好可以放入OLED
#define COMPRESSED_BYTE_NUM (COMPRESSED_SIZE * BIT_PER_POINT / 8)   // 压缩后地图每一行的字节数
extern uint8_t car_map_compressed[COMPRESSED_SIZE][COMPRESSED_BYTE_NUM];
extern FunctionalState car_map_compressed_complete;

// 按钮信息
#define KEY_LONG_CNT 10  // 长按的tick判别阈值
extern int key_adc_val;
extern KeyType car_key[5];

// LED和蜂鸣器信息
extern Led_Color car_led_color;
extern BuzzerParamType car_buzzer_param;

// 和电脑交互的串口信息
#define UART1_TX_SIZE 64    // 发送缓冲区大小
#define UART1_RX_SIZE 64    // 接收缓冲区大小
extern char uart1_send_buf[UART1_TX_SIZE];
extern char uart1_rxdata[UART1_RX_SIZE];
extern int last_rx_time;

// main函数运行时间分析信息
extern int main_start_tick, main_all_tick, main_all_tick_max;
extern int main_during_tick[10];
extern int main_during_tick_max[10];

#endif
