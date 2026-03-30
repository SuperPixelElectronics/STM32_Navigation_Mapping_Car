#include "system.h"

// 标R和W代表需要通过nrf交互的信息
// R：小车对遥控器可读的数值，是小车回传给遥控器的数值
// W：小车对遥控器可写，是遥控器自己设置后传给小车的数值

// 小车位置信息
CombinedProcessed car_combined_processed; // R

// 小车地图信息
FunctionalState map_detect_switch_feedback = DISABLE;                                        // R 地图绘制使能开关
FunctionalState map_detect_switch_set = DISABLE;                                             // W 地图绘制使能开关
uint8_t car_map[CAR_MAP_SIZE][(CAR_MAP_SIZE * BIT_PER_POINT) / (sizeof(uint8_t) * 8)] = {0}; // R 小车的全局地图

// 小车目标位置信息
PointType car_goal_position_feedback; // R
PointType car_goal_position_set;      // W

// 小车轨迹追踪器信息
TrajectoryTracker tracker;                // RW 轨迹追踪器结构
PointType car_track[TRACK_LEN_MAX] = {0}; // R  小车的追踪点
uint16_t current_track_length = 1;        // R  当前轨迹点长度

// 小车执行器信息
VectorType car_set_velocity;                        // R 小车设定速度
VectorType car_feedback_velocity;                   // W 小车设定速度
NRFControlModeType nrf_control_mode = CAR_CONTROL;  //控制所有权信息

// 遥控器NRF信息
char nrf_receive_buf[NRF_RECEIVE_CHAR_NUM]; // NRF接收缓冲区
char nrf_send_buf[NRF_SEND_CHAR_NUM];       // NRF发送缓冲区
NRF_statusType nrf_running_status;          // NRF当前状态
FlagStatus nrf_error_state = RESET;         // NRF硬件错误状态
float four_dirction_velocity_stright = 0.9, four_dirction_velocity_turning = 0.6, four_dirction_angular = 13.0; //摇杆控制小车时的相关参数
uint32_t last_packet_time = 0;  // 记录最后一次收到包的时间戳
uint32_t last_request_time = 0; // 记录最后请求的时间戳

// 遥控器OLED信息
OLED_Page_Tpye oled_page;       // OLED显示页面索引
int track_data_page;            // 轨迹追踪数据显示的子页面索引
OLED_Curser_Tpye oled_curser;   // OLED光标信息

// 遥控器OLED显示地图相关信息
int map_show_level = 1; // 显示的是哪个层级的地图 0:未探测, 1:空闲, 2:占据, 3:静态障碍

// 遥控器OLED显示的压缩地图缓存信息
uint8_t car_map_compressed[COMPRESSED_SIZE][COMPRESSED_BYTE_NUM]; // 压缩后的地图
FunctionalState car_map_compressed_complete;                      // 是否压缩完成

// 遥控器按钮信息
KeyType controler_key[8];           // 按键结构体
TrackKeyType controler_track_key;   // 黑莓轨迹球结构体

// 遥控器LED和蜂鸣器信息
Led_Color controler_led_color = BREATH; // LED灯状态
BuzzerParamType controler_buzzer_param; // 蜂鸣器信息

// 遥控器和电脑交互的串口信息
char uart1_send_buf[UART1_TX_SIZE]; // 发送缓冲区
char uart1_rxdata[UART1_RX_SIZE];   // 接收缓冲区
int last_rx_time;                   // 上一次接收到数据的时间戳

// 遥控器MPU6050信息
Mpu6050Data controler_mpu6050_data;               // MPU6050+磁力计读取到的原始信息
Mpu6050DataProcessed controler_mpu6050_processed; // MPU6050+磁力计处理后的信息
