#include "led.h"

// private start------------------
/**
 * @brief 设置LED引脚状态
 * @param red   红色引脚
 * @param green 绿色引脚
 * @param red   蓝色引脚
 */
static void led_set(GPIO_PinState red, GPIO_PinState green, GPIO_PinState blue)
{
    //共阳极led进行翻转
    HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_R_Pin, (GPIO_PinState)!red);
    HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, (GPIO_PinState)!green);
    HAL_GPIO_WritePin(LED_B_GPIO_Port, LED_B_Pin, (GPIO_PinState)!blue);
}

/**
 * @brief 翻转led指定的颜色
 * @param color   颜色
 */
static void led_toggle(Led_Color color)
{
    // 1. 处理红色分量 (RED, YELLOW, PURPLE, WHITE)
    if (color == RED || color == YELLOW || color == PURPLE || color == WHITE)
    {
        HAL_GPIO_TogglePin(LED_R_GPIO_Port, LED_R_Pin);
    }

    // 2. 处理绿色分量 (GREEN, YELLOW, CYAN, WHITE)
    if (color == GREEN || color == YELLOW || color == CYAN || color == WHITE)
    {
        HAL_GPIO_TogglePin(LED_G_GPIO_Port, LED_G_Pin);
    }

    // 3. 处理蓝色分量 (BLUE, PURPLE, CYAN, WHITE)
    if (color == BLUE || color == PURPLE || color == CYAN || color == WHITE)
    {
        HAL_GPIO_TogglePin(LED_B_GPIO_Port, LED_B_Pin);
    }
    // 备注：如果 color 为 OFF，则不触发任何 Toggle，逻辑自然闭环。
}

/**
 * @brief 周期性触发LED翻转
 * @note 需要确保主函数其他地方没有反复操作led的地方，否则闪烁效果会不符合预期
 */
static void Run_LED_Pattern_Task(void)
{
    static uint16_t counter = 0; // 静态计数器，记录函数执行次数
    counter++;                   // 每次进入函数，计数自增
    // 逻辑判定
    if (counter == 9)
    {
        led_toggle(RED);
    }
    else if (counter == 10)
    {
        led_toggle(RED);
        counter = 0; // 完成一个周期，计数器清零重新开始
    }
}

// private end------------------

/**
 * @brief 初始化LED
 */
void led_init(){
    led_set(GPIO_PIN_RESET, GPIO_PIN_RESET, GPIO_PIN_RESET);
}

/**
 * @brief LED处理函数
 */
void led_process()
{
    switch (car_led_color)
    {
    case OFF:{
        led_set(GPIO_PIN_RESET, GPIO_PIN_RESET, GPIO_PIN_RESET);
        break;
    }
    case RED:{
        led_set(GPIO_PIN_SET, GPIO_PIN_RESET, GPIO_PIN_RESET);
        break;
    }
    case GREEN:{
        led_set(GPIO_PIN_RESET, GPIO_PIN_SET, GPIO_PIN_RESET);
        break;
    }
    case BLUE:{
        led_set(GPIO_PIN_RESET, GPIO_PIN_RESET, GPIO_PIN_SET);
        break;
    }
    case YELLOW:{
        led_set(GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_RESET);
        break;
    }
    case PURPLE:{
        led_set(GPIO_PIN_SET, GPIO_PIN_RESET, GPIO_PIN_SET);
        break;
    }
    case CYAN:
    {
        led_set(GPIO_PIN_RESET, GPIO_PIN_SET, GPIO_PIN_SET);
        break;
    }
    case WHITE:{
        led_set(GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_SET);
        break;
    }
    case BREATH:
    {
        Run_LED_Pattern_Task();
        break;
    }

    default:
        break;
    }
}
