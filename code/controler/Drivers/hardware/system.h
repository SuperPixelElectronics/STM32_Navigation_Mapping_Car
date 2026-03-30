#ifndef __SYSTEM_H
#define __SYSTEM_H

// 标准头文件
#include "stdio.h"
#include "string.h"
#include "math.h"
#include "stdlib.h"
#include "main.h"

// 外设头文件用到的类型定义

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
    float x;       // X坐标，单位：m
    float y;       // Y坐标，单位：m
    float theta;   // 朝向角，单位：rad，x轴正方向为0，逆时针为正
    float v;       // 线速度 m/s
    float angular; // 角速度 rad/s
} VectorType;

// 小车里程计融合信息------------------------------------------------------------
// 编码器和MPU6050+磁力计融合后的小车里程计数据
typedef struct
{
    VectorType CombinedPosition; // 最后的处理结果
} CombinedProcessed;

// 小车轨迹追踪器信息------------------------------------------------------------
// 轨迹追踪器结构
typedef struct
{
    uint16_t near_index;            // R 当前追踪的路径点索引（最近点）
    uint16_t lookahead_index;       // R 预瞄点索引
    FunctionalState state_feedback; // R 追踪器状态读取值
    FunctionalState state_set;      // W 追踪器状态写入值
} TrajectoryTracker;

// 执行器信息------------------------------------------------------------
// NRF控制模式
typedef enum
{
    CAR_CONTROL,   // 相关参数由车子自己控制
    REMOTE_CONTROL // 相关参数完全由NRF控制
} NRFControlModeType;

// NRF状态
typedef enum
{
    NRF_IDLE = 0U,      // 空闲状态
    NRF_WAIT_FOR_PARAM, // 正在等待小车回传参数数据
    NRF_WAIT_FOR_MAP,   // 正在等待小车回传地图数据
    NRF_WAIT_FOR_TRACK, // 正在等待小车回传路径数据
} NRF_statusType;

// 其他外设------------------------------------------------------------
// OLED页面
typedef enum
{
    MENU_PAGE = 1U,         // 菜单
    CONTROL_PAGE,           // 控制小车
    SOLVE_GRAPH_PAGE,       // 路径求解器绘图
    TRACK_DATA_PAGE,        // 轨迹追踪器数据
    MPU6050_DATA_PAGE,      // 遥控器MPU6050+磁力计数据
    LED_BUZZER_PAGE,        // LED 蜂鸣器 系统相关数据
    NRF_PAGE,               // NRF数据
    OLED_PAGE_COUNT_TWOMORE // 用于获取这个枚举的元素个数，在光标越界检测中用到
} OLED_Page_Tpye;

// OLED光标
typedef struct
{
    int curser;               // 当前光标位置
    int maxnum;               // 当前页面中光标可移动的位置的数量
    FunctionalState selected; // 光标是否处于选中状态
} Single_Curser_Tpye;
typedef struct
{
    Single_Curser_Tpye menu_page;        // 菜单页面的光标
    Single_Curser_Tpye control_page;     // 控制小车的光标
    Single_Curser_Tpye solve_graph_page; // 路径求解器绘图的光标
    Single_Curser_Tpye track_data_page;  // 轨迹追踪器数据的光标
    Single_Curser_Tpye led_buzzer_page;  // LED 蜂鸣器 相关数据的光标
} OLED_Curser_Tpye;

// LED灯颜色
typedef enum
{
    OFF = 0U,
    RED,
    GREEN,
    BLUE,
    YELLOW,
    BREATH
} Led_Color;

// 蜂鸣器信息
typedef struct
{
    uint32_t set_freq;      // 设置频率
    uint32_t set_volume;    // 设置音量
    FunctionalState status; // 工作状态
} BuzzerParamType;

// 按键信息
typedef enum
{
    KEY_RELEASING = 0U, // 正在被按下
    KEY_PRESSING        // 松开中
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
    KEY_EDGE1,
    KEY_EDGE2,
    KEY_TRACK_CENTER,
    KEY_NO
} KeyNumType;
typedef enum
{
    KEY_UP_SHORT = 0U,
    KEY_DOWN_SHORT,
    KEY_LEFT_SHORT,
    KEY_RIGHT_SHORT,
    KEY_CENTER_SHORT,
    KEY_EDGE1_SHORT,
    KEY_EDGE2_SHORT,
    KEY_TRACK_CENTER_SHORT,
    KEY_UP_LONG,
    KEY_DOWN_LONG,
    KEY_LEFT_LONG,
    KEY_RIGHT_LONG,
    KEY_CENTER_LONG,
    KEY_EDGE1_LONG,
    KEY_EDGE2_LONG,
    KEY_TRACK_CENTER_LONG,
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
typedef struct
{
    GPIO_PinState up_key;
    GPIO_PinState down_key;
    GPIO_PinState left_key;
    GPIO_PinState right_key;
    GPIO_PinState last_up_key;
    GPIO_PinState last_down_key;
    GPIO_PinState last_left_key;
    GPIO_PinState last_right_key;
    PointType track_key_value;      //处理结果
} TrackKeyType;

// 遥控器MPU6050+磁力计信息
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
    PointType angle_remove_first; // 移除上电后初始朝向角的数据
    PointType first_angle;        // 上电后的初始朝向角
} Mpu6050DataProcessed;


