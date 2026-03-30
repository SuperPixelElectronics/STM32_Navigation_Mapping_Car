#include "mpu6050.h"
#define MPU_TX_SIZE 50          // MPU6050发送缓冲区大小
#define MPU_RX_SIZE 56          // MPU6050接收缓冲区大小
char mpu_send_buf[MPU_TX_SIZE]; // MPU6050发送缓冲区
char mpu_rxdata[MPU_RX_SIZE];   // MPU6050接收缓冲区

uint8_t mpu_rxdat;              // 接收到的单个字符
volatile unsigned char mpu_rx_p;// 接收缓冲区的字符指针

typedef enum
{
    MPU_IDLE,
    MPU_WAITING
} MPU_State;
MPU_State mpu_step = MPU_IDLE;  // MPU6050状态
uint32_t request_time = 0;      // 上次请求MPU6050的时间戳

// private start----------------
static void MPU_UART_ChangeBaudRate(UART_HandleTypeDef *huart, uint32_t new_baudrate);
static HAL_StatusTypeDef mpu6050_change_baudrate(uint32_t new_baudrate);
static void mpu6050_reset_integrator(void);
static void mpu6050_set_first_angle(void);
static void mpu6050_init(void);
static void mpu6050_odometry_init(void);
static void process_mpu_data(void);
static HAL_StatusTypeDef mpu6050_read(void);
static void remove_gravity_in_real_time(void);
static void lowpass_filter_acceleration(void);
static void mpu6050_integrate(void);
static void zero_velocity_update(void);
static void mpu6050_remove_first_angle(void);

/**
 * @brief 更改和MPU6050通信的stm32串口的波特率
 * @param huart 具体哪个串口
 * @param new_baudrate 新的波特率
 */
static void MPU_UART_ChangeBaudRate(UART_HandleTypeDef *huart, uint32_t new_baudrate)
{
    // 1. 先停止串口
    HAL_UART_DeInit(huart);

    // 2. 修改波特率
    huart->Init.BaudRate = new_baudrate;

    // 3. 重新初始化
    HAL_UART_Init(huart);

    // 4. 如果需要，重新启动接收中断
    HAL_UART_Receive_IT(&huart2, &mpu_rxdat, 1);
}

/**
 * @brief 更改MPU6050本身的通信波特率，然后同步修改和MPU6050通信的stm32串口的波特率
 * @param new_baudrate 新的波特率
 * @return 新的波特率参数是否有效
 */
static HAL_StatusTypeDef mpu6050_change_baudrate(uint32_t new_baudrate)
{
    mpu_send_buf[0] = 0xFF, mpu_send_buf[1] = 0xAA, mpu_send_buf[2] = 0x69, mpu_send_buf[3] = 0x88, mpu_send_buf[4] = 0xB5;
    HAL_UART_Transmit(&huart2, (uint8_t *)mpu_send_buf, 5, MPU_TX_SIZE);
    HAL_Delay(200);
    if (new_baudrate == 4800)
    {
        mpu_send_buf[3] = 0x01;
    }
    else if (new_baudrate == 9600)
    {
        mpu_send_buf[3] = 0x02;
    }
    else if (new_baudrate == 19200)
    {
        mpu_send_buf[3] = 0x03;
    }
    else if (new_baudrate == 38400)
    {
        mpu_send_buf[3] = 0x04;
    }
    else if (new_baudrate == 57600)
    {
        mpu_send_buf[3] = 0x05;
    }
    else if (new_baudrate == 115200)
    {
        mpu_send_buf[3] = 0x06;
    }
    else if (new_baudrate == 230400)
    {
        mpu_send_buf[3] = 0x07;
    }
    else
    {
        return HAL_ERROR;
    }
    mpu_send_buf[0] = 0xFF, mpu_send_buf[1] = 0xAA, mpu_send_buf[2] = 0x04, mpu_send_buf[4] = 0x00;
    HAL_UART_Transmit(&huart2, (uint8_t *)mpu_send_buf, 5, MPU_TX_SIZE);
    MPU_UART_ChangeBaudRate(&huart2, new_baudrate);

    // 下面是给陀螺仪模块发送保存波特率参数的指令，但是实际不保存，每次都是在上电的时候现场设置
    // mpu_send_buf[0] = 0xFF,mpu_send_buf[1] = 0xAA, mpu_send_buf[2] = 0x69, mpu_send_buf[3] = 0x88, mpu_send_buf[4] = 0xB5;
    // HAL_UART_Transmit(&huart2, (uint8_t *)mpu_send_buf, 5, MPU_TX_SIZE);
    // HAL_Delay(200);
    // mpu_send_buf[0] = 0xFF, mpu_send_buf[1] = 0xAA, mpu_send_buf[2] = 0x00, mpu_send_buf[3] = 0x00, mpu_send_buf[4] = 0x00;
    // HAL_UART_Transmit(&huart2, (uint8_t *)mpu_send_buf, 5, MPU_TX_SIZE);

    return HAL_OK;
}

