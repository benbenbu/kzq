
#include  "uart.h"
#include  "modbus.h"
#include <intrins.h>  // 包含_nop_()

volatile uint8_t rx_buf[RX_BUF_SIZE];   /* 接收缓冲区 */
volatile uint8_t rx_state;              /* 接收状态：0=空闲，1=接收中，2=接收完成 */
volatile uint8_t rx_len;                /* 已接收字节数 */
volatile uint8_t rx_timeout_cnt;        /* 接收超时计数器（10ms递增） */

/* 发送相关 */
volatile uint8_t tx_buf[TX_BUF_SIZE];   /* 发送缓冲区 */
volatile uint8_t tx_len;                /* 待发送字节数 */
volatile uint8_t tx_state;              /* 发送状态：0=空闲，1=发送中 */
volatile uint8_t tx_index;              /* 发送字节索引 */
volatile bit tx_complete_flag=0;

// 串口0初始化函数
/* ==================== UART0初始化函数（动态波特率，C89格式） ==================== */
/* 
 * @brief  UART0初始化（适配2400/4800/9600等波特率）
 * @param  uart_baudrate：波特率（2400/4800/9600等）
 * @note   基于定时器1模式2，SYSCLK需提前定义（如系统时钟，例：11059200UL）
 */
void UART0_Init(uint32_t uart_baudrate) {
    /* C89要求局部变量全部放在函数开头 */
    unsigned char SFRPAGE_SAVE; 
    uint32_t temp;
    uint8_t th1_value;

    /* 1. 保存当前SFR页，避免影响其他模块 */
    SFRPAGE_SAVE = SFRPAGE; 
    SFRPAGE = UART0_PAGE;    

    /* 2. 停止定时器1，避免配置过程中触发 */
    TR1 = 0;

    /* 3. 禁能UART0并配置工作模式 */

    SCON0 |= 0x50;  // 设置SM0=0, SM1=1 (8位UART模式)
    
    /* 4. 动态计算定时器1重载值（核心：根据传入的波特率计算） */
    // 公式：temp = SYSCLK / (12 * 32 * 波特率)
    // 注：SYSCLK需在代码中提前定义（如#define SYSCLK 11059200UL）
    temp = (SYSCLK / (12UL * 32UL * uart_baudrate));
    th1_value = 256 - (uint8_t)temp;
    
    TH1 = th1_value;  // 设置定时器1重载值
//    TL1 = th1_value;  // 设置定时器1初值
    
    /* 5. 配置定时器1为模式2（8位自动重载） */
    TMOD |= 0x20;  // 设置定时器1为模式2
    CKCON |= 0X00;   // 定时器1 使用Fosc/12（22.1184MHz/12=1.8432MHz）    
    /* 6. 启动定时器1 */
    TR1 = 1;   // 启动定时器1
    
    /* 7. 清除串口中断标志，配置SSTA0 */
    RI0 = 0;  // 清除接收中断标志
    TI0 = 0;  // 清除发送中断标志
    SSTA0 = 0x00; // 清标志,使用定时器1 
    
    /* 8. 使能UART0接收 */
    REN0 = 1;  // 允许接收
    ES0=1;


    /* 9. 切换到CONFIG_PAGE配置485 */
    SFRPAGE = CONFIG_PAGE;  
    TX_EN = 0; // 485初始化为接收模式

    /* 10. 恢复SFR页，初始化接收/发送状态 */
    SFRPAGE = SFRPAGE_SAVE;	
    rx_state = 0;
    rx_len = 0;
    rx_timeout_cnt = 0;
    tx_state = 0;
    tx_len = 0;
    tx_index = 0;		
}


// 串口发送一个字节
void UART0_SendByte(uint8_t dat) {
	  unsigned char SFRPAGE_SAVE = SFRPAGE; 

   SFRPAGE = UART0_PAGE; 	 
    SBUF0 = dat;        // 将数据写入发送缓冲区
    while(!TI0);        // 等待发送完成
    TI0 = 0;            // 清除发送中断标志
	 SFRPAGE=SFRPAGE_SAVE; 
}

// 串口发送字符串
void UART0_SendString(const char* str) {
    while(*str) {
        UART0_SendByte(*str++);
    }
}

//// 串口发送十六进制数据
//void UART0_SendHex(uint8_t hex) {
//    uint8_t high = (hex >> 4) & 0x0F;
//    uint8_t low = hex & 0x0F;
//    
//    // 转换为十六进制字符
//    high = (high > 9) ? (high - 10 + 'A') : (high + '0');
//    low = (low > 9) ? (low - 10 + 'A') : (low + '0');
//    
//    UART0_SendByte(high);
//    UART0_SendByte(low);
//}

//// 串口发送整数（十进制）
//void UART0_SendInt(int32_t num) {
//    if(num == 0) {
//        UART0_SendByte('0');
//        return;
//    }
//    
//    if(num < 0) {
//        UART0_SendByte('-');
//        num = -num;
//    }
//    
//    char buffer[12];
//    int i = 0;
//    
//    while(num > 0) {
//        buffer[i++] = (num % 10) + '0';
//        num /= 10;
//    }
//    
//    // 反向输出
//    while(i > 0) {
//        UART0_SendByte(buffer[--i]);
//    }
//}

// 串口发送数据数组
void UART0_SendArray(uint8_t *send_data, uint8_t length) {
    uint8_t i;
    for(i = 0; i < length; i++) {
        UART0_SendByte(send_data[i]);
    }
}

