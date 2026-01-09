#ifndef __SPI_H__
#define __SPI_H__


#include "config.h"

// -------------------------- SPI 引脚定义（3线模式，与I2C无冲突）--------------------------
// 交叉开关映射：SPI0 -> P0.2(MOSI)、P0.3(MISO)、P0.4(SCK)
#define SPI_MOSI_PIN    P0_2    // 主机输出，从机输入
#define SPI_MISO_PIN    P0_3    // 主机输入，从机输出
#define SPI_SCK_PIN     P0_4    // 时钟线


// -------------------------- SPI 配置参数 --------------------------
#define SPI_CLOCK_FREQ  200000UL // SPI 时钟频率：200K（可调整）

#define SPIBSY  0x80

typedef enum
{

   SPI_OK =0,
	 SPI_TIMEOUT,//超时
   SPI_PARA_ERR//参数错误

}SPI_ID;









// -------------------------- SPI 函数声明 --------------------------
/**
 * @brief  SPI 初始化（3线模式，主机模式，CPOL=0，CPHA=0）
 * @param  无
 * @return 无
 */
void SPI_Init(void);

/**
 * @brief  SPI 发送接收一个字节（全双工）
 * @return 接收的字节
 */
uint8_t SPI_Transceive_Byte(uint8_t tx_data,uint8_t *read_data);




#endif // __SPI_H__