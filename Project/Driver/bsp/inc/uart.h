

#ifndef _UART_H
#define _UART_H



#include "config.h"

#define RX_BUF_SIZE         32          /* 接收缓冲区大小 */
#define TX_BUF_SIZE         256         /* 发送缓冲区大小 */
#define MB_FRAME_TIMEOUT    4           /* 接收超时计数（4*10ms=40ms） */

extern volatile uint8_t rx_buf[RX_BUF_SIZE];   /* 接收缓冲区 */
extern volatile uint8_t rx_state;              /* 接收状态：0=空闲，1=接收中，2=接收完成 */
extern volatile uint8_t rx_len;                /* 已接收字节数 */
extern volatile uint8_t rx_timeout_cnt;        /* 接收超时计数器（10ms递增） */

/* 发送相关 */
extern volatile uint8_t tx_buf[TX_BUF_SIZE];   /* 发送缓冲区 */
extern volatile uint8_t tx_len;                /* 待发送字节数 */
extern volatile uint8_t tx_state;              /* 发送状态：0=空闲，1=发送中 */
extern volatile uint8_t tx_index;              /* 发送字节索引 */
/**
 * @brief 串口初始化函数
 * @param 无
 * @return 无
 */
void UART0_Init(uint32_t uart_baudrate);

/**
 * @brief 串口重新配置波特率
 * @param new_baudrate 新的波特率值
 * @return 无
 */
void UART0_SetBaudrate(uint32_t new_baudrate);


/**
 * @brief 串口发送一个字节
 * @param dat 要发送的数据
 * @return 无
 */
void UART0_SendByte(uint8_t dat);

/**
 * @brief 串口发送字符串
 * @param str 字符串指针
 * @return 无
 */
void UART0_SendString(const char* str);


/**
 * @brief 串口发送数组
 * @param send_data 数组指针
 * @return 无
 */
void UART0_SendArray(uint8_t *send_data, uint8_t length);

uint8_t UART0_Start_Send(uint8_t len) ;
void UART0_SendBuf_Block(uint8_t *buf, uint8_t len);
#endif


