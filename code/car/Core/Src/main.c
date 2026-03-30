/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "system.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

uint32_t ms_cnt = 0;

/**
 * @brief 各个时间点的定时执行函数
 */
void time_1ms_function()
{
  buzzer_process();
}
void time_10ms_function()
{
  position_10ms_process();
}
void time_100ms_function()
{
}
void time_1s_function()
{
}
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM1)
  {
    ms_cnt++;
    time_1ms_function();
    if (ms_cnt % 10 == 0)
    {
      time_10ms_function();
    }
    if (ms_cnt % 100 == 0)
    {
      time_100ms_function();
    }
    if (ms_cnt == 1000) // 最开始写成了“ ms_cnt = 1000 ”导致if一直被执行，ms_cnt一直被自动赋值为0
    {
      time_1s_function();
      ms_cnt = 0;
    }
  }
}

/**
 * @brief 串口中断函数
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart == &huart2)
  {
    MPU_UART_Receive();
  }
  else if (huart == &huart1)
  {
    uart1_receivce();
  }
}

/**
 * @brief 各个时间点的定时执行函数
 * @param num 需要记录第几个函数的执行时间
 */
void caculate_tick(int num)
{
  static uint32_t last_time = 0;
  uint32_t current_time = HAL_GetTick();

  if (num == 0)
  {
    // 第一个任务：从循环开始到第一个任务结束的时间
    main_during_tick[0] = (int)(current_time - main_start_tick);
    if (main_during_tick[0] > main_during_tick_max[0])
    {
      main_during_tick_max[0] = main_during_tick[0];
    }
  }
  else
  {
    // 其他任务：从上个任务结束到当前任务结束的时间
    main_during_tick[num] = (int)(current_time - last_time);
    if (main_during_tick[num] > main_during_tick_max[num])
    {
      main_during_tick_max[num] = main_during_tick[num];
    }
  }

  // 保存当前时间，供下一个任务使用
  last_time = current_time;
}

/**
 * @brief nrf外部中断执行函数
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  nrf_receive_process_irq();
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  MX_USART2_UART_Init();
  MX_ADC1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  show_init();
  key_init();
  position_init();
  motor_init();
  NRF24L01_Init();
  buzzer_init();
  uart1_init();
  led_init();
  VL53L1_init_all();
  Tracker_Init(&tracker, car_track, current_track_length, 0.1f, 0.8f);

  control_mode = NRF_CTRL;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    main_start_tick = HAL_GetTick();

    key_read(), caculate_tick(0);
    key_process(), caculate_tick(1);
    oled_process(), caculate_tick(2);
    oled_display(), caculate_tick(3);
    nrf_receive_process_main(), caculate_tick(4);
    led_process(), caculate_tick(5);
    uart1_process(), caculate_tick(6);
    position_main_process(), caculate_tick(7);
    drive(), caculate_tick(8);
    Update_Map(), caculate_tick(9);

    main_all_tick = HAL_GetTick() - main_start_tick;
    if (main_all_tick > main_all_tick_max)
    {
      main_all_tick_max = main_all_tick;
    }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