/**
 * @brief 重置MPU6050的积分器
 * @note 这里是演示尝试使用MPU6050的加速度进行二次积分来计算速度和路程的环节之一。但是实际上误差经过二次积分后会迅速增大，因此积分出来的速度和路程不可用
 */
static void mpu6050_reset_integrator()
{
    memset(&car_mpu6050_processed, 0, sizeof(car_mpu6050_processed));
    car_mpu6050_processed.is_first_sample = ENABLE;
}

/**
 * @brief 记录MPU6050刚上电的三轴角度
 */
static void mpu6050_set_first_angle()
{
    uint8_t retry_cnt = 0;
    HAL_StatusTypeDef ret;

    // 最多尝试 20 次，每次间隔一下，给传感器一点面子
    while (retry_cnt < 20)
    {
        ret = mpu6050_read();
        if (ret == HAL_OK)
        {
            // 成功拿到了数据，执行初始化
            // 刚上电时的瞬时值
            car_mpu6050_processed.first_angle.x = car_mpu6050_data.angle.x * PI / 180.0f;
            car_mpu6050_processed.first_angle.y = car_mpu6050_data.angle.y * PI / 180.0f;
            car_mpu6050_processed.first_angle.z = car_mpu6050_data.angle.z * PI / 180.0f;
            return; // 功成身退
        }

        // 如果是 HAL_BUSY 或 HAL_ERROR，稍微等一下再试
        HAL_Delay(10);
        retry_cnt++;
    }

    // 如果跑到这里，说明 20 次尝试全失败了

    // 强制给个默认值，防止后续除以 0 或出现 NaN
    car_mpu6050_processed.first_angle.x = 0;
    car_mpu6050_processed.first_angle.y = 0;
    car_mpu6050_processed.first_angle.z = 0;
}

/**
 * @brief 初始化MPU6050
 */
static void mpu6050_init()
{
    HAL_UART_Receive_IT(&huart2, &mpu_rxdat, 1);
    mpu6050_change_baudrate(115200);
}

static void mpu6050_odometry_init()
{
    mpu6050_reset_integrator();
    mpu6050_set_first_angle();
}

/**
 * @brief 处理MPU6050传来的原始数据
 */
static void process_mpu_data()
{
    // 从陀螺仪得到的原始信息
    car_mpu6050_data.acceleration.x = ((short)((mpu_rxdata[MPU6050_AXH] << 8) | mpu_rxdata[MPU6050_AXL]) * 16 * MPU6050_G) / 32768.0;
    car_mpu6050_data.acceleration.y = ((short)((mpu_rxdata[MPU6050_AYH] << 8) | mpu_rxdata[MPU6050_AYL]) * 16 * MPU6050_G) / 32768.0;
    car_mpu6050_data.acceleration.z = ((short)((mpu_rxdata[MPU6050_AZH] << 8) | mpu_rxdata[MPU6050_AZL]) * 16 * MPU6050_G) / 32768.0;
    car_mpu6050_data.temperature = ((mpu_rxdata[MPU6050_TH] << 8) | mpu_rxdata[MPU6050_TL]) / 100.0;
    car_mpu6050_data.angular_velocity.x = ((short)((mpu_rxdata[MPU6050_WXH] << 8) | mpu_rxdata[MPU6050_WXL]) * 2000) / 32768.0;
    car_mpu6050_data.angular_velocity.y = ((short)((mpu_rxdata[MPU6050_WYH] << 8) | mpu_rxdata[MPU6050_WYL]) * 2000) / 32768.0;
    car_mpu6050_data.angular_velocity.z = ((short)((mpu_rxdata[MPU6050_WZH] << 8) | mpu_rxdata[MPU6050_WZL]) * 2000) / 32768.0;
    car_mpu6050_data.angle.x = (((mpu_rxdata[MPU6050_ROLLH] << 8) | mpu_rxdata[MPU6050_ROLLL]) * 180) / 32768.0;
    car_mpu6050_data.angle.y = (((mpu_rxdata[MPU6050_PITCHH] << 8) | mpu_rxdata[MPU6050_PITCHL]) * 180) / 32768.0;
    car_mpu6050_data.angle.z = (((mpu_rxdata[MPU6050_YAWH] << 8) | mpu_rxdata[MPU6050_YAWL]) * 180) / 32768.0;
    car_mpu6050_data.magnet.x = ((mpu_rxdata[MPU6050_HXH] << 8) | mpu_rxdata[MPU6050_HXL]);
    car_mpu6050_data.magnet.y = ((mpu_rxdata[MPU6050_HYH] << 8) | mpu_rxdata[MPU6050_HYL]);
    car_mpu6050_data.magnet.z = ((mpu_rxdata[MPU6050_HZH] << 8) | mpu_rxdata[MPU6050_HZL]);
    car_mpu6050_data.time = HAL_GetTick();
}

