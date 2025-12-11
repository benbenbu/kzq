#ifndef _MODBUS_H
#define _MODBUS_H


#include "config.h"



// Modbus相关定义
extern uint8_t modbus_slave_addr;       // 从机地址（可修改）
#define MODBUS_TIMEOUT_MS   1500        // Modbus超时时间（1.5秒）
#define MODBUS_RX_BUF_SIZE  256         // 接收缓冲区大小

// Modbus功能码定义
#define MODBUS_READ_DISCRETE_INPUTS     0x02  // 读离散输入
#define MODBUS_READ_HOLDING_REGISTERS   0x03  // 读保持寄存器
#define MODBUS_READ_INPUT_REGISTERS     0x04  // 读输入寄存器

// Modbus异常码定义
#define MODBUS_EXCEPTION_ILLEGAL_FUNCTION     0x01  // 非法功能
#define MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS 0x02  // 非法数据地址
#define MODBUS_EXCEPTION_ILLEGAL_DATA_VALUE   0x03  // 非法数据值
#define MODBUS_EXCEPTION_SLAVE_DEVICE_FAILURE 0x04  // 从机设备故障

// 全局变量声明
extern uint32_t uart_baudrate;                  // 波特率变量
extern uint8_t modbus_rx_buffer[MODBUS_RX_BUF_SIZE];  // Modbus接收缓冲区
extern volatile uint8_t modbus_rx_index;        // 接收缓冲区索引
extern volatile uint8_t modbus_rx_complete;     // 接收完成标志
extern volatile uint16_t modbus_timeout_counter; // 超时计数器
extern volatile uint8_t modbus_timer_active;    // 计时器活动标志
extern uint16_t modbus_registers[100];          // Modbus寄存器数组

// 函数声明






/**
 * @brief CRC16计算
 * @param data 数据指针
 * @param length 数据长度
 * @return CRC16校验值
 */
uint16_t Modbus_CRC16(uint8_t *pucBuff, uint8_t unNum);

/**
 * @brief Modbus接收超时检查
 * @param 无
 * @return 无
 */
void Modbus_CheckTimeout(void);

/**
 * @brief 启动Modbus接收超时计时器
 * @param 无
 * @return 无
 */
void Modbus_StartTimeout(void);

/**
 * @brief 停止Modbus接收超时计时器
 * @param 无
 * @return 无
 */
void Modbus_StopTimeout(void);



/**
 * @brief 处理Modbus请求
 * @param 无
 * @return 无
 */
void Modbus_ProcessRequest(void);

/**
 * @brief 处理读离散输入请求
 * @param 无
 * @return 无
 */
void Modbus_HandleReadDiscreteInputs(void);

/**
 * @brief 处理读保持寄存器请求
 * @param 无
 * @return 无
 */
void Modbus_HandleReadHoldingRegisters(void);

/**
 * @brief 处理读输入寄存器请求
 * @param 无
 * @return 无
 */
void Modbus_HandleReadInputRegisters(void);

/**
 * @brief 发送Modbus错误响应
 * @param function_code 功能码
 * @param exception_code 异常码
 * @return 无
 */
void Modbus_SendError(uint8_t function_code, uint8_t exception_code);



#endif // _C8051F120_MODBUS_H