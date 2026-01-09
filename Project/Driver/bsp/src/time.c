
/*
*********************************************************************************************************
*
*   模块名称 : 定时器
*   文件名称 : 
*   版    本 : 
*   说    明 : 定时器2  10ms作为触发adc，按键扫描  定时器1作为uart0.
*             
*
*********************************************************************************************************
*/



#include "config.h"
#include "time.h"
#include "adc_key.h"



volatile bit flag_timer_10ms = 0;

volatile uint32_t system_time_ms = 0;



//1MS
void Timer0_Init(void) {
    uint8_t SFRPAGE_SAVE = SFRPAGE;
    SFRPAGE = TIMER01_PAGE; 

 
    TMOD |= 0x01;   // 定时器0：16位定时器模式（模式1）
    CKCON =0X00;//定时器0,1 使用sys/12

    TH0 = 0xF8;     // 高8位
    TL0 = 0xCD;     // 低8位


    ET0 = 1;         // 使能定时器0中断
    TR0 = 1;         // 启动定时器0
    SFRPAGE = SFRPAGE_SAVE; // 恢复SFR页面
}

/**
 * @brief  初始化 Timer2，实现每 10ms 定时中断
 *         使用 16 位自动重载模式
 */
void Timer2_Init(void) {
	
   char SFRPAGE_SAVE = SFRPAGE;        // Save Current SFR page
   SFRPAGE = TMR2_PAGE;             // Set SFR page

    // 停止 Timer2
    TMR2CN = 0x00;           // 停止定时器，清所有控制位,定时器模式，自动重装载

    // 设置时钟源：使用 SYSCLK/12（与 Timer0 一致）
    TMR2CF = 0;           //  Timer2 时钟为 SYSCLK/12 ，向上计数

    // 设置重载初值（）
    RCAP2L = 0x00;        // 低字节
    RCAP2H = 0xB8;        // 高字节
	
	  TMR2L=0;
	  TMR2H=0XB8;

    // 清除溢出标志
    TF2     = 0;             // TMR2CN.7
    // 开启中断
    ET2     = 1;             // 允许 Timer2 中断

    // 启动 Timer2：工作在 16 位自动重载定时器模式
    TR2     = 1;             // 启动定时器
		SFRPAGE = SFRPAGE_SAVE;             // Restore SFR page
}

////配置成25 ms 用于scl超时检测

//void Timer3_Init(void) {
//			
//    char SFRPAGE_SAVE = SFRPAGE;        // 保存当前页面

//    SFRPAGE = TMR3_PAGE;                // 切换到 Timer3 页面 (Page 1)

//    // 1. 停止定时器
//    TMR3CN = 0x00;                      // 清除控制位，停止计数

//    // 2. 配置 TMR3CF：设置时钟源为 SYSCLK/12
//    TMR3CF = 0x00;                      // TnM1=0, TnM0=0 → SYSCLK/12
//                                        // TOGn=0, TnOE=0, DCEN=0

//    // 3. 设置自动重载值（25ms @ SYSCLK=22.1184MHz）
//    RCAP3H = 0x4C;                       // 高字节
//    RCAP3L = 0x00;                       // 低字节

//    // 4. 初始化当前计数值（可选）
//    TMR3H = 0x4C;
//    TMR3L = 0x00;

//    // 5. 清除溢出标志
//    TF3 = 0;                            // TMR3CN.7

//    // 6. 使能 Timer3 中断
//    EIE2 |= 0x01;                       // ET3 = 1

//    // 7. 启动定时器（16位自动重载模式），由硬件iic启动
//		    TR3 = 1;                            // TMR3CN.2 = 1

