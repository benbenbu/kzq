
#include  "uart.h"
#include  "modbus.h"


extern volatile uint8_t modbus_rx_index ;
// 串口0初始化函数
void UART0_Init() {
	 uint32_t temp;
	 uint8_t th1_value;
	    // 停止定时器1
    TR1 = 0;
    // 1. 禁能UART0
    SCON0 &= ~0x40; // 清除TI0和RI0标志位
    SCON0 |= 0x10;  // 设置SM0=0, SM1=1 (8位UART模式)
    
    // 2. 配置波特率发生器
     temp = (SYSCLK / (12 * 32 * uart_baudrate));
     th1_value = 256 - (uint8_t)temp;
    
    TH1 = th1_value;  // 设置定时器1重载值
    TL1 = th1_value;  // 设置定时器1初值
    
    // 3. 配置定时器1为模式2（8位自动重载）
    TMOD &= ~0xF0; // 清除定时器1模式位
    TMOD |= 0x20;  // 设置定时器1为模式2
    
    // 4. 启动定时器1
    TR1 = 1;   // 启动定时器1
    
    // 5. 配置端口引脚
//    P0MDOUT |= 0x10;  // 设置P0.4为推挽输出
//    P0 |= 0x10;       // 设置P0.4为高电平
//    P0MDIN &= ~0x20;  // 设置P0.5为数字输入
    
    // 6. 使能串口中断
    RI0 = 0;  // 清除接收中断标志
    TI0 = 0;  // 清除发送中断标志
//    RCLK0 = 0; // 使用定时器1作为接收时钟
//    TCLK0 = 0; // 使用定时器1作为发送时钟
		SSTA0=0x10;//清标志,使用定时器1
    
    // 7. 使能UART0接收
    REN0 = 1;  // 允许接收
}



// 串口发送一个字节
void UART0_SendByte(uint8_t dat) {
    SBUF0 = dat;        // 将数据写入发送缓冲区
    while(!TI0);        // 等待发送完成
    TI0 = 0;            // 清除发送中断标志
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

// 串口接收中断服务程序
void UART0_ISR() interrupt 4 {
	uint8_t received_data;
    if(RI0) {           // 接收中断
        RI0 = 0;        // 清除接收中断标志
         received_data = SBUF0;
        
        // 检查缓冲区是否已满
        if(modbus_rx_index < MODBUS_RX_BUF_SIZE) {
            modbus_rx_buffer[modbus_rx_index++] = received_data;
            
            // 重置并启动超时计时器
            Modbus_StopTimeout();
            Modbus_StartTimeout();
        } else {
            // 缓冲区溢出，重置
            modbus_rx_index = 0;
            Modbus_StopTimeout();
        }
    }
    
    if(TI0) {           // 发送中断
        TI0 = 0;        // 清除发送中断标志
    }
}