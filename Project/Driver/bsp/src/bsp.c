
#include "config.h"
#include "bsp.h"
#include "spi.h"
#include "i2c.h"
#include "time.h"

//函数声明

void ExternalClock_Init(void);
void Port_Init();
void Watchdog_Disable(void);
void ADC0_Init(void);
void Sys_Reset(void);




void  bsp_int()
{
   ExternalClock_Init();
	 Sys_Reset();
	 Watchdog_Disable();
	 SPI_Init();	
   Port_Init();
	 ADC0_Init();
	 I2C_Init();	
	 Timer0_Init();//系统定时1ms
	 Timer2_Init();//adc用10ms定时器	
//	 Timer3_Init();//iic用25ms定时器


}


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

void Sys_Reset(void)
{
    char SFRPAGE_SAVE = SFRPAGE; // 保存当前SFR页
    SFRPAGE = LEGACY_PAGE;       // 切换到配置页 (0x00)

    RSTSRC=0x02;//选择 VDD 监视器为复位源

    SFRPAGE = SFRPAGE_SAVE;      // 恢复SFR页
}





// 端口初始化
void Port_Init() {
  char SFRPAGE_SAVE = SFRPAGE; // 保存当前SFR页（C8051F多页寄存器机制）
    SFRPAGE = CONFIG_PAGE;       // 切换到配置页（端口/交叉开关配置必须此页）

    // 1. 配置引脚输出方式（PnMDOUT）
    P0MDOUT |= 0x95;  // P0.2(MOSI)、P0.4(SCK) 推挽输出,END输出，TX输出

    P1MDOUT = 0xff;  // P1口控制信号（RST/CD/CS0/WR/OE）=推挽（强驱动）
    P2MDOUT = 0xFF;  // P2口数据总线=推挽（数据传输需强驱动）	
    P3MDOUT |= 0xFF;   // 
    P4MDOUT |= 0xC0;   //
		P4=0X3F;//P46，P47输出，其他输入
    // 2. 配置交叉开关功能映射（XBR0/XBR1）
    XBR0 = 0x07;      // 使能UART0(P0.0/P0.1)、SPI(P0.2/P0.3/P0.4)、IIC(P0.5/P0.6)
    XBR1 = 0x00;      // 无额外外设功能需要映射

    // 3. 最后使能交叉开关（XBARE=1）
    XBR2 = 0x40;      // 

    // 4. 配置端口初始电平（避免上电后引脚电平不确定）
    P0 |= 0x08;        // 设置P0.3 Miso为输入
    P1 = 0X00;        // 控制信号初始低电平（可根据硬件需求调整，如RST需高电平则改为0xFF）
    P2 = 0x00;        // 数据总线初始低电平（清空总线，避免误读）
		P3 = 0X0f;

    SFRPAGE = SFRPAGE_SAVE;      // 恢复原SFR页
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
//	  uint8_t i;
	  EA=0;
    // 禁用看门狗定时器
    // 需要两步序列：0xDE + 0xAD，在4个系统周期内完成
    WDTCN = 0xDE;  // 第一步
    WDTCN = 0xAD;  // 第二步（必须在4个系统周期内）
//	  EA=1;
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




void ADC0_Init(void) {
	
	  char SFRPAGE_SAVE = SFRPAGE; // Save Current SFR page

	  SFRPAGE = ADC0_PAGE;
	
  	AMX0CF = 0X00;   //单独输入
    AMX0SL = 0x00;           // 选择 AIN0.0 ADC0CF
	
	
    REF0CN = 0x03;           // 参考电压来自vref引脚2.4v,使用内部基准电压，启用温度传感器
    ADC0CF = 0x28;           // SAR Clock1.8m   1增益

    // 配置 ADC0 启动方式：Timer2 溢出触发
    ADC0CN = 0x8c;           // AD0EN=1
                             // AD0CM=01 → Timer2 溢出触发
                             // 默认右对齐，AD0TM=0（连续跟踪）

    // 使能 ADC 中断（用于读取结果）
    EIE2   |= 0x02;          // 使能 ADC0 中断

	  EIP2|=0X02;//高优先级
	  SFRPAGE = SFRPAGE_SAVE;      // 恢复SFR页	
}















