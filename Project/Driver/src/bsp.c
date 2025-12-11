#include "bsp.h"

















// 系统时钟初始化
void ExternalClock_Init(void)
{
    char SFRPAGE_SAVE = SFRPAGE; // 保存当前SFR页
    SFRPAGE = CONFIG_PAGE;       // 切换到配置页 (0x01)

    // 关键：22.1184MHz 晶振标准配置 (OSCXCN = 0x67)
    OSCXCN = 0x67;               // 0b01100111: 
                                 //   EXTEN=1, XTAL=1, OSCEN=1, XFCN=1
    while (!(OSCXCN & 0x80));    // 等待外部振荡器稳定 (OSCSTB=1)

    CLKSEL = 0x01;               // 选择外部时钟源 (CLKSEL.0=1)

    SFRPAGE = SFRPAGE_SAVE;      // 恢复SFR页
}

// 端口初始化
void Port_Init() {
	

	  char SFRPAGE_SAVE = SFRPAGE; // Save Current SFR page

	  SFRPAGE = CONFIG_PAGE;

    // 2. 配置 P1 口 (控制信号)
    //    P1.0 (RST), P1.1 (CD), P1.2 (CS0), P1.4   (WR) → P1.7   (LVC245 OE)       推挽输出
    P1MDOUT = 0xff;  // 0x16 = 0b00010110 → 设置位 0,1,2,4 为 1 (推挽输出)
    
    // 3. 配置 P2 口 (数据总线 + RD 信号)
    P2MDOUT = 0xFF;   // 0xFF = 0b11111111 → 所有位推挽输出
	

//		XBR0 = 0x06;	//  使能串口0 分配到IO口P0.0 P0.1      04    //    06
//		XBR2 = 0x40;  
	
	    XBR0 = 0x00;  // 禁止所有数字外设引脚
    XBR1 = 0x00;  // 禁止所有数字外设引脚
    XBR2 = 0x40;  // 使能交叉开关，但不分配任何外设功能
    P1=0X00;
    P2 = 0x00;  // 清空数据总线
	  SFRPAGE = SFRPAGE_SAVE;      // 恢复SFR页
}

// 看门狗配置函数 - 设置溢出时间
void Watchdog_Config(unsigned char timeout_setting)
{
    // 设置看门狗超时间隔
    // 超时间隔 = 4^(3+WDTCN[2:0]) × TSYSCLK
    // WDTCN[2:0]的值范围是0-7
    
    // 直接写入超时间隔设置
    // 注意：写入此值不会立即生效，需要喂狗命令才能生效
    WDTCN = timeout_setting & 0x07;  // 只使用低3位设置超时间隔
}

// 看门狗使能函数 - 使能并配置看门狗
void Watchdog_Enable(unsigned char timeout_setting)
{
    // 先配置超时间隔
    Watchdog_Config(timeout_setting);
    
    // 然后使能看门狗（喂狗操作）
    WDTCN = 0xA5;  // 使能并重装载WDT
}

// 看门狗禁用函数
void Watchdog_Disable(void)
{
	  EA=0;
    // 禁用看门狗定时器
    // 需要两步序列：0xDE + 0xAD，在4个系统周期内完成
    WDTCN = 0xDE;  // 第一步
    WDTCN = 0xAD;  // 第二步（必须在4个系统周期内）
	  EA=1;
}

// 看门狗锁定函数 - 永久禁用看门狗
void Watchdog_Lock(void)
{
    // 锁定看门狗禁用功能，永久禁用
    // 一旦执行此操作，无法再启用看门狗
    WDTCN = 0xFF;  // 锁定禁用功能
}

// 喂狗函数 - 重置看门狗计数器
void Watchdog_Feed(void)
{
    // 喂狗操作，重置看门狗计数器
    // 命令：0xA5 - 使能并重装载WDT
    WDTCN = 0xA5;  // 使能并重装载WDT（喂狗）
}









// GPIO配置函数
void GPIO_Config(unsigned char port, unsigned char pin, unsigned char mode) {
    switch(port) {
        case 0:
            if(mode == 1) P0MDOUT |= (1 << pin); // 输出
            else P0MDOUT &= ~(1 << pin);         // 输入
            break;
        case 1:
            if(mode == 1) P1MDOUT |= (1 << pin);
            else P1MDOUT &= ~(1 << pin);
            break;
        case 2:
            if(mode == 1) P2MDOUT |= (1 << pin);
            else P2MDOUT &= ~(1 << pin);
            break;
        case 3:
            if(mode == 1) P3MDOUT |= (1 << pin);
            else P3MDOUT &= ~(1 << pin);
            break;
    }
}

// GPIO输出函数
void GPIO_Write(unsigned char port, unsigned char pin, unsigned char value) {
    switch(port) {
        case 0:
            if(value) P0 |= (1 << pin);
            else P0 &= ~(1 << pin);
            break;
        case 1:
            if(value) P1 |= (1 << pin);
            else P1 &= ~(1 << pin);
            break;
        case 2:
            if(value) P2 |= (1 << pin);
            else P2 &= ~(1 << pin);
            break;
        case 3:
            if(value) P3 |= (1 << pin);
            else P3 &= ~(1 << pin);
            break;
    }
}

// GPIO读取函数
unsigned char GPIO_Read(unsigned char port, unsigned char pin) {
    switch(port) {
        case 0: return (P0 & (1 << pin)) ? 1 : 0;
        case 1: return (P1 & (1 << pin)) ? 1 : 0;
        case 2: return (P2 & (1 << pin)) ? 1 : 0;
        case 3: return (P3 & (1 << pin)) ? 1 : 0;
        default: return 0;
    }
}



void  bsp_int()
{
  ExternalClock_Init();
	 Watchdog_Disable();
  Port_Init();

}