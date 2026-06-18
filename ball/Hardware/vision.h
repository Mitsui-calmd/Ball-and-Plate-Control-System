#ifndef __VISION_H__
#define __VISION_H__

#include "cmsis_os.h"  /* osMessageQueueId_t, osMessageQueuePut 等 */
#include "queue.h"     /* QueueHandle_t，FreeRTOS 队列 API */

/* ——— 视觉通信协议 ——— */
#define VISION_FRAME_HEADER  0xA5  /* 帧头标识 */
#define VISION_FRAME_SIZE    18    /* 一帧总字节：帧头(1) + nx(4) + ny(4) + vx(4) + vy(4) + 包尾(1) */
#define DMA_RX_BUF_SIZE      256   /* DMA 环形缓冲区大小，约可缓冲 ~14 帧 */

/* ——— 视觉数据结构体 ——— */
typedef struct {
    float    nx;        /* 归一化坐标 X [-1.0, +1.0] */
    float    ny;        /* 归一化坐标 Y [-1.0, +1.0] */
    float    vx;        /* X 方向速度 */
    float    vy;        /* Y 方向速度 */
    uint32_t timestamp; /* 时间戳 ms，PID 端用于判断数据新鲜度 */
} VisionData_t;

/* ——— 共享变量 ——— */
extern QueueHandle_t      VisionQueue;                  /* 视觉数据队列，长度3，FreeRTOS原生API */
extern osSemaphoreId_t    CtrlBinarySemHandle;          /* TIM2 → PIDTask 同步信号量（当前闲置） */
extern uint8_t            dma_rx_buf[DMA_RX_BUF_SIZE];  /* DMA 环形缓冲区 */
extern uint16_t           vision_parse_idx;             /* 上次解析到的环内位置 */
extern uint16_t           vision_cur_idx;               /* ISR 写入的 DMA 当前接收位置 */
extern osSemaphoreId_t    VisionBinarySemHandle;        /* ISR→VisionTask 通知信号量 */
extern osThreadId_t        CommandTaskHandle;            /* CommandTask 句柄，PIDTask 用 osThreadFlagsSet 唤醒 */

/* ——— 函数声明 ——— */
void Vision_StartDMA(void);                                              /* 启动 USART1 DMA 环形接收 + IDLE 中断 */
void Vision_ParseFrame(uint8_t *buf, uint16_t len, VisionData_t *out);   /* 从环内数据解析一帧 */

#endif
