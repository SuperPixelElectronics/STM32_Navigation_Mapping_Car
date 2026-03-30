#include "motor.h"

// private start----------------
static void Drive_Motor(float V, float delta);
static void Incremental_PI_A(void);
static void Incremental_PI_B(void);
static float target_limit_float(float insert, float low, float high);
static void Limit_Pwm(int amplitude);
static void Set_Pwm(int motor_a, int motor_b);
/**************************************************************************
Function: The inverse kinematics solution is used to calculate the target speed of each wheel according to the target speed of three axes
Input   : X and Y, Z axis direction of the target movement speed
Output  : none
函数功能：运动学逆解，根据三轴目标速度计算各车轮目标转速
入口参数：
返回  值：无
**************************************************************************/
static void Drive_Motor(float V, float delta)
{
    float amplitude = 1; // Wheel target speed limit //车轮目标速度限幅
    // Inverse kinematics //运动学逆解
    MOTOR_B.Target = V + delta * Wheel_spacing / 2.0f; // 计算出右轮的目标速度
    MOTOR_A.Target = V - delta * Wheel_spacing / 2.0f; // 计算出左轮的目标速度

    // Wheel (motor) target speed limit //车轮(电机)目标速度限幅
    MOTOR_A.Target = target_limit_float(MOTOR_A.Target, -amplitude, amplitude);
    MOTOR_B.Target = target_limit_float(MOTOR_B.Target, -amplitude, amplitude);
}

/**************************************************************************
Function: Incremental PI controller
Input   : Encoder measured value (actual speed), target speed
Output  : Motor PWM
According to the incremental discrete PID formula
pwm+=Kp[e（k）-e(k-1)]+Ki*e(k)+Kd[e(k)-2e(k-1)+e(k-2)]
e(k) represents the current deviation
e(k-1) is the last deviation and so on
PWM stands for incremental output
In our speed control closed loop system, only PI control is used
pwm+=Kp[e（k）-e(k-1)]+Ki*e(k)

函数功能：增量式PI控制器
入口参数：编码器测量值(实际速度)，目标速度
返回  值：电机PWM
根据增量式离散PID公式
pwm+=Kp[e（k）-e(k-1)]+Ki*e(k)+Kd[e(k)-2e(k-1)+e(k-2)]
e(k)代表本次偏差
e(k-1)代表上一次的偏差  以此类推
pwm代表增量输出
在我们的速度控制闭环系统里面，只使用PI控制
pwm+=Kp[e（k）-e(k-1)]+Ki*e(k)
**************************************************************************/
static void Incremental_PI_A()
{
    MOTOR_A.Bias = MOTOR_A.Target - MOTOR_A.Encoder; // Calculate the deviation //计算偏差

    // 小车走了一会儿，如果把速度置零，会出现电机转速变慢，然后电机扭力不够，电机变成静止，但是因为电机编码器没转够，算出的pwm仍有数值，电机上仍有电流
    // 这是典型的积分饱和（积分风车）问题。当目标速度设为0，但积分项还在累加，导致电机“卡”在一个有电流但不转动的状态。
    // 因此需要当车子电机为静止状态时对pwm数值进行衰减
    if (abs(MOTOR_A.Target) < 0.001f && abs(MOTOR_A.Bias) < 0.01f)
    {
        MOTOR_A.Motor_Pwm = MOTOR_A.Motor_Pwm * 0.7;
    }

    MOTOR_A.Motor_Pwm += MOTOR_A.Velocity_KP * (MOTOR_A.Bias - MOTOR_A.Last_bias) + MOTOR_A.Velocity_KI * MOTOR_A.Bias;
    MOTOR_A.Last_bias = MOTOR_A.Bias; // Save the last deviation //保存上一次偏差
}
static void Incremental_PI_B()
{
    MOTOR_B.Bias = MOTOR_B.Target - MOTOR_B.Encoder; // Calculate the deviation //计算偏差
    if (abs(MOTOR_B.Target) < 0.001f && abs(MOTOR_B.Bias) < 0.01f)
    {
        MOTOR_B.Motor_Pwm = MOTOR_B.Motor_Pwm * 0.7;
    }
    MOTOR_B.Motor_Pwm += MOTOR_B.Velocity_KP * (MOTOR_B.Bias - MOTOR_B.Last_bias) + MOTOR_B.Velocity_KI * MOTOR_B.Bias;
    MOTOR_B.Last_bias = MOTOR_B.Bias; // Save the last deviation //保存上一次偏差
}

/**************************************************************************
Function: Limiting function
Input   : Value
Output  : none
函数功能：限幅函数
入口参数：幅值
返回  值：无
**************************************************************************/
static float target_limit_float(float insert, float low, float high)
{
    if (insert < low)
        return low;
    else if (insert > high)
        return high;
    else
        return insert;
}

