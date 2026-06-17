/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* 视觉数据结构体 — USART1 DMA 接收后打包 → VisionQueue → PIDTask */
typedef struct {
    float    nx;        /* 归一化坐标 X [-1.0, +1.0] */
    float    ny;        /* 归一化坐标 Y [-1.0, +1.0] */
    float    vx;        /* X 方向速度 */
    float    vy;        /* Y 方向速度 */
    uint32_t timestamp; /* 时间戳 ms，PID 端用于判断数据新鲜度 */
} VisionData_t;

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

extern osMessageQueueId_t VisionQueueHandle;  /* 视觉数据消息队列句柄，ISR 和任务共用 */

/* —— 视觉模块初始化 —— */
void Vision_StartDMA(void);                   /* 启动 USART1 DMA 环形接收 + IDLE 中断 */
void Vision_ParseFrame(uint8_t *buf, uint16_t len, VisionData_t *out);  /* 从环内数据解析一帧 */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/

/* USER CODE BEGIN Private defines */

/* ——— 视觉通信协议 ——— */
#define VISION_FRAME_HEADER  0xA5  /* 帧头标识 */
#define VISION_FRAME_SIZE    18    /* 一帧总字节数：帧头(1) + nx(4) + ny(4) + vx(4) + vy(4) + CRC8(1) */
#define DMA_RX_BUF_SIZE      128   /* DMA 环形缓冲区大小，约可缓冲 ~7 帧 */
#define VISION_CRC8_POLY     0x07  /* CRC-8-ATM 多项式 */

/* ——— 视觉模块共享变量 ——— */
extern uint8_t  dma_rx_buf[DMA_RX_BUF_SIZE]; /* DMA 环形缓冲区 */
extern uint16_t vision_parse_idx;            /* 上次解析到的环内位置 */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
