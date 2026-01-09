#ifndef __I2C_H__
#define __I2C_H__


#include "Type_def.h" 




// -------------------------- 系统配置宏（需根据硬件修改） --------------------------
#define I2C_FREQ    100000UL    // I2C/SMBus通信速率（Hz，默认100KHz）
#define I2C_TIMEOUT_LONG      10000
// -------------------------- 错误码定义（通用I2C错误） --------------------------
#define I2C_SUCCESS        0x00  // 操作成功
#define I2C_ERR_NACK       0x01  // 未收到ACK应答
#define I2C_ERR_TIMEOUT    0x02  // SCL低电平25ms超时
#define I2C_ERR_ARB_LOST   0x03  // 总线仲裁失败
#define I2C_ERR_BUSY       0x04  // 总线忙
#define I2C_ERR_PARAM      0x05  // 参数错误
#define I2C_ERR_STATE      0x06  // 错误



// -------------------------- 通用I2C状态码（核心状态） --------------------------
#define I2C_STA_SENT       0x08  // 起始条件发送成功
#define I2C_REP_STA_SENT   0x10  // 重复起始条件发送成功
#define I2C_SLA_W_ACK      0x18  // 发送从地址+写，收到ACK
#define I2C_SLA_W_NACK     0x20  // 发送从地址+写，未收到ACK
#define I2C_DATA_W_ACK     0x28  // 发送数据，收到ACK
#define I2C_DATA_W_NACK    0x30  // 发送数据，未收到ACK
#define I2C_SLA_R_ACK      0x40  // 发送从地址+读，收到ACK
#define I2C_SLA_R_NACK     0x48  // 发送从地址+读，未收到ACK
#define I2C_DATA_R_ACK     0x50  // 接收数据，发送ACK
#define I2C_DATA_R_NACK    0x58  // 接收数据，发送NACK
#define I2C_ARB_LOST       0x38  // 仲裁失败

// -------------------------- 通用I2C对外接口声明 --------------------------
/**
 * @brief  通用I2C初始化（含引脚、定时器3、中断配置，遵循SMBus规范）
 * @param  无
 * @return 无
 */
void I2C_Init(void);

/**
 * @brief  发送I2C起始条件
 * @param  无
 * @return 错误码（I2C_SUCCESS=成功）
 */
uint8_t I2C_Start(void);

/**
 * @brief  发送I2C重复起始条件
 * @param  无
 * @return 错误码（I2C_SUCCESS=成功）
 */
uint8_t I2C_RepeatStart(void);

/**
 * @brief  发送I2C停止条件
 * @param  无
 * @return 无
 */
void I2C_Stop(void);

/**
 * @brief  发送从设备地址（含读写方向）
 * @param  slaAddr：7位从地址（bit7~bit1）+ 读写位（bit0：0=写，1=读）
 * @return 错误码（I2C_SUCCESS=收到ACK，I2C_ERR_NACK=未收到ACK）
 */
uint8_t I2C_SendAddr(uint8_t slaAddr);

/**
 * @brief  发送1字节数据
 * @param  data：要发送的字节
 * @return 错误码（I2C_SUCCESS=收到ACK，I2C_ERR_NACK=未收到ACK）
 */
uint8_t I2C_SendData(uint8_t send_data);

/**
 * @brief  接收1字节数据（可配置ACK/NACK）
 * @param  ack：1=接收后发送ACK，0=接收后发送NACK
 * @param  pData：接收数据的缓冲区指针
 * @return 错误码（I2C_SUCCESS=成功）
 */
uint8_t I2C_RecvData(uint8_t ack, uint8_t *pData);

/**
 * @brief  检查I2C总线是否空闲
 * @param  无
 * @return 1=总线忙，0=总线空闲
 */
bit I2C_IsBusy(void);

/**
 * @brief  等待I2C操作完成（内部使用，带超时）
 * @param  timeout：超时计数
 * @return 错误码（I2C_SUCCESS=成功，I2C_ERR_TIMEOUT=超时）
 */
uint8_t I2C_WaitDone(uint32_t timeout);

#endif  // __I2C_H__