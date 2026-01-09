#include "spi.h"



// -------------------------- SPI 初始化 --------------------------
void SPI_Init(void) {
	  uint8_t spi0ckr;
    char SFRPAGE_SAVE = SFRPAGE;
    SFRPAGE = SPI0_PAGE; // 切换到 SPI0 模块 SFR 页（C8051F120 标准页地址）


    SPI0CFG = 0x60;      // CPOL=0（空闲时SCK低），CPHA=0（第一个时钟沿采样），主机
    SPI0CN = 0x00; 
    //  配置 SPI 时钟频率
    // 公式：SPI 时钟 = SYSCLK / (2 * (SPI0CKR + 1))
    spi0ckr = (SYSCLK / (2 * SPI_CLOCK_FREQ)) - 1;
    if (spi0ckr > 0xFF) spi0ckr = 0xFF; // 限制最大值
    SPI0CKR = spi0ckr;

    // 启用 SPI0 模块

		NSSMD1=0;
		NSSMD0=0;
    SPIEN =1;	
    SFRPAGE = SFRPAGE_SAVE;


}

// 功能：发送tx_data字节，同时接收1字节并返回；超时返回SPI_TIMEOUT
uint8_t SPI_Transceive_Byte(uint8_t tx_data,uint8_t *read_data) {
    // C89要求：变量声明放在函数开头
    unsigned char SFRPAGE_SAVE = SFRPAGE;
    unsigned int timeout = 0;

    if(read_data==NULL)
			return SPI_PARA_ERR;

    SFRPAGE = SPI0_PAGE;

    // 等待SPI总线空闲（避免总线忙时写数据冲突）
    timeout = 0;
    while ((SPI0CFG & SPIBSY) != 0) {
        timeout++;
        if (timeout > 200) {  // 短超时，防止卡死
            SFRPAGE = SFRPAGE_SAVE;
            return SPI_TIMEOUT;
        }
    }

    // 等待发送缓冲器空（确保能写入新数据）
    timeout = 0;
    while ((TXBMT) == 0) {
        timeout++;
        if (timeout > 200) {
            SFRPAGE = SFRPAGE_SAVE;
            return SPI_TIMEOUT;
        }
    }

    // 写入发送数据，触发SPI传输（主模式自动输出SCK）
    SPI0DAT = tx_data;

    // 等待传输完成（SPIF置1=发送完成+接收就绪）
    timeout = 0;
    while ((SPIF) == 0) {
        timeout++;
        if (timeout > 500) {  // ~800μs超时，适配多数从机响应
            SPI0CN &= ~SPIF;  // 超时强制清标志，避免后续异常
            SFRPAGE = SFRPAGE_SAVE;
            return SPI_TIMEOUT;
        }
    }

    // 读取接收的字节（全双工核心，必须读！否则SPIF清不掉）
    *read_data = SPI0DAT;

    //清除SPIF标志，准备下一次传输
    SPIF=0;

    // 恢复SFR分页，避免影响其他外设
    SFRPAGE = SFRPAGE_SAVE;

    // 返回接收的字节
    return SPI_OK;
}