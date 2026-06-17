#include "vision.h"
#include "usart.h"     /* huart1, HAL_UART_xxx */

/* ================================================================ */
/*  全局变量定义                                                     */
/* ================================================================ */
uint8_t  dma_rx_buf[DMA_RX_BUF_SIZE];  /* DMA 环形缓冲区 */
uint16_t vision_parse_idx = 0;         /* 上次解析到的环内偏移 */

/* ================================================================ */
/*  帧解析：buf 指向帧头 0xA5 之后的 17 字节，尾字节 0x8A 则解包    */
/* ================================================================ */
void Vision_ParseFrame(uint8_t *buf, uint16_t len, VisionData_t *out)
{
    (void)len;

    if (buf[16] == 0x8A)
    {
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
        out->nx = out->ny = out->vx = out->vy = 0.0f;
        out->timestamp = 0;
    }
}

/* ================================================================ */
/*  启动 USART1 DMA 环形接收 + IDLE 中断                             */
/* ================================================================ */
void Vision_StartDMA(void)
{
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
    HAL_UART_Receive_DMA(&huart1, dma_rx_buf, DMA_RX_BUF_SIZE);
}
