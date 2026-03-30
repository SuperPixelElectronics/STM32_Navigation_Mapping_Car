#include "system.h"

// 传感器相关
float Encoder_precision = 42 * 7;              // 编码器精度(轮子转一圈后编码器的脉冲数)=减速比*电机转一圈后编码器的脉冲数
float Wheel_perimeter = PI * 34 * 0.001;       // 轮子周长，单位：m
float Wheel_spacing = 56 * 0.001;              // 主动轮轮距，单位：m
float k_motor_speed = 0.2, b_motor_speed = 0;  // 轮子编码器数值映射到车子线速度的微调系数
float k_motor_delta = 0.36, b_motor_delta = 0; // 轮子编码器数值映射到车子角速度的微调系数
EncoderProcessed car_encoder_processed;        // 电机编码器处理后的信息
Mpu6050Data car_mpu6050_data;                  // MPU6050+磁力计读取到的原始信息
Mpu6050DataProcessed car_mpu6050_processed;    // 编码器和MPU6050+磁力计融合后的小车里程计数据

float position_angle_combined_k = 1.0;      // “电机编码器”和“MPU6050+磁力计”的航向角融合系数，范围为0-1，越大代表越相信“MPU6050+磁力计”信息
CombinedProcessed car_combined_processed;   // 编码器和MPU6050+磁力计融合后的小车里程计数据

float vl53l1_distance;                      // vl53l1检测到的距离信息，单位：m
ObstacleType obstacle_info;                 // 激光传感器检测到的障碍物相关信息

// 地图信息
FunctionalState map_detect_switch = DISABLE; //地图绘制使能开关
uint8_t car_map[CAR_MAP_SIZE][(CAR_MAP_SIZE * BIT_PER_POINT) / (sizeof(uint8_t) * 8)] = {0}; // 小车的全局地图

// 路径规划求解器信息
PointType car_goal_position;             // 小车的目的地
TrackPointType solves[MAX_SOLVE_NODES];  // 存储找到的骨架点
SolveErrorType path_solve_status = IDLE; // 求解器状态
int solves_count = 0;                    // 解出来的路径点的数量

// 轨迹追踪器信息
TrajectoryTracker tracker;                // 轨迹追踪器结构
PointType car_track[TRACK_LEN_MAX] = {0}; // 小车的追踪点
uint16_t current_track_length = 1;        // 当前轨迹点长度

// 执行器信息
NRFControlModeType nrf_control_mode = CAR_CONTROL;             // 控制所有权信息
Control_Mode control_mode;                                     // 小车控制模式
VectorType car_set_velocity;                                   // 小车设定速度
float four_dirction_velocity = 0.8, four_dirction_angular = 6; //使用车子上面摇杆直接控制车子的参数
Motor_parameter MOTOR_A, MOTOR_B;                              // 电机的参数结构体

// NRF信息
char nrf_receive_buf[NRF_RECEIVE_CHAR_NUM]; // NRF接收缓冲区
char nrf_send_buf[NRF_SEND_CHAR_NUM];       // NRF发送缓冲区
FlagStatus nrf_error_state = RESET;         // NRF硬件错误状态

// OLED信息
OLED_Page_Tpye oled_page;                   // OLED显示页面索引
int track_data_page;                        // 轨迹追踪数据显示的子页面索引
int solve_data_page;                        // 路径求解数据显示的子页面索引
char oled_refresh_gap = 3;                  // OLED刷新间隔(主函数每过x轮，oled刷新一次)
OLED_Curser_Tpye oled_curser;               // OLED光标信息
FunctionalState display_max_time = DISABLE; // 系统相关数据页面中显示的时间是否是显示最大时间(否则为实时运行时间)

// OLED显示地图相关信息
int map_show_level = 1;                      // 显示的是哪个层级的地图 0:未探测, 1:空闲, 2:占据, 3:静态障碍
PointType map_show_area, map_show_area_max;  //在1:1渲染页面下显示的区块索引和其最大值

// OLED显示的压缩地图缓存信息
uint8_t car_map_compressed[COMPRESSED_SIZE][COMPRESSED_BYTE_NUM];   // 压缩后的地图
FunctionalState car_map_compressed_complete = DISABLE;              // 是否压缩完成

// 按钮信息
int key_adc_val;    // 按键ADC数值
KeyType car_key[5]; // 按键结构体

// LED和蜂鸣器信息
Led_Color car_led_color = BREATH;   // LED灯状态
BuzzerParamType car_buzzer_param;   // 蜂鸣器信息

// 和电脑交互的串口信息
char uart1_send_buf[UART1_TX_SIZE]; // 发送缓冲区
char uart1_rxdata[UART1_RX_SIZE];   // 接收缓冲区
int last_rx_time;                   // 上一次接收到数据的时间戳

// main函数运行时间分析信息，主要用来分析代码性能
int main_start_tick, main_all_tick, main_all_tick_max;  //主函数计时相关信息
int main_during_tick[10];       // 各个函数运行实时时间
int main_during_tick_max[10];   // 各个函数运行的最大时间
