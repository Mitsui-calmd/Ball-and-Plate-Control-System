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
#include "cmsis_os.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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

/* DMA 环形缓冲区 — USART1 IDLE 中断中解析，CIRCULAR 模式自动回绕 */
uint8_t  dma_rx_buf[DMA_RX_BUF_SIZE];
uint16_t vision_parse_idx = 0;  /* 上次解析到的环内偏移，用于增量搜索 */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  MX_DMA_Init();
  MX_TIM2_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

  /* 启动 USART1 DMA 环形接收 + IDLE 中断 */
  Vision_StartDMA();

  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();  /* Call init function for freertos objects (in cmsis_os2.c) */
  MX_FREERTOS_Init();

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* ================================================================ */
/*  CRC-8 查表（多项式 0x07，初始值 0x00）                         */
/* ================================================================ */
static uint8_t Vision_CRC8(const uint8_t *data, uint16_t len)
{
  uint8_t crc = 0x00;
  while (len--)
  {
    crc ^= *data++;
    for (uint8_t i = 0; i < 8; i++)
      crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ VISION_CRC8_POLY) : (uint8_t)(crc << 1);
  }
  return crc;
}

/* ================================================================ */
/*  从环形缓冲区指定位置拷贝一个 float（小端，4 字节）                */
/* ================================================================ */
static float Vision_RingBufReadFloat(uint16_t start)
{
  union {
    float   f;
    uint8_t b[4];
  } u;
  u.b[0] = dma_rx_buf[(start + 0) % DMA_RX_BUF_SIZE];
  u.b[1] = dma_rx_buf[(start + 1) % DMA_RX_BUF_SIZE];
  u.b[2] = dma_rx_buf[(start + 2) % DMA_RX_BUF_SIZE];
  u.b[3] = dma_rx_buf[(start + 3) % DMA_RX_BUF_SIZE];
  return u.f;
}

/* ================================================================ */
/*  帧解析：buf 指向帧头 0xA5 之后的 17 字节，校验+填充              */
/*  调用方保证 buf 包含完整 17 字节（IDLE 中断中已验证）             */
/* ================================================================ */
void Vision_ParseFrame(uint8_t *buf, uint16_t len, VisionData_t *out)
{
  (void)len;  /* 调用方保证 len >= 17 */

  /* CRC8 校验: 对 buf[0]~buf[15]（nx+ny+vx+vy 共 16 字节）计算 */
  uint8_t crc_calc = Vision_CRC8(buf, 16);
  uint8_t crc_recv = buf[16];

  if (crc_calc == crc_recv)
  {
    /* 小端解包 */
    union { float f; uint8_t b[4]; } tmp;

    tmp.b[0] = buf[0];  tmp.b[1] = buf[1];  tmp.b[2] = buf[2];  tmp.b[3] = buf[3];
    out->nx = tmp.f;

    tmp.b[0] = buf[4];  tmp.b[1] = buf[5];  tmp.b[2] = buf[6];  tmp.b[3] = buf[7];
    out->ny = tmp.f;

    tmp.b[0] = buf[8];  tmp.b[1] = buf[9];  tmp.b[2] = buf[10]; tmp.b[3] = buf[11];
    out->vx = tmp.f;

    tmp.b[0] = buf[12]; tmp.b[1] = buf[13]; tmp.b[2] = buf[14]; tmp.b[3] = buf[15];
    out->vy = tmp.f;

    out->timestamp = HAL_GetTick();
  }
  else
  {
    /* 校验失败 — 标记为无效帧 */
    out->nx = out->ny = out->vx = out->vy = 0.0f;
    out->timestamp = 0;
  }
}

/* ================================================================ */
/*  启动 USART1 DMA 环形接收 + IDLE 中断                            */
/* ================================================================ */
void Vision_StartDMA(void)
{
  /* 使能 USART1 IDLE 中断 */
  __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);

  /* 启动 DMA 环形接收，数据持续写入 dma_rx_buf, CIRCULAR 模式自动回绕 */
  HAL_UART_Receive_DMA(&huart1, dma_rx_buf, DMA_RX_BUF_SIZE);
}

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM4 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM4)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

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
#ifdef USE_FULL_ASSERT
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