/* 全局发送相关变量（需在文件开头定义，C89格式） */



///* ==================== 启动非阻塞发送（主函数调用） ==================== */
///* 
// * @brief  启动串口非阻塞发送（适配485）
// * @param  buf：待发送数据缓冲区
// * @param  len：待发送长度
// * @return 0-成功，1-失败（正在发送/长度为0）
// */
//uint8_t UART0_Start_Send(uint8_t *buf, uint8_t len) {
//    unsigned char SFRPAGE_SAVE = SFRPAGE; 
//    uint8_t i=0;
//    // 校验：正在发送/长度为0/缓冲区溢出，返回失败
//    if(tx_state == 1 || len == 0 || len > TX_BUF_SIZE) {
//        return 1;
//    }

//    SFRPAGE = UART0_PAGE; 
//    EA = 0;  // 关总中断，避免拷贝数据时被中断打断
//    // 1. 拷贝发送数据到tx_buf
//    for( i=0; i<len; i++) {
//        tx_buf[i] = buf[i];
//    }
//    tx_len = len;
//    tx_index = 0;
//    tx_state = 1;  // 标记为发送中
//    EA = 1;  // 开总中断

//    // 2. 置485为发送模式
//    TX_EN = 1;
//    // 3. 发送第一个字节，触发发送中断
//    SBUF0 = tx_buf[tx_index];

//    SFRPAGE = SFRPAGE_SAVE;
//    return 0;
//}

/* 极简版UART0_Start_Send（零拷贝专用，去掉拷贝循环） */
uint8_t UART0_Start_Send(uint8_t len) {  // 仅传长度，不传buf
    unsigned char SFRPAGE_SAVE = SFRPAGE;

    /* 合法性校验 */
    if(tx_state == 1 || len == 0 || len > TX_BUF_SIZE) {
        return 1;
    }

    SFRPAGE = UART0_PAGE;
    EA = 0;
    tx_len = len;
    tx_index = 0;
    tx_state = 1;
    tx_complete_flag = 0;
    EA = 1;

    /* 切换485+发送第一个字节 */
    TX_EN = 1;
    _nop_();_nop_();_nop_();_nop_();
    SBUF0 = tx_buf[tx_index];

    SFRPAGE = SFRPAGE_SAVE;
    return 0;
}

// 对应修改Send_Response_ZeroCopy的最后一行：
// UART0_Start_Send(len + 2);



/* 串口波特率适配：以9600为例，1位起始+8位数据+1位停止=10位/字节
   200字节发送耗时：200*10/9600 ≈ 208ms，仪表场景完全可接受 */
void UART0_SendBuf_Block(uint8_t *buf, uint8_t len) {
    unsigned char SFRPAGE_SAVE = SFRPAGE; 
    uint8_t i;

    if(len == 0 || buf == NULL) return;

    SFRPAGE = UART0_PAGE; 
    // 1. 切换485为发送模式，延时确保切换完成（关键）
    TX_EN = 1;
    _nop_();_nop_();_nop_();_nop_();  // 多延时几个周期，适配485芯片

    // 2. 阻塞发送所有字节（仅一次循环，无冗余拷贝）
    for(i = 0; i < len; i++) {
        SBUF0 = buf[i];        // 直接发送原缓冲区数据，无拷贝
        while(!TI0);           // 等待单字节发送完成
        TI0 = 0;               // 清标志
    }

    // 3. 发送完成后，延时再切换485为接收（避免最后一字节被截断）
    _nop_();_nop_();_nop_();_nop_();
    TX_EN = 0;

    SFRPAGE = SFRPAGE_SAVE;
}




/* ==================== 串口中断服务函数（接收+非阻塞发送） ==================== */
void UART_ISR(void) interrupt 4 {
    uint8_t temp_data;
    unsigned char SFRPAGE_SAVE = SFRPAGE; 
     
    SFRPAGE = UART0_PAGE;     
    // ========== 接收中断处理（原有逻辑，保留） ==========
    if(RI0) {
        RI0 = 0;  // 清接收标志
        temp_data = SBUF0;  // 读取接收数据
        
        if(rx_state == 1 && rx_len < RX_BUF_SIZE) {
            rx_buf[rx_len] = temp_data;
            rx_len++;
            rx_timeout_cnt = 0;  // 接收数据，重置超时计数器
        } else if(rx_state == 0) {
            // 首次接收数据，启动接收
            rx_state = 1;
            rx_len = 0;
            rx_buf[rx_len] = temp_data;
            rx_len++;
            rx_timeout_cnt = 0;
        }
    }
    
    // ========== 发送中断处理（修正核心逻辑） ==========
    if(TI0) {
        TI0 = 0;  // 清发送标志
        
        // 仅当处于发送状态时，处理下一字节（关键！）
        if(tx_state == 1) {
            tx_index++;  // 索引自增，指向下一字节
            if(tx_index < tx_len) {
                // 还有数据要发，发送下一字节
                SBUF0 = tx_buf[tx_index];
            } else {
                // 发送完成：恢复485为接收模式，重置发送状态
                TX_EN = 0;
                tx_state = 0;
                tx_index = 0;
                tx_len = 0;
							  tx_complete_flag=1;
            }
        }
        // 非发送状态：仅清标志，不做任何操作
    }
    SFRPAGE = SFRPAGE_SAVE;
}