/**
 * @brief 读取MPU6050
 * @note 为了保证MPU6050的积分结果可靠，我想要定时触发这个函数，这样子MPU6050读出数据的间隔就是固定的，累加的时候只需要固定的dt即可。
            1.如果把int mpu6050_read()放在定时器中断服务函数里，就无法进入中断服务函数了，我先后设置定时器和串口中断优先级为0，1和1，0，都没用
            2.放在主函数里，然后用中断里设置标志位去触发主函数的读取，函数会进入mpu6050_busy_error。猜测是因为MPU6050的执行任务执行时间 > 任务周期的情况，导致任务积压
                还会出现“mpu6050读取时间实际上仅5ms，但是因为在中断里给6050使能了，但是6050要到主函数才会执行，导致6050实际上不是马上执行的，下一次中断来的时候，6050可能根本没在执行”的异常情况
            3.最后的实现方案：把读取过程放回主函数，但是在读取的时候同时记录时间戳，积分的时候根据时间戳大小差值进行积分
    */
static HAL_StatusTypeDef mpu6050_read()
{
    switch (mpu_step)
    {
    case MPU_IDLE:
        // 发送指令
        mpu_send_buf[0] = 0xFF, mpu_send_buf[1] = 0xAA, mpu_send_buf[2] = 0x27, mpu_send_buf[3] = 0x00, mpu_send_buf[4] = 0x00;
        HAL_UART_Transmit(&huart2, (uint8_t *)mpu_send_buf, 5, MPU_TX_SIZE);

        mpu_rx_p = 0;
        request_time = HAL_GetTick();
        mpu_step = MPU_WAITING;
        return HAL_BUSY;

    case MPU_WAITING:
        // 这里不使用 while 和 Delay！
        // 如果数据够了就解析
        // 最早用while(mpu_rx_p < 55)发现一直卡在里面，后面发现关闭编译优化即可解决，猜测是因为编译优化使得mpu_rx_p被放入寄存器而不是内存，导致更新时无法被检测等
        if (mpu_rx_p >= 55)
        {
            process_mpu_data(); // 把你那些复杂的浮点运算搬到这里
            mpu_step = MPU_IDLE;
            return HAL_OK;
        }
        // 如果超时 9600 32ms ; 115200 5ms ; 230400 5ms
        else if (HAL_GetTick() - request_time > 100)
        {
            mpu_rx_p = 0;
            mpu_step = MPU_IDLE; // 超时重置
            return HAL_TIMEOUT;
        }
        return HAL_BUSY;
    }
    return HAL_ERROR;
}

/**
 * @brief 使用MPU6050输出的角度实时移除重力
 * @note 这里是演示尝试使用MPU6050的加速度进行二次积分来计算速度和路程的环节之一。但是实际上误差经过二次积分后会迅速增大，因此积分出来的速度和路程不可用
 */