/**************************************************************************
Function: Limit PWM value
Input   : Value
Output  : none
函数功能：限制PWM值
入口参数：幅值
返回  值：无
**************************************************************************/
static void Limit_Pwm(int amplitude)
{
    // 之前写成了“MOTOR_A.Target >= 0”导致判断不准确
    if (MOTOR_A.Motor_Pwm >= 0)
    {
        MOTOR_A.Motor_Pwm = target_limit_float(MOTOR_A.Motor_Pwm, 0, amplitude);
    }
    else if (MOTOR_A.Motor_Pwm < 0)
    {
        MOTOR_A.Motor_Pwm = target_limit_float(MOTOR_A.Motor_Pwm, -amplitude, 0);
    }
    if (MOTOR_B.Motor_Pwm >= 0)
    {
        MOTOR_B.Motor_Pwm = target_limit_float(MOTOR_B.Motor_Pwm, 0, amplitude);
    }
    else if (MOTOR_B.Motor_Pwm < 0)
    {
        MOTOR_B.Motor_Pwm = target_limit_float(MOTOR_B.Motor_Pwm, -amplitude, 0);
    }
}

/**************************************************************************
Function: Assign a value to the PWM register to control wheel speed and direction
Input   : PWM
Output  : none
函数功能：控制车轮转速与方向
入口参数：PWM
返回  值：无
**************************************************************************/
static void Set_Pwm(int motor_a, int motor_b)
{
    // Forward and reverse control of motor
    // 电机正反转控制
    if (motor_a < 0)
    {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);   // MOTOR_A1
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET); // MOTOR_A2

        // 这里之前把pwm参数写成了“1000 - motor_a”，造成当传入很小的负值pwm的时候电机转的很快
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, -motor_a);
    }
    else
    {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET); // MOTOR_A1
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);   // MOTOR_A2
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, motor_a);
    }

    // Forward and reverse control of motor
    // 电机正反转控制
    if (motor_b < 0)
    {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_SET);    // MOTOR_B1
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET); // MOTOR_B2
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, -motor_b);
    }
    else
    {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET); // MOTOR_B1
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_SET);  // MOTOR_B2
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, motor_b);
    }
}
// private end----------------

/**************************************************************************
函数功能：电机初始化函数
入口参数：无
返回  值：无
**************************************************************************/
void motor_init()
{
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    MOTOR_A.Velocity_KP = 300;
    MOTOR_A.Velocity_KI = 300;
    MOTOR_B.Velocity_KP = 300;
    MOTOR_B.Velocity_KI = 300;
}


/**************************************************************************
函数功能：根据不同模式来控制电机
入口参数：无
返回  值：无
**************************************************************************/
void drive()
{
    if (control_mode == HAND_PWM)
    {
    }
    else if (control_mode == HAND_LR)
    {
        Incremental_PI_A();
        Incremental_PI_B();
    }
    else if (control_mode == HAND_XYZ)
    {
        Drive_Motor(car_set_velocity.v, car_set_velocity.angular);
        Incremental_PI_A();
        Incremental_PI_B();
    }
    else if (control_mode == JOY_XYZ)
    {
        if (oled_curser.motor_control_page.curser == 3 && oled_curser.motor_control_page.selected == ENABLE)
        {
            if (car_key[0].state == KEY_PRESSING)
            {
                car_set_velocity.v = four_dirction_velocity, car_set_velocity.angular = 0;
            }
            else if (car_key[1].state == KEY_PRESSING)
            {
                car_set_velocity.v = -four_dirction_velocity, car_set_velocity.angular = 0;
            }
            else if (car_key[2].state == KEY_PRESSING)
            {
                car_set_velocity.v = four_dirction_velocity, car_set_velocity.angular = four_dirction_angular;
            }
            else if (car_key[3].state == KEY_PRESSING)
            {
                car_set_velocity.v = four_dirction_velocity, car_set_velocity.angular = -four_dirction_angular;
            }
            else
            {
                car_set_velocity.v = 0, car_set_velocity.angular = 0;
            }
            Drive_Motor(car_set_velocity.v, car_set_velocity.angular);
        }
        Incremental_PI_A();
        Incremental_PI_B();
    }
    else if (control_mode == HAND_TRACK)
    {
        if (tracker.state == ENABLE)
        {
            // 更新轨迹追踪器
            Tracker_Update(&tracker, car_combined_processed.CombinedPosition);

            // 获取控制指令并应用到全局变量
            car_set_velocity.v = tracker.cmd_linear;
            car_set_velocity.angular = tracker.cmd_angular;
            
        }
        else
        {
            car_set_velocity.v = 0;
            car_set_velocity.angular = 0;
        }

        Drive_Motor(car_set_velocity.v, car_set_velocity.angular);
        Incremental_PI_A();
        Incremental_PI_B();
    }
    else if (control_mode == NRF_CTRL)
    {
        if (tracker.state == ENABLE)
        {
            // 更新轨迹追踪器
            Tracker_Update(&tracker, car_combined_processed.CombinedPosition);

            if (nrf_control_mode == CAR_CONTROL){
                // 获取控制指令并应用到全局变量
                car_set_velocity.v = tracker.cmd_linear;
                car_set_velocity.angular = tracker.cmd_angular;
            }

        }
        Drive_Motor(car_set_velocity.v, car_set_velocity.angular);
        Incremental_PI_A();
        Incremental_PI_B();
    }

    Limit_Pwm(1000);
    Set_Pwm((int)MOTOR_A.Motor_Pwm, (int)MOTOR_B.Motor_Pwm);
}
