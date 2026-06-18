/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "Emm_V5.h"  /* 电机控制函数、MOTOR_X/Y_ADDR、ABS 宏 */
#include "queue.h"    /* FreeRTOS 队列 API：xQueueCreate / xQueueSendToBack / xQueueReceive */
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
/* USER CODE BEGIN Variables */

/* 视觉数据队列句柄（FreeRTOS 原生，长度 3，取最新帧时弹旧帧） */
QueueHandle_t VisionQueue;

/* PIDTask → CommandTask 共享电机指令 */
MotorCmd_t motor_cmd;

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for PIDTask */
osThreadId_t PIDTaskHandle;
const osThreadAttr_t PIDTask_attributes = {
  .name = "PIDTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for VisionTask */
osThreadId_t VisionTaskHandle;
const osThreadAttr_t VisionTask_attributes = {
  .name = "VisionTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for CommandTask */
osThreadId_t CommandTaskHandle;
const osThreadAttr_t CommandTask_attributes = {
  .name = "CommandTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for CmdMutex */
osMutexId_t CmdMutexHandle;
const osMutexAttr_t CmdMutex_attributes = {
  .name = "CmdMutex"
};
/* Definitions for CtrlBinarySem */
osSemaphoreId_t CtrlBinarySemHandle;
const osSemaphoreAttr_t CtrlBinarySem_attributes = {
  .name = "CtrlBinarySem"
};
/* Definitions for VisionBinarySem */
osSemaphoreId_t VisionBinarySemHandle;
const osSemaphoreAttr_t VisionBinarySem_attributes = {
  .name = "VisionBinarySem"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void StartPIDTask(void *argument);
void StartVisionTask(void *argument);
void StartCommandTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */
  /* Create the mutex(es) */
  /* creation of CmdMutex */
  CmdMutexHandle = osMutexNew(&CmdMutex_attributes);

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of CtrlBinarySem */
  CtrlBinarySemHandle = osSemaphoreNew(1, 1, &CtrlBinarySem_attributes);

  /* creation of VisionBinarySem */
  VisionBinarySemHandle = osSemaphoreNew(1, 1, &VisionBinarySem_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* 视觉数据队列：长度 3，FreeRTOS 原生，手动覆盖旧数据 */
  VisionQueue = xQueueCreate(3, sizeof(VisionData_t));
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of PIDTask */
  PIDTaskHandle = osThreadNew(StartPIDTask, NULL, &PIDTask_attributes);

  /* creation of VisionTask */
  VisionTaskHandle = osThreadNew(StartVisionTask, NULL, &VisionTask_attributes);

  /* creation of CommandTask */
  CommandTaskHandle = osThreadNew(StartCommandTask, NULL, &CommandTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
  osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartPIDTask */
/**
* @brief Function implementing the PIDTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartPIDTask */
void StartPIDTask(void *argument)
{
  /* USER CODE BEGIN StartPIDTask */
  VisionData_t data;
  const float   kyp  = 500.0f;
  const float   kxp  = 400.0f;
  const float   kyd  = 110.0f;
  const float   kxd  = 80.0f;

  for(;;)
  {
    /* 阻塞等待视觉数据 */
    if (xQueueReceive(VisionQueue, &data, portMAX_DELAY) == pdTRUE)
    {
      float out_x = kxp * data.nx + kxd * (data.vx);
      float out_y = kyp * data.ny + kyd * (data.vy);

      int32_t raw_x = (int32_t)out_x;
      int32_t raw_y = (int32_t)out_y;

      /* 限幅 ±400 */
      if (raw_x >  400) raw_x =  500;
      if (raw_x < -400) raw_x = -500;
      if (raw_y >  400) raw_y =  500;
      if (raw_y < -400) raw_y = -500;

      /* 填指令结构体 */
      motor_cmd.dir_x = (raw_x >= 0) ? 1 : 0;  /* X 轴反向 */
      motor_cmd.clk_x = (uint32_t)ABS(raw_x);
      motor_cmd.dir_y = (raw_y >= 0) ? 1 : 0;  /* Y 轴反向 */
      motor_cmd.clk_y = (uint32_t)ABS(raw_y);
      /* 唤醒 CommandTask 执行电机指令 */
      xTaskNotifyGive(CommandTaskHandle);
    }
  }
  /* USER CODE END StartPIDTask */
}

/* USER CODE BEGIN Header_StartVisionTask */
/**
* @brief Function implementing the VisionTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartVisionTask */
void StartVisionTask(void *argument)
{
  /* USER CODE BEGIN StartVisionTask */
  VisionData_t data;
  uint16_t    safety;

  for(;;)
  {
    /* 等待 ISR 唤醒 */
    osSemaphoreAcquire(VisionBinarySemHandle, osWaitForever);

    safety = 0;
    while (vision_parse_idx != vision_cur_idx && safety < DMA_RX_BUF_SIZE)
    {
      safety++;
      if (dma_rx_buf[vision_parse_idx] == VISION_FRAME_HEADER)
      {
        uint16_t data_start = (vision_parse_idx + 1) % DMA_RX_BUF_SIZE;

        if (data_start + 17 <= DMA_RX_BUF_SIZE)
        {
          /* 帧在环内连续 */
          Vision_ParseFrame(&dma_rx_buf[data_start], 17, &data);
        }
        else
        {
          /* 帧跨环尾，拼两段到临时 buf */
          uint8_t tmp[17];
          uint16_t part1 = DMA_RX_BUF_SIZE - data_start;
          uint16_t part2 = 17 - part1;
          for (uint16_t i = 0; i < part1; i++) tmp[i]      = dma_rx_buf[data_start + i];
          for (uint16_t i = 0; i < part2; i++) tmp[part1 + i] = dma_rx_buf[i];
          Vision_ParseFrame(tmp, 17, &data);
        }

        if (data.timestamp != 0)
        {
          /* 写队列：满了弹最老再写，始终保留最新 3 帧 */
          if (xQueueSendToBack(VisionQueue, &data, 0) == errQUEUE_FULL) {
            VisionData_t dummy;
            xQueueReceive(VisionQueue, &dummy, 0);
            xQueueSendToBack(VisionQueue, &data, 0);
          }
        }

        vision_parse_idx = (vision_parse_idx + VISION_FRAME_SIZE) % DMA_RX_BUF_SIZE;
      }
      else
      {
        vision_parse_idx = (vision_parse_idx + 1) % DMA_RX_BUF_SIZE;
      }
    }
  }
  /* USER CODE END StartVisionTask */
}

/* USER CODE BEGIN Header_StartCommandTask */
/**
* @brief Function implementing the CommandTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartCommandTask */
void StartCommandTask(void *argument)
{
  /* USER CODE BEGIN StartCommandTask */
  const uint16_t vel = 1000;
  const uint8_t  acc = 200;

  for(;;)
  {
    /* 等待 PIDTask 唤醒 */
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    /* 快照电机指令，防止 PIDTask 在 X/Y 之间覆盖 */
    MotorCmd_t cmd = motor_cmd;
    /* X 电机目标位置 */
    Emm_V5_Pos_Control(MOTOR_X_ADDR, cmd.dir_x, vel, acc, cmd.clk_x, true, true);
    osDelay(3);
    /* Y 电机目标位置 */
    Emm_V5_Pos_Control(MOTOR_Y_ADDR, cmd.dir_y, vel, acc, cmd.clk_y, true, true);
    osDelay(3);
    /* 广播同步触发 */
    Emm_V5_Synchronous_motion(0);
    osDelay(3);
  }
  /* USER CODE END StartCommandTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

