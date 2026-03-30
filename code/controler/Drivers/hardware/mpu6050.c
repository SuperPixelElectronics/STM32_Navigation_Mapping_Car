#include "mpu6050.h"
#define MPU_TX_SIZE 50          // MPU6050发送缓冲区大小
#define MPU_RX_SIZE 56          // MPU6050接收缓冲区大小
char mpu_send_buf[MPU_TX_SIZE]; // MPU6050发送缓冲区
char mpu_rxdata[MPU_RX_SIZE];   // MPU6050接收缓冲区

uint8_t mpu_rxdat;              // 接收到的单个字符
volatile unsigned char mpu_rx_p; // 接收缓冲区的字符指针

typedef enum
{
    MPU_IDLE,
    MPU_WAITING
} MPU_State;
MPU_State mpu_step = MPU_IDLE; // MPU6050状态
uint32_t request_time = 0;     // 上次请求MPU6050的时间戳

// private start------------------
static void MPU_UART_ChangeBaudRate(UART_HandleTypeDef *huart, uint32_t new_baudrate);
static HAL_StatusTypeDef mpu6050_change_baudrate(uint32_t new_baudrate);
static void mpu6050_set_first_angle(void);
static void process_mpu_data(void);
static HAL_StatusTypeDef mpu6050_read(void);
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
            controler_mpu6050_processed.first_angle.x = controler_mpu6050_data.angle.x * PI / 180.0f;
            controler_mpu6050_processed.first_angle.y = controler_mpu6050_data.angle.y * PI / 180.0f;
            controler_mpu6050_processed.first_angle.z = controler_mpu6050_data.angle.z * PI / 180.0f;
            return; // 功成身退
        }

        // 如果是 HAL_BUSY 或 HAL_ERROR，稍微等一下再试
        HAL_Delay(10);
        retry_cnt++;
    }

    // 如果跑到这里，说明 20 次尝试全失败了

    // 强制给个默认值，防止后续除以 0 或出现 NaN
    controler_mpu6050_processed.first_angle.x = 0;
    controler_mpu6050_processed.first_angle.y = 0;
    controler_mpu6050_processed.first_angle.z = 0;
}

/**
 * @brief 处理MPU6050传来的原始数据
 */
static void process_mpu_data()
{
    controler_mpu6050_data.acceleration.x = ((short)((mpu_rxdata[MPU6050_AXH] << 8) | mpu_rxdata[MPU6050_AXL]) * 16 * MPU6050_G) / 32768.0;
    controler_mpu6050_data.acceleration.y = ((short)((mpu_rxdata[MPU6050_AYH] << 8) | mpu_rxdata[MPU6050_AYL]) * 16 * MPU6050_G) / 32768.0;
    controler_mpu6050_data.acceleration.z = ((short)((mpu_rxdata[MPU6050_AZH] << 8) | mpu_rxdata[MPU6050_AZL]) * 16 * MPU6050_G) / 32768.0;
    controler_mpu6050_data.temperature = ((mpu_rxdata[MPU6050_TH] << 8) | mpu_rxdata[MPU6050_TL]) / 100.0;
    controler_mpu6050_data.angular_velocity.x = ((short)((mpu_rxdata[MPU6050_WXH] << 8) | mpu_rxdata[MPU6050_WXL]) * 2000) / 32768.0;
    controler_mpu6050_data.angular_velocity.y = ((short)((mpu_rxdata[MPU6050_WYH] << 8) | mpu_rxdata[MPU6050_WYL]) * 2000) / 32768.0;
    controler_mpu6050_data.angular_velocity.z = ((short)((mpu_rxdata[MPU6050_WZH] << 8) | mpu_rxdata[MPU6050_WZL]) * 2000) / 32768.0;
    controler_mpu6050_data.angle.x = (((mpu_rxdata[MPU6050_ROLLH] << 8) | mpu_rxdata[MPU6050_ROLLL]) * 180) / 32768.0;
    controler_mpu6050_data.angle.y = (((mpu_rxdata[MPU6050_PITCHH] << 8) | mpu_rxdata[MPU6050_PITCHL]) * 180) / 32768.0;
    controler_mpu6050_data.angle.z = (((mpu_rxdata[MPU6050_YAWH] << 8) | mpu_rxdata[MPU6050_YAWL]) * 180) / 32768.0;
    controler_mpu6050_data.magnet.x = ((mpu_rxdata[MPU6050_HXH] << 8) | mpu_rxdata[MPU6050_HXL]);
    controler_mpu6050_data.magnet.y = ((mpu_rxdata[MPU6050_HYH] << 8) | mpu_rxdata[MPU6050_HYL]);
    controler_mpu6050_data.magnet.z = ((mpu_rxdata[MPU6050_HZH] << 8) | mpu_rxdata[MPU6050_HZL]);
    controler_mpu6050_data.time = HAL_GetTick();
}

/**
 * @brief 读取MPU6050
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
 * @brief 将MPU6050三轴角度减去刚上电的角度，得到相对开机时候的角度变化
 */
static void mpu6050_remove_first_angle()
{
    controler_mpu6050_processed.angle_remove_first.x = (controler_mpu6050_data.angle.x * PI / 180.0f) - controler_mpu6050_processed.first_angle.x;
    controler_mpu6050_processed.angle_remove_first.y = (controler_mpu6050_data.angle.y * PI / 180.0f) - controler_mpu6050_processed.first_angle.y;
    controler_mpu6050_processed.angle_remove_first.z = (controler_mpu6050_data.angle.z * PI / 180.0f) - controler_mpu6050_processed.first_angle.z + PI / 2;
    if (controler_mpu6050_processed.angle_remove_first.z < 0)
    {
        controler_mpu6050_processed.angle_remove_first.z += 2 * PI;
    }
    else if (controler_mpu6050_processed.angle_remove_first.z > 2 * PI)
    {
        controler_mpu6050_processed.angle_remove_first.z -= 2 * PI;
    }
}

// private end------------------

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
void mpu6050_init()
{
    HAL_UART_Receive_IT(&huart2, &mpu_rxdat, 1);
    mpu6050_change_baudrate(115200);
    mpu6050_set_first_angle();
}

/**
 * @brief 读取MPU6050并运行相关算法
 */
void mpu6050_process(){
    mpu6050_read();
    mpu6050_remove_first_angle();
}