// 外设头文件
#include "tim.h"
#include "OLED.h"
#include "usart.h"
#include "mpu6050.h"
#include "key.h"
#include "show.h"
#include "nRF24L01_API.h"
#include "led.h"
#include "buzzer.h"
#include "uart_app.h"
#include "car_map.h"
#include "user_interaction.h"

#ifndef PI
#define PI 3.14159265358979f
#endif

// 其他共享变量和定义
// 小车位置信息
extern CombinedProcessed car_combined_processed;

// 小车地图信息
extern FunctionalState map_detect_switch_feedback;
extern FunctionalState map_detect_switch_set;
#define CAR_MAP_SIZE 200                                                       // 地图大小
#define BIT_PER_POINT 2                                                        // 每个点占的比特数 在2bit下：0:未探测, 1:空闲, 2:占据, 3:静态障碍
#define POINT_GAP 0.01                                                         // 地图中2个点格子之间的距离，单位为m
#define MAP_ROW_BYTES ((CAR_MAP_SIZE * BIT_PER_POINT) / (sizeof(uint8_t) * 8)) // 每行50字节
extern uint8_t car_map[CAR_MAP_SIZE][MAP_ROW_BYTES];

// 小车目标位置信息
extern PointType car_goal_position_feedback;
extern PointType car_goal_position_set;

// 小车轨迹追踪器信息
#define TRACK_LEN_MAX 129 // 轨迹追踪器路径的最大长度，这里比MAX_SOLVE_NODES多了1，是为起点预留的
extern TrajectoryTracker tracker;
extern PointType car_track[TRACK_LEN_MAX];
extern uint16_t current_track_length;

// 小车执行器信息
extern VectorType car_set_velocity;
extern VectorType car_feedback_velocity;
extern NRFControlModeType nrf_control_mode;

// 遥控器NRF信息
#define NRF_RECEIVE_CHAR_NUM 32 // 接收缓冲区大小
#define NRF_SEND_CHAR_NUM 32    // 发送缓冲区大小
extern char nrf_receive_buf[NRF_RECEIVE_CHAR_NUM];
extern char nrf_send_buf[NRF_SEND_CHAR_NUM];
extern NRF_statusType nrf_running_status;
extern FlagStatus nrf_error_state;
extern float four_dirction_velocity_stright, four_dirction_velocity_turning, four_dirction_angular;
extern uint32_t last_packet_time;  // 记录最后一次收到包的时间戳
extern uint32_t last_request_time; // 记录最后请求的时间戳

// 遥控器OLED信息
extern OLED_Page_Tpye oled_page;
extern int track_data_page;
extern OLED_Curser_Tpye oled_curser;

// 遥控器OLED显示地图相关信息
#define METER_PIXEL 32 // 几个像素点代表1m (50)
#define SHOW_CENTER_X (64) // 地图显示原点X坐标 (+25)
#define SHOW_CENTER_Y (32) // 地图显示原点Y坐标 (+20)
#define OLED_MAX_WIDTH 128 // OLED宽
#define OLED_MAX_HIGHT 64  // OLED高
extern int map_show_level;

// 遥控器OLED显示的压缩地图缓存信息
#define COMPRESSED_SIZE 64                                          // 压缩后地图的尺寸，正好可以放入OLED
#define COMPRESSED_BYTE_NUM (COMPRESSED_SIZE * BIT_PER_POINT / 8)   // 压缩后地图每一行的字节数
extern uint8_t car_map_compressed[COMPRESSED_SIZE][COMPRESSED_BYTE_NUM];
extern FunctionalState car_map_compressed_complete;

// 遥控器按钮信息
#define KEY_LONG_CNT 10 // 长按的tick判别阈值
extern KeyType controler_key[8];
extern TrackKeyType controler_track_key;

// 遥控器LED和蜂鸣器信息
extern Led_Color controler_led_color;
extern BuzzerParamType controler_buzzer_param;

// 遥控器和电脑交互的串口信息
#define UART1_TX_SIZE 64 // 发送缓冲区大小
#define UART1_RX_SIZE 64 // 接收缓冲区大小
extern char uart1_send_buf[UART1_TX_SIZE];
extern char uart1_rxdata[UART1_RX_SIZE];
extern int last_rx_time;

// 遥控器MPU6050信息
extern Mpu6050Data controler_mpu6050_data;
extern Mpu6050DataProcessed controler_mpu6050_processed;

#endif
