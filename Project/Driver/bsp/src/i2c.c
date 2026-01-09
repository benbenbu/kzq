
#include "config.h"
#include "i2c.h"

// -------------------------- 全局变量（内部使用，不暴露给外部） --------------------------
static bit g_i2cBusy = 0;  // 软件忙标志（配合硬件BUSY位）
static uint8_t g_i2cErr = I2C_SUCCESS;  // 内部错误码



// -------------------------- SMB0CR计算（基于SMBus时钟公式） --------------------------
//空闲超时：TBFT ? [10 × (4×(256 - SMB0CR) + 1)] / SYSCLK
//SCL 低电平时间：TL = 4 × (256 - SMB0CR) / SYSCLK
//SCL 高电平时间：TH ≈ [4×(258 - SMB0CR) / SYSCLK] + 0.625
//I2C 速率：f = 1 / (TL + TH)（SCL 周期 = 高电平时间 + 低电平时间） 约等于93khz
// -------------------------- 通用I2C初始化 --------------------------
void I2C_Init(void) {

    char SFRPAGE_SAVE = SFRPAGE; 
    SFRPAGE = SMB0_PAGE;       // 切换到配置页
    // 配置SMBus时钟速率寄存器
    SMB0CR = 228;

    SMB0CN = 0x00;  // 清空所有位
    SMBFTE = 1;         // 使能50μs空闲超时检测
    SMBTOE = 0;         // 使能25ms低电平超时检测
    AA = 1;        // 默认允许发送ACK
    ENSMB = 1;      // 使能SMBus/I2C接口

    // 使能SMBus中断
//	  EIE1|=0x02;// 使能I2C中断
//    EIP1|=0x02; // 中断优先级（高）
    g_i2cBusy = 0;
    g_i2cErr = I2C_SUCCESS;
    SFRPAGE = SFRPAGE_SAVE;      
 
}




// -------------------------- I2C等待操作完成（带超时） --------------------------
uint8_t I2C_WaitDone(uint32_t timeout) {
    while ((SI == 0) && (timeout-- > 0)) {
        ;
    }
    if (SI == 0) {
        return I2C_ERR_TIMEOUT;
    }
    return I2C_SUCCESS;
}


// -------------------------- 发送I2C起始条件 --------------------------
uint8_t I2C_Start(void) {


    // 1. 检查总线是否空闲（硬件忙+软件忙）
    if (I2C_IsBusy()) {
        return I2C_ERR_BUSY;
    }

    g_i2cBusy = 1;
    g_i2cErr = I2C_SUCCESS;


    STA = 1;  /// 硬件开始生成起始条件
    SI = 0;
    
    if (I2C_WaitDone(10000) != I2C_SUCCESS) {
        g_i2cErr = I2C_ERR_TIMEOUT;
        g_i2cBusy = 0;
        return g_i2cErr;
    }

    // 4. 检查起始条件状态

    if (SMB0STA != I2C_STA_SENT) {  // 
        g_i2cErr = (SMB0STA == I2C_ARB_LOST) ? I2C_ERR_ARB_LOST : I2C_ERR_NACK;
        // 错误处理：清除STA标志+发送停止条件，复位总线
        STA = 0;
				I2C_Stop();
        g_i2cBusy = 0;			
        return g_i2cErr;
    }

    // 5. 清除SI标志（关键：告知硬件“已处理当前状态”，为后续操作做准备）
    STA = 0;  // 
    return I2C_SUCCESS;
}

// -------------------------- 发送I2C重复起始条件 --------------------------
uint8_t I2C_RepeatStart(void) {
    if (!g_i2cBusy) {
        return I2C_ERR_BUSY;
    }

    STA = 1;  // 硬件开始生成重复起始条件
    SI = 0;
    if (I2C_WaitDone(I2C_TIMEOUT_LONG) != I2C_SUCCESS) {
        g_i2cErr = I2C_ERR_TIMEOUT;
        g_i2cBusy = 0;
        I2C_Stop();
        return g_i2cErr;
    }

    // 4. 检查重复起始条件状态
    if (SMB0STA != I2C_REP_STA_SENT) {
        g_i2cErr = (SMB0STA == I2C_ARB_LOST) ? I2C_ERR_ARB_LOST : I2C_ERR_BUSY;

        I2C_Stop();
        g_i2cBusy = 0;
        return g_i2cErr;
    }
    STA = 0;    // 起始关闭  关键 
    return I2C_SUCCESS;
}

/**-------------------------- 发送I2C停止条件 --------------------------*/
void I2C_Stop(void) {
	 uint32_t timeout;
    if (g_i2cBusy) {
        STO = 1;  // 发送停止条件
			  SI=0;
        timeout = I2C_TIMEOUT_LONG; 
        while (STO && timeout--) {
            ;
        }
        g_i2cBusy = 0;
    }
}