//    // 8. 恢复原始 SFR 页面
//    SFRPAGE = SFRPAGE_SAVE;		
//		
//}
//10ms
void Timer3_Init(void) {
			
    char SFRPAGE_SAVE = SFRPAGE;        // 保存当前页面

    SFRPAGE = TMR3_PAGE;                // 切换到 Timer3 页面 (Page 1)

    // 1. 停止定时器
    TMR3CN = 0x00;                      // 清除控制位，停止计数

    // 2. 配置 TMR3CF：设置时钟源为 SYSCLK/12
    TMR3CF = 0x00;                      // TnM1=0, TnM0=0 → SYSCLK/12
                                        // TOGn=0, TnOE=0, DCEN=0

    // 3. 设置自动重载值（10ms @ SYSCLK=22.1184MHz）
    RCAP3H = 0xb8;                       // 高字节
    RCAP3L = 0x00;                       // 低字节

    // 4. 初始化当前计数值（可选）
    TMR3H = 0xb8;
    TMR3L = 0x00;

    // 5. 清除溢出标志
    TF3 = 0;                            // TMR3CN.7

    // 6. 使能 Timer3 中断
    EIE2 |= 0x01;                       // ET3 = 1

    // 7. 启动定时器（16位自动重载模式），由硬件iic启动
		    TR3 = 1;                            // TMR3CN.2 = 1

    // 8. 恢复原始 SFR 页面
    SFRPAGE = SFRPAGE_SAVE;		
		
}

uint32_t Get_SystemTime(void) {
    uint32_t temp_time;
    uint8_t SFRPAGE_SAVE = SFRPAGE;
    
    EA = 0; // 关闭总中断（仅耗时2条指令，几乎不影响其他中断）
    // 也可仅关闭定时器0中断：ET0 = 0; （更精准，不影响ADC/定时器1中断）
    temp_time = system_time_ms;
    EA = 1; // 恢复总中断

    SFRPAGE = SFRPAGE_SAVE;
    return temp_time;
}

// 定时器0中断服务程序
void Timer0_ISR() interrupt 1 {


    
    TH0 = 0xF8;     // 重装初值（避免计时偏差）
    TL0 = 0xCD;

    TF0 = 0;

    // 核心：1ms计数器加1（仅1条指令，极致精简）
    system_time_ms++; 
}


/**
 * @brief  Timer2 中断服务程序（Vector 5）
 */
void Timer2_ISR(void) interrupt 5 {
    // 必须手动清除 TF2 标志
    TF2 = 0;
	  bsp_KeyScan();
}





// 定时器3中断服务程序
void Timer3_ISR() interrupt 14 {
    // 清除溢出标志
   TF3     = 0;             // TMR3CN.7

    
}


// 停止指定定时器
void Timer_Stop(uint8_t timer_num) {
    if(timer_num >= 5) return;
    
    switch(timer_num) {
        case 0:
            TR0 = 0;    // 停止定时器0
            ET0 = 0;    // 禁止定时器0中断
            break;
        case 1:
            TR1 = 0;    // 停止定时器1
            ET1 = 0;    // 禁止定时器1中断
            break;
        case 2:
            TR2 = 0;    // 停止定时器2
            ET2 = 0;    // 禁止定时器2中断
            break;
        case 3:
            TR3 = 0;    // 停止定时器3
//            ET3 = 0;    // 禁止定时器3中断
				    EIE2&=~0X01;
            break;
        case 4:
            TR4 = 0;    // 停止定时器4
//            ET4 = 0;    // 禁止定时器4中断
		        EIE2&=~0X04;				
            break;
    }
    

}

// 启动指定定时器
void Timer_Start(uint8_t timer_num) {
    if(timer_num >= 5) return;
    
    switch(timer_num) {
        case 0:
            TR0 = 1;    // 启动定时器0
            ET0 = 1;    // 使能定时器0中断
            break;
        case 1:
            TR1 = 1;    // 启动定时器1
            ET1 = 1;    // 使能定时器1中断
            break;
        case 2:
            TR2 = 1;    // 启动定时器2
            ET2 = 1;    // 使能定时器2中断
            break;
        case 3:
            TR3 = 1;    // 启动定时器3
//            ET3 = 1;    // 使能定时器3中断
						EIE2|=0X01;
            break;
        case 4:
            TR4 = 1;    // 启动定时器4
//            ET4 = 1;    // 使能定时器4中断
		        EIE2|=0X04;				
            break;
    }
    

}

