#include "encoder.h"

#define CONTROL_FREQUENCY 100
// Encoder structure
// 编码器结构体
typedef struct
{
    int A;
    int B;
} Encoder;
Encoder OriginalEncoder; // Encoder raw data //编码器原始数据

// private start------------------
static void Encoder_init(void);
static void Encoder_Odometry_Init(float init_x, float init_y, float init_theta);
static uint32_t Read_Encoder(int TIMX);
static void Get_Velocity_Form_Encoder(void);
static void Encoder_Get_Car_Speed(void);
static void Encoder_Odometry_Update(void);

/**************************************************************************
函数功能：初始化编码器
入口参数：无
返回  值：无
**************************************************************************/
static void Encoder_init()
{
    HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);
    HAL_TIM_Encoder_Start(&htim4, TIM_CHANNEL_ALL);
    HAL_TIM_Base_Start_IT(&htim1);
}

/**************************************************************************
函数功能：初始化位置
入口参数：无
返回  值：无
**************************************************************************/
static void Encoder_Odometry_Init(float init_x, float init_y, float init_theta)
{
    car_encoder_processed.Encoder_Position.x = init_x;
    car_encoder_processed.Encoder_Position.y = init_y;
    car_encoder_processed.Encoder_Position.theta = init_theta;
}

/**************************************************************************
Function: Read the encoder count
Input   : The timer
Output  : Encoder value (representing speed)
函数功能：读取编码器计数
入口参数：定时器
返回  值：编码器数值(代表速度)
**************************************************************************/
static uint32_t Read_Encoder(int TIMX)
{
    uint32_t Encoder_TIM;

    switch (TIMX)
    {
    case 3:
    {
        Encoder_TIM = __HAL_TIM_GetCounter(&htim3); // 获取计数值
        __HAL_TIM_SetCounter(&htim3, 0);            // 清空计数值
        break;
    }

    case 4:
    {
        Encoder_TIM = __HAL_TIM_GetCounter(&htim4); // 获取计数值
        __HAL_TIM_SetCounter(&htim4, 0);            // 清空计数值
        break;
    }

    default:
        Encoder_TIM = 0;
    }
    return Encoder_TIM;
}

/**************************************************************************
Function: Read the encoder value and calculate the wheel speed, unit m/s
Input   : none
Output  : none
函数功能：读取编码器数值并计算车轮速度，单位m/s
入口参数：无
返回  值：无
**************************************************************************/
static void Get_Velocity_Form_Encoder(void)
{
    // Retrieves the original data of the encoder
    // 获取编码器的原始数据
    float Encoder_A_pr, Encoder_B_pr;
    OriginalEncoder.A = Read_Encoder(3);
    OriginalEncoder.B = Read_Encoder(4);

    if (OriginalEncoder.A > 0x8000)
    {
        Encoder_A_pr = -(0xffff - OriginalEncoder.A);
    }
    else
    {
        Encoder_A_pr = OriginalEncoder.A; // 编码器的实际数据
    }

    if (OriginalEncoder.B > 0x8000)
    {
        Encoder_B_pr = -(0xffff - OriginalEncoder.B);
    }
    else
    {
        Encoder_B_pr = OriginalEncoder.B; // 编码器的实际数据
    }

    // The encoder converts the raw data to wheel speed in m/s
    // 编码器原始数据转换为车轮速度，单位m/s
    MOTOR_A.Encoder = (Encoder_A_pr * CONTROL_FREQUENCY * Wheel_perimeter / Encoder_precision);
    MOTOR_B.Encoder = (Encoder_B_pr * CONTROL_FREQUENCY * Wheel_perimeter / Encoder_precision);
}

/**************************************************************************
函数功能：运动学正解
入口参数：无
返回  值：无
**************************************************************************/
static void Encoder_Get_Car_Speed(void)
{
    // 车子的线速度和角速度
    car_encoder_processed.Encoder_Velocity.motor_speed = ((MOTOR_A.Encoder + MOTOR_B.Encoder) / 2) * k_motor_speed + b_motor_speed;
    car_encoder_processed.Encoder_Velocity.motor_delta = ((MOTOR_B.Encoder - MOTOR_A.Encoder) / Wheel_spacing) * k_motor_delta + b_motor_delta;
    car_encoder_processed.Encoder_Velocity.time = HAL_GetTick();
}

/**************************************************************************
函数功能：仅使用编码器的里程计的更新函数
入口参数：无
返回  值：无
**************************************************************************/
static void Encoder_Odometry_Update(void)
{
    uint32_t current_time = car_encoder_processed.Encoder_Velocity.time;
    float dt = (current_time - car_encoder_processed.Encoder_Velocity.last_time) / 1000.0f;

    // 获取传感器数据
    float motor_speed = car_encoder_processed.Encoder_Velocity.motor_speed; // 前向速度
    float motor_delta = car_encoder_processed.Encoder_Velocity.motor_delta; // 角速度

    // 计算角度变化
    float delta_theta = motor_delta * dt;

    float delta_x, delta_y;

    if (fabsf(delta_theta) < 0.0001f)
    {
        // 当角度变化很小时，使用近似公式
        // 在数学坐标系中，x方向对应cos，y方向对应sin
        delta_x = motor_speed * dt * cosf(car_encoder_processed.Encoder_Position.theta);
        delta_y = motor_speed * dt * sinf(car_encoder_processed.Encoder_Position.theta);
    }
    else
    {
        // 精确公式，考虑圆弧运动
        // 注意：当速度接近0时，需要处理除零问题
        if (fabsf(motor_speed) < 0.001f)
        {
            delta_x = 0;
            delta_y = 0;
        }
        else
        {
            float radius = motor_speed / motor_delta; // 转弯半径
            delta_x = radius * (sinf(car_encoder_processed.Encoder_Position.theta + delta_theta) - sinf(car_encoder_processed.Encoder_Position.theta));
            delta_y = -radius * (cosf(car_encoder_processed.Encoder_Position.theta + delta_theta) - cosf(car_encoder_processed.Encoder_Position.theta));
        }
    }

    // 更新位置 只依靠编码器获得的里程计信息
    car_encoder_processed.Encoder_Position.x += delta_x;
    car_encoder_processed.Encoder_Position.y += delta_y;
    car_encoder_processed.Encoder_Position.theta += delta_theta;

    // 角度规范化到 [-π, π]
    while (car_encoder_processed.Encoder_Position.theta > PI)
    {
        car_encoder_processed.Encoder_Position.theta -= 2 * PI;
    }
    while (car_encoder_processed.Encoder_Position.theta < -PI)
    {
        car_encoder_processed.Encoder_Position.theta += 2 * PI;
    }

    car_encoder_processed.Encoder_Velocity.last_time = current_time;
}

// private end------------------

/**************************************************************************
函数功能：初始化编码器本身和其相关处理函数
入口参数：无
返回  值：无
**************************************************************************/
void Encoder_init_all(){
    Encoder_init();
    Encoder_Odometry_Init(0, 0, PI / 2); // 初始化在(0,0)，朝向y轴正方向
}

/**************************************************************************
函数功能：编码器读取和相关处理函数
入口参数：无
返回  值：无
**************************************************************************/
void Encoder_process_all(){
    Get_Velocity_Form_Encoder();
    Encoder_Get_Car_Speed();
    Encoder_Odometry_Update();
}