// -------------------------- 发送从设备地址 --------------------------
uint8_t I2C_SendAddr(uint8_t slaAddr) {
    if (!g_i2cBusy) {
        return I2C_ERR_BUSY;
    }
    if (SI == 0) {

        g_i2cErr = I2C_ERR_STATE; 
        I2C_Stop();              
        g_i2cBusy = 0;           
        return g_i2cErr;
    }


    SMB0DAT = slaAddr;  
    SI = 0; // 发送数据字节 
    if (I2C_WaitDone(I2C_TIMEOUT_LONG) != I2C_SUCCESS) {
        g_i2cErr = I2C_ERR_TIMEOUT;

        I2C_Stop();
        g_i2cBusy = 0;
        return g_i2cErr;
    }

    // 检查状态码（收到ACK/未收到ACK）
    switch (SMB0STA) {
        case I2C_SLA_W_ACK:
        case I2C_SLA_R_ACK:
            return I2C_SUCCESS;
        case I2C_SLA_W_NACK:
        case I2C_SLA_R_NACK:
            g_i2cErr = I2C_ERR_NACK;
            break;
        case I2C_ARB_LOST:
            g_i2cErr = I2C_ERR_ARB_LOST;
            break;
        default:
            g_i2cErr = I2C_ERR_BUSY;
            break;
    }

    I2C_Stop();
		g_i2cBusy = 0;
    return g_i2cErr;
}

// -------------------------- 发送1字节数据 --------------------------
uint8_t I2C_SendData(uint8_t send_data) {
    if (!g_i2cBusy) {
        return I2C_ERR_BUSY;
    }			
    if (SI == 0) {

        g_i2cErr = I2C_ERR_STATE; 
        I2C_Stop();              
        g_i2cBusy = 0;           
        return g_i2cErr;
    }


    SMB0DAT = send_data;// 
    SI = 0; // 
    if (I2C_WaitDone(I2C_TIMEOUT_LONG) != I2C_SUCCESS) {
        g_i2cErr = I2C_ERR_TIMEOUT;
        I2C_Stop();
        g_i2cBusy = 0;
        return g_i2cErr;
    }

    // 检查状态码（收到ACK/未收到ACK）
    if (SMB0STA == I2C_DATA_W_ACK) {
        return I2C_SUCCESS;
    } else if (SMB0STA == I2C_DATA_W_NACK) {
        g_i2cErr = I2C_ERR_NACK;
    } else if (SMB0STA == I2C_ARB_LOST) {
        g_i2cErr = I2C_ERR_ARB_LOST;
    } else {
        g_i2cErr = I2C_ERR_STATE;
    }


    I2C_Stop();
		g_i2cBusy = 0;
    return g_i2cErr;
}

// -------------------------- 接收1字节数据 --------------------------
uint8_t I2C_RecvData(uint8_t ack, uint8_t *pData) {
    if (!g_i2cBusy || pData == NULL) {
        return I2C_ERR_PARAM;
    }
    // 配置ACK/NACK
    AA = ack ? 1 : 0;
		SI=0;
    if (I2C_WaitDone(I2C_TIMEOUT_LONG) != I2C_SUCCESS) {
        g_i2cErr = I2C_ERR_TIMEOUT;

        I2C_Stop();
        g_i2cBusy = 0;
        return g_i2cErr;
    }

 
    // 检查状态码（接收成功）
    if (SMB0STA == I2C_DATA_R_ACK || SMB0STA == I2C_DATA_R_NACK) {
        *pData = SMB0DAT;  // ????????????
        return I2C_SUCCESS;
    } else if (SMB0STA == I2C_ARB_LOST) {
        g_i2cErr = I2C_ERR_ARB_LOST;
    } else {
        g_i2cErr = I2C_ERR_STATE;
			
    }


    I2C_Stop();
    g_i2cBusy = 0;
    return g_i2cErr;
}

// -------------------------- 检查I2C总线是否空闲 --------------------------
bit I2C_IsBusy(void) {
    return (BUSY || g_i2cBusy);
}


/**
 * @brief  C8051F120硬件I2C复位SMBus状态机（适配中断执行，无死循环）
 * @retval 0=成功，1=失败（中断中忽略返回值，仅执行复位操作）
 */
uint8_t SMBus_HwI2C_Reset(void) {
	char SFRPAGE_SAVE = SFRPAGE;
    SFRPAGE = SMB0_PAGE;	
    // 复位SMBus接口
    ENSMB = 0;	
 
    ENSMB = 1;			
    // 恢复标志位
	  BUSY=0;
    STA = 0;
    STO = 0;
    AA = 1;
    g_i2cBusy = 0;	
	  SFRPAGE = SFRPAGE_SAVE;

    return 0;
}



// -------------------------- SCL低电平25ms超时中断（定时器3） --------------------------
//void Timer3_ISR(void) interrupt 14 {
//    TF3 = 0;  // 清除中断标志
//SMBus_HwI2C_Reset();
//}
