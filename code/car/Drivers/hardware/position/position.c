#include "position.h"

// 整个MPU6050模块的状态
typedef enum
{
    MPU6050_DISABLE = 0U,
    MPU6050_ENABLE,
    MPU6050_BUSY
} Mpu6050StateType;
Mpu6050StateType mpu6050_state = MPU6050_DISABLE;

// private start------------------
/**
 * @brief 融合里程计的初始化函数
 */
static void Combined_Init(float init_x, float init_y, float init_theta)
{
    car_combined_processed.CombinedPosition.x = init_x;
    car_combined_processed.CombinedPosition.y = init_y;
    car_combined_processed.CombinedPosition.theta = init_theta;
}

/**
 * @brief 融合里程计的更新函数
 */
static void Combined_Odometry_Update(void)
{
    uint32_t current_time = car_combined_processed.Encoder_Velocity.time;
    float dt = (current_time - car_combined_processed.Encoder_Velocity.last_time) / 1000.0f;

    // 获取传感器数据
    float combined_speed = car_combined_processed.Encoder_Velocity.motor_speed;    // 前向速度
    float combined_delta = car_combined_processed.Mpu6050_Data.angular_velocity.z; // 角速度
    // float combined_delta = car_combined_processed.Encoder_Velocity.motor_delta; // 角速度

    // 计算角度变化
    float delta_theta = combined_delta * dt;

    float delta_x, delta_y;

    // if (fabsf(delta_theta) < 0.0001f)
    // {
    // 当角度变化很小时，使用近似公式
    // 在数学坐标系中，x方向对应cos，y方向对应sin
    delta_x = combined_speed * dt * cosf(car_combined_processed.CombinedPosition.theta);
    delta_y = combined_speed * dt * sinf(car_combined_processed.CombinedPosition.theta);
    // }
    // else
    // {
    //     // 精确公式，考虑圆弧运动
    //     // 注意：当速度接近0时，需要处理除零问题
    //     if (fabsf(combined_speed) < 0.001f)
    //     {
    //         delta_x = 0;
    //         delta_y = 0;
    //     }
    //     else
    //     {
    //         float radius = combined_speed / combined_delta; // 转弯半径
    //         delta_x = radius * (sinf(car_combined_processed.CombinedPosition.theta + delta_theta) - sinf(car_combined_processed.CombinedPosition.theta));
    //         delta_y = -radius * (cosf(car_combined_processed.CombinedPosition.theta + delta_theta) - cosf(car_combined_processed.CombinedPosition.theta));
    //     }
    // }

    // 更新位置
    // 融合后的位置信息
    car_combined_processed.CombinedPosition.x += delta_x;
    car_combined_processed.CombinedPosition.y += delta_y;
    car_combined_processed.Encoder_Velocity.last_time = current_time;
}

// private end------------------

/**
 * @brief 所有里程计的初始化函数
 */
void position_init()
{
    Encoder_init_all();
    mpu6050_init_all();
    Combined_Init(0, 0, PI / 2);         // 初始化在(0,0)，朝向y轴正方向
}

/**
 * @brief 所有里程计的定时更新函数
 * @note 需要每10ms执行一次
 */
void position_10ms_process()
{
    Encoder_process_all();

    if (mpu6050_state == MPU6050_DISABLE)
    {
        mpu6050_state = MPU6050_ENABLE;
    }
}

/**
 * @brief 所有里程计的主更新函数
 * @note
 * 查询小车相关比赛等视频，得到的定位方法如下：
 * 编码器       我正在使用
 * IMU、磁力计  我正在使用，但是加速度积分出来的速度和路程误差较大，只能使用航向角等信息
 * 激光雷达     需要Linux等复杂soc板子才可以提供解析算力
 * 光流法       设计小车的时候没想到，并且我自己不会
 * obs9        小车放不下超小麦轮
 * uwb          需要在场地中布置天线，太复杂了！！！
 * 最后使用方法：
 * 线速度：使用编码器
 * 航向角：使用编码器+IMU和磁力计通过一定比例融合，正常情况下完全使用IMU和磁力计的航向角
 */
void position_main_process()
{
    if (mpu6050_state == MPU6050_ENABLE)
    {
        mpu6050_state = MPU6050_BUSY;
        mpu6050_process();
        mpu6050_state = MPU6050_DISABLE;
    }
    
    // 不能使用" car_combined_processed.Encoder_Velocity = car_encoder_processed.Encoder_Velocity;"的方法
    // 因为这两个结构体是匿名结构体，编译器认为它们是不同的类型
    car_combined_processed.Encoder_Velocity.motor_delta = car_encoder_processed.Encoder_Velocity.motor_delta;
    car_combined_processed.Encoder_Velocity.motor_speed = car_encoder_processed.Encoder_Velocity.motor_speed;
    car_combined_processed.Encoder_Velocity.time = car_encoder_processed.Encoder_Velocity.time;
    car_combined_processed.Encoder_Position.theta = car_encoder_processed.Encoder_Position.theta;
    car_combined_processed.Mpu6050_Data.angular_velocity.x = car_mpu6050_data.angular_velocity.x;
    car_combined_processed.Mpu6050_Data.angular_velocity.y = car_mpu6050_data.angular_velocity.y;
    car_combined_processed.Mpu6050_Data.angular_velocity.z = car_mpu6050_data.angular_velocity.z;

    car_combined_processed.Mpu6050DataProcessed.angle_remove_first.x = car_mpu6050_processed.angle_remove_first.x;
    car_combined_processed.Mpu6050DataProcessed.angle_remove_first.y = car_mpu6050_processed.angle_remove_first.y;
    car_combined_processed.Mpu6050DataProcessed.angle_remove_first.z = car_mpu6050_processed.angle_remove_first.z;
    car_combined_processed.Mpu6050DataProcessed.time = car_mpu6050_processed.time;

    // 融合后的角度
    // 如果融合系数不是1，小车会参考电机编码器来计算航向角，导致用手拿车子转圈圈时，算出的航向角不准确
    // 会出现“当航向角转一圈即将回到起点时，显示在oled上面的线会出现15度左右的跳变”
    car_combined_processed.CombinedPosition.theta = position_angle_combined_k * car_combined_processed.Mpu6050DataProcessed.angle_remove_first.z + (1 - position_angle_combined_k) * car_combined_processed.Encoder_Position.theta;
    Combined_Odometry_Update();
}
