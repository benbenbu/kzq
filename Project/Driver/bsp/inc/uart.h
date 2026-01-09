

#ifndef _UART_H
#define _UART_H



#include "config.h"


/**
 * @brief 串口初始化函数
 * @param 无
 * @return 无
 */
void UART0_Init(void);

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

#endif