static void remove_gravity_in_real_time()
{

    // 从MPU6050获取角度（单位：弧度）
    float roll = car_mpu6050_data.angle.x * PI / 180.0f;  // 横滚角
    float pitch = car_mpu6050_data.angle.y * PI / 180.0f; // 俯仰角

    // 计算重力在各轴的分量
    float gx = -MPU6050_G * sinf(pitch);
    float gy = MPU6050_G * sinf(roll) * cosf(pitch);
    float gz = MPU6050_G * cosf(roll) * cosf(pitch);

    // 移除重力分量后的信息
    car_mpu6050_processed.acceleration_no_gravity.x = -(car_mpu6050_data.acceleration.x - gx);
    car_mpu6050_processed.acceleration_no_gravity.y = -(car_mpu6050_data.acceleration.y - gy);
    car_mpu6050_processed.acceleration_no_gravity.z = -(car_mpu6050_data.acceleration.z - gz);
}

/**
 * @brief 对MPU6050的加速度进行简单的一阶低通滤波
 * @note 这里是演示尝试使用MPU6050的加速度进行二次积分来计算速度和路程的环节之一。但是实际上误差经过二次积分后会迅速增大，因此积分出来的速度和路程不可用
 */
static void lowpass_filter_acceleration(void)
{
    static float last_accel_x = 0, last_accel_y = 0, last_accel_z = 0;
    const float alpha = 0.2f; // 滤波系数，越小越平滑

    car_mpu6050_processed.acceleration_no_gravity.x =
        alpha * car_mpu6050_processed.acceleration_no_gravity.x +
        (1 - alpha) * last_accel_x;

    car_mpu6050_processed.acceleration_no_gravity.y =
        alpha * car_mpu6050_processed.acceleration_no_gravity.y +
        (1 - alpha) * last_accel_y;

    car_mpu6050_processed.acceleration_no_gravity.z =
        alpha * car_mpu6050_processed.acceleration_no_gravity.z +
        (1 - alpha) * last_accel_z;

    last_accel_x = car_mpu6050_processed.acceleration_no_gravity.x;
    last_accel_y = car_mpu6050_processed.acceleration_no_gravity.y;
    last_accel_z = car_mpu6050_processed.acceleration_no_gravity.z;
}

/**
 * @brief 使用MPU6050输出的加速度进行积分得到速度和路程
 * @note 这里是演示尝试使用MPU6050的加速度进行二次积分来计算速度和路程的环节之一。但是实际上误差经过二次积分后会迅速增大，因此积分出来的速度和路程不可用
 */
static void mpu6050_integrate()
{
    uint32_t current_time = car_mpu6050_data.time;
    car_mpu6050_processed.time = car_mpu6050_data.time;
    if (car_mpu6050_processed.is_first_sample)
    {
        // 第一个样本，只记录不积分
        car_mpu6050_processed.last_accel.x = car_mpu6050_processed.acceleration_no_gravity.x;
        car_mpu6050_processed.last_accel.y = car_mpu6050_processed.acceleration_no_gravity.y;
        car_mpu6050_processed.last_accel.z = car_mpu6050_processed.acceleration_no_gravity.z;
        car_mpu6050_processed.last_time = current_time;
        car_mpu6050_processed.is_first_sample = DISABLE;
        return;
    }

    // 计算时间间隔（秒）
    float dt = (current_time - car_mpu6050_processed.last_time) / 1000.0f;

    if (dt <= 0 || dt > 1.0f)
    { // 无效时间间隔
        car_mpu6050_processed.last_time = current_time;
        return;
    }

    // 使用梯形法则积分（更准确）
    float avg_accel_x = (car_mpu6050_processed.last_accel.x + car_mpu6050_processed.acceleration_no_gravity.x) / 2.0f;
    float avg_accel_y = (car_mpu6050_processed.last_accel.y + car_mpu6050_processed.acceleration_no_gravity.y) / 2.0f;
    float avg_accel_z = (car_mpu6050_processed.last_accel.z + car_mpu6050_processed.acceleration_no_gravity.z) / 2.0f;

    // 第一次积分：加速度 → 速度
    car_mpu6050_processed.vel.x += avg_accel_x * dt;
    car_mpu6050_processed.vel.y += avg_accel_y * dt;
    car_mpu6050_processed.vel.z += avg_accel_z * dt;

    // 第二次积分：速度 → 位置
    car_mpu6050_processed.pos.x += car_mpu6050_processed.vel.x * dt;
    car_mpu6050_processed.pos.y += car_mpu6050_processed.vel.y * dt;
    car_mpu6050_processed.pos.z += car_mpu6050_processed.vel.z * dt;

    // 保存当前值
    car_mpu6050_processed.last_accel.x = car_mpu6050_processed.acceleration_no_gravity.x;
    car_mpu6050_processed.last_accel.y = car_mpu6050_processed.acceleration_no_gravity.y;
    car_mpu6050_processed.last_accel.z = car_mpu6050_processed.acceleration_no_gravity.z;
    car_mpu6050_processed.last_time = current_time;
}

/**
 * @brief 在无运动和无旋转的时候衰减MPU6050算出的速度
 * @note 这里是演示尝试使用MPU6050的加速度进行二次积分来计算速度和路程的环节之一。但是实际上误差经过二次积分后会迅速增大，因此积分出来的速度和路程不可用
 */
static void zero_velocity_update(void)
{
    // 检测是否静止
    float accel_mag = sqrtf(car_mpu6050_processed.acceleration_no_gravity.x *
                                car_mpu6050_processed.acceleration_no_gravity.x +
                            car_mpu6050_processed.acceleration_no_gravity.y *
                                car_mpu6050_processed.acceleration_no_gravity.y +
                            car_mpu6050_processed.acceleration_no_gravity.z *
                                car_mpu6050_processed.acceleration_no_gravity.z);

    float gyro_mag = sqrtf(car_mpu6050_data.angular_velocity.x *
                               car_mpu6050_data.angular_velocity.x +
                           car_mpu6050_data.angular_velocity.y *
                               car_mpu6050_data.angular_velocity.y +
                           car_mpu6050_data.angular_velocity.z *
                               car_mpu6050_data.angular_velocity.z);

    // 静止条件：加速度很小（无运动），角速度很小（无旋转）
    if (accel_mag < 0.2f && gyro_mag < 0.5f)
    {
        // 缓慢衰减速度，而不是直接置零
        const float decay = 0.1f; // 衰减系数
        car_mpu6050_processed.vel.x *= (1.0f - decay);
        car_mpu6050_processed.vel.y *= (1.0f - decay);
        car_mpu6050_processed.vel.z *= (1.0f - decay);

        // 可选：也衰减位置漂移
        // car_mpu6050_processed.pos.x *= 0.999f;
        // car_mpu6050_processed.pos.y *= 0.999f;
        // car_mpu6050_processed.pos.z *= 0.999f;
    }
}

/**
 * @brief 将MPU6050三轴角度减去刚上电的角度，得到相对开机时候的角度变化
 */
static void mpu6050_remove_first_angle()
{
    // 去除刚上电时测得的信息，此时获得的为上电后的变化值
    car_mpu6050_processed.angle_remove_first.x = (car_mpu6050_data.angle.x * PI / 180.0f) - car_mpu6050_processed.first_angle.x;
    car_mpu6050_processed.angle_remove_first.y = (car_mpu6050_data.angle.y * PI / 180.0f) - car_mpu6050_processed.first_angle.y;
    car_mpu6050_processed.angle_remove_first.z = (car_mpu6050_data.angle.z * PI / 180.0f) - car_mpu6050_processed.first_angle.z + PI / 2;
    if (car_mpu6050_processed.angle_remove_first.z < 0)
    {
        car_mpu6050_processed.angle_remove_first.z += 2 * PI;
    }
    else if (car_mpu6050_processed.angle_remove_first.z > 2 * PI)
    {
        car_mpu6050_processed.angle_remove_first.z -= 2 * PI;
    }
}

// private end----------------

/**
 * @brief 接收MPU6050单个字符的串口中断函数
 */
void MPU_UART_Receive()
{
    mpu_rxdata[mpu_rx_p] = mpu_rxdat;
    HAL_UART_Receive_IT(&huart2, &mpu_rxdat, 1);
    mpu_rx_p++;
}

/**
 * @brief 初始化MPU6050外设和相关算法
 */
void mpu6050_init_all(){
    mpu6050_init();
    mpu6050_odometry_init();
}

/**
 * @brief 读取MPU6050并运行相关算法
 */
void mpu6050_process(){
    mpu6050_read();
    remove_gravity_in_real_time();
    lowpass_filter_acceleration();
    mpu6050_integrate();
    zero_velocity_update();
    mpu6050_remove_first_angle();
}
