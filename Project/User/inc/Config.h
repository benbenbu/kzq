
#ifndef __CONFIG_H
#define __CONFIG_H

//========================================================================
//                               主时钟定义
//========================================================================

  #define SYSCLK	 22118400L	//定义主时钟


//========================================================================
//                                头文件
//========================================================================


#include "C8051F120.h"
#include "Type_def.h"

#define VER_MAIN       0x01    // 主版本号（v1.xx）
#define VER_SUB        0x17    // 次版本号（vx.23）
#define HW_VERSION     0x01    // 硬件版本号（V1.0）
#define PARA_VERSION   0x04    // 参数版本号，和烧的默认版本参数一致，默认版本参数04


sbit TF2H= TMR2CN^7;//定时器2标志




sbit CAP1_ON_1=P5^3;//电容1投控制引脚1  
sbit CAP1_ON_2=P7^0;//电容1投控制引脚2
sbit CAP1_OF_1=P5^2;//电容1切控制引脚1  
sbit CAP1_OF_2=P6^7;//电容1切控制引脚2
sbit CAP1_OP_1=P5^1;//电容1保护引脚1    
sbit CAP1_OP_2=P6^6;//电容1保护引脚2



sbit CAP2_ON_1=P5^0;//电容2投控制引脚1  
sbit CAP2_ON_2=P6^5;//电容2投控制引脚2
sbit CAP2_OF_1=P4^7;//电容2切控制引脚1  
sbit CAP2_OF_2=P6^4;//电容2切控制引脚2
sbit CAP2_OP_1=P4^6;//电容2保护引脚1   
sbit CAP2_OP_2=P6^3;//电容2保护引脚2




sbit CAP3_ON_1=P5^4;//电容3投控制引脚1 
sbit CAP3_ON_2=P7^1;//电容3投控制引脚2
sbit CAP3_OF_1=P5^5;//电容3切控制引脚1  
sbit CAP3_OF_2=P7^2;//电容3切控制引脚2
sbit CAP3_OP_1=P5^6;//电容3保护引脚1   
sbit CAP3_OP_2=P7^3;//电容3保护引脚2



sbit CAP4_ON_1=P5^7;//电容4投控制引脚1
sbit CAP4_ON_2=P7^4;//电容4投控制引脚2
sbit CAP4_OF_1=P6^0;//电容4切控制引脚1
sbit CAP4_OF_2=P7^5;//电容4切控制引脚2
sbit CAP4_OP_1=P6^1;//电容4保护引脚1
sbit CAP4_OP_2=P7^6;//电容4保护引脚2


sbit ERR_ON_1=P6^2;//故障控制引脚1
sbit ERR_ON_2=P7^7;//故障控制引脚2



sbit BEEP=P1^6;//蜂鸣器
sbit TX_EN=P0^7;        /* 485发送使能引脚（高电平发送） */


sbit LED_RUN  = P3^5;  // 运行指示灯
sbit LED_COM  = P3^6;  // 通讯指示灯
sbit LED_ERR  = P3^7;  // 故障指示灯

sbit IRQ1 =P4^0;
sbit IRQ2 =P4^1;
sbit IRQ3 =P4^2;
sbit IRQ4 =P4^3;
sbit LVC245_OE = P1^7;//LVC245  OE




//输入检测
sbit I1 =P4^4;  //对应1c
sbit I2 =P4^5;
sbit I3 =P2^0;
sbit I4 =P2^1;
sbit I5 =P2^2;
sbit I6 =P2^3;
sbit I7 =P2^4;
sbit I8 =P2^5;
sbit I9 =P2^6;
sbit I10 =P2^7;
sbit I11 =P1^3;//对应备用

/******输出  13个继电器 + 蜂鸣器+故障灯+通讯灯*******/
#define CAP1_ON    0x0001
#define CAP1_OFF    0x0002
#define C1_ON    0x0004
#define CAP2_ON    0x0008
#define CAP2_OFF    0x0010
#define C2_ON    0x0020
#define CAP3_ON    0x0040
#define CAP3_OFF    0x0080
#define C3_ON    0x0100
#define CAP4_ON   0x0200
#define CAP4_OFF   0x0400
#define C4_ON    0x0800
#define ERR_ON    0x1000
#define BEEP_ON    0x2000
#define ERR_LED    0x4000
#define COM_LED    0x8000



/******输入*******/
#define IO1_STATE    0x0001
#define IO2_STATE    0x0002
#define IO3_STATE    0x0004
#define IO4_STATE    0x0008
#define IO5_STATE    0x0010
#define IO6_STATE    0x0020
#define IO7_STATE    0x0040
#define IO8_STATE    0x0080
#define IO9_STATE    0x0100
#define IO10_STATE   0x0200
#define IO11_STATE   0x0400



#define VERSION "20251118"
#define PASSWORD_DEFAULT 202320
#define PI 3.1415
#define UNUSED(expr) if ((expr) == 0)
	
// ==================== 原有定义完全保留（无需修改） ====================
// ==================== 过流IA/IC区分（g_sys_flag[0]） ====================
#define CAP1_IA_OVER_CURRENT 0x01  // 1路IA过流 （g_sys_flag[0] bit0）
#define CAP2_IA_OVER_CURRENT 0x02  // 2路IA过流 （g_sys_flag[0] bit1）
#define CAP3_IA_OVER_CURRENT 0x04  // 3路IA过流 （g_sys_flag[0] bit2）
#define CAP4_IA_OVER_CURRENT 0x08  // 4路IA过流 （g_sys_flag[0] bit3）
#define CAP1_IC_OVER_CURRENT 0x10  // 1路IC过流 （g_sys_flag[0] bit4）
#define CAP2_IC_OVER_CURRENT 0x20  // 2路IC过流 （g_sys_flag[0] bit5）
#define CAP3_IC_OVER_CURRENT 0x40  // 3路IC过流 （g_sys_flag[0] bit6）
#define CAP4_IC_OVER_CURRENT 0x80  // 4路IC过流 （g_sys_flag[0] bit7）

// ==================== 速断IA/IC区分（g_sys_flag[1]） ====================
#define CAP1_IA_QUICK_CURRENT 0x01 // 1路IA速断 （g_sys_flag[1] bit0）
#define CAP2_IA_QUICK_CURRENT 0x02 // 2路IA速断 （g_sys_flag[1] bit1）
#define CAP3_IA_QUICK_CURRENT 0x04 // 3路IA速断 （g_sys_flag[1] bit2）
#define CAP4_IA_QUICK_CURRENT 0x08 // 4路IA速断 （g_sys_flag[1] bit3）
#define CAP1_IC_QUICK_CURRENT 0x10 // 1路IC速断 （g_sys_flag[1] bit4）
#define CAP2_IC_QUICK_CURRENT 0x20 // 2路IC速断 （g_sys_flag[1] bit5）
#define CAP3_IC_QUICK_CURRENT 0x40 // 3路IC速断 （g_sys_flag[1] bit6）
#define CAP4_IC_QUICK_CURRENT 0x80 // 4路IC速断 （g_sys_flag[1] bit7）

// ==================== 零序+电压+拒投（g_sys_flag[2]） ====================
#define CAP1_ZERO_SEQUENCE   0x01  // 1路零序    （g_sys_flag[2] bit0）
#define CAP2_ZERO_SEQUENCE   0x02  // 2路零序    （g_sys_flag[2] bit1）
#define CAP3_ZERO_SEQUENCE   0x04  // 3路零序    （g_sys_flag[2] bit2）
#define CAP4_ZERO_SEQUENCE   0x08  // 4路零序    （g_sys_flag[2] bit3）
#define SYS_OVER_VOLTAGE     0x10  // 系统过压   （g_sys_flag[2] bit4）
#define SYS_UNDER_VOLTAGE    0x20  // 系统欠压   （g_sys_flag[2] bit5）
#define CAP1_REFUSE_CHARGE   0x40  // 1路拒投    （g_sys_flag[2] bit6）
#define CAP2_REFUSE_CHARGE   0x80  // 2路拒投    （g_sys_flag[2] bit7）

// 第3组（8位）：g_sys_flag[3] （拒投+拒切+外部故障，编号17-24）- 完全保留
#define CAP3_REFUSE_CHARGE   0x01  // 3拒投    （g_sys_flag[3] bit0）
#define CAP4_REFUSE_CHARGE   0x02  // 4拒投    （g_sys_flag[3] bit1）
#define CAP1_REFUSE_DISCHARGE 0x04 // 1拒切    （g_sys_flag[3] bit2）
#define CAP2_REFUSE_DISCHARGE 0x08 // 2拒切    （g_sys_flag[3] bit3）
#define CAP3_REFUSE_DISCHARGE 0x10 // 3拒切    （g_sys_flag[3] bit4）
#define CAP4_REFUSE_DISCHARGE 0x20 // 4拒切    （g_sys_flag[3] bit5）
#define CAP1_EXTERNAL_FAULT  0x40  // 1外部故障（g_sys_flag[3] bit6）
#define CAP2_EXTERNAL_FAULT  0x80  // 2外部故障（g_sys_flag[3] bit7）

// 第4组（8位）：g_sys_flag[4] （外部故障+断电+投入状态，编号25-32）- 完全保留
#define CAP3_EXTERNAL_FAULT  0x01  // 3外部故障（g_sys_flag[4] bit0）
#define CAP4_EXTERNAL_FAULT  0x02  // 4外部故障（g_sys_flag[4] bit1）
#define FRONT_MAIN_POWER_OFF 0x04  // 前级总闸断电（g_sys_flag[4] bit2）
#define CAP1_CHARGE_OK       0x08  // 1投入    （g_sys_flag[4] bit3）
#define CAP2_CHARGE_OK       0x10  // 2投入    （g_sys_flag[4] bit4）
#define CAP3_CHARGE_OK       0x20  // 3投入    （g_sys_flag[4] bit5）
#define CAP4_CHARGE_OK       0x40  // 4投入    （g_sys_flag[4] bit6）
#define CAP1_DISCHARGE_OK    0x80  // 1切除    （g_sys_flag[4] bit7）

// 第5组（8位）：g_sys_flag[5] （切除状态，编号，剩余预留）- 完全保留
#define CAP2_DISCHARGE_OK    0x01  // 2切除    （g_sys_flag[5] bit0）
#define CAP3_DISCHARGE_OK    0x02  // 3切除    （g_sys_flag[5] bit1）
#define CAP4_DISCHARGE_OK    0x04  // 4切除    （g_sys_flag[5] bit2）
#define RESERVED_BIT3        0x08  // 预留bit3
#define RESERVED_BIT4        0x10  // 预留bit4
#define RESERVED_BIT5        0x20  // 预留bit5
#define RESERVED_BIT6        0x40  // 预留bit6
#define RESERVED_BIT7        0x80  // 预留bit7



// -------------------- 单通道全故障掩码（核心，对应你原有的合并判断需求） --------------------
// ==================== 核心：每路专属故障掩码（仅含该路的过流/速断/零序） ====================
// 1路掩码：IA过流+IC过流 + IA速断+IC速断 + 零序
#define CAP1_FAULT_MASK_SYS0  (CAP1_IA_OVER_CURRENT | CAP1_IC_OVER_CURRENT)
#define CAP1_FAULT_MASK_SYS1  (CAP1_IA_QUICK_CURRENT | CAP1_IC_QUICK_CURRENT)
#define CAP1_FAULT_MASK_SYS2  (CAP1_ZERO_SEQUENCE)

// 2路掩码：IA过流+IC过流 + IA速断+IC速断 + 零序
#define CAP2_FAULT_MASK_SYS0  (CAP2_IA_OVER_CURRENT | CAP2_IC_OVER_CURRENT)
#define CAP2_FAULT_MASK_SYS1  (CAP2_IA_QUICK_CURRENT | CAP2_IC_QUICK_CURRENT)
#define CAP2_FAULT_MASK_SYS2  (CAP2_ZERO_SEQUENCE)

// 3路掩码：IA过流+IC过流 + IA速断+IC速断 + 零序
#define CAP3_FAULT_MASK_SYS0  (CAP3_IA_OVER_CURRENT | CAP3_IC_OVER_CURRENT)
#define CAP3_FAULT_MASK_SYS1  (CAP3_IA_QUICK_CURRENT | CAP3_IC_QUICK_CURRENT)
#define CAP3_FAULT_MASK_SYS2  (CAP3_ZERO_SEQUENCE)

// 4路掩码：IA过流+IC过流 + IA速断+IC速断 + 零序
#define CAP4_FAULT_MASK_SYS0  (CAP4_IA_OVER_CURRENT | CAP4_IC_OVER_CURRENT)
#define CAP4_FAULT_MASK_SYS1  (CAP4_IA_QUICK_CURRENT | CAP4_IC_QUICK_CURRENT)
#define CAP4_FAULT_MASK_SYS2  (CAP4_ZERO_SEQUENCE)

// -------------------- 系统电压故障掩码（独立分类） --------------------
#define SYS_VOLTAGE_FAULT_MASK    (SYS_OVER_VOLTAGE | SYS_UNDER_VOLTAGE) // 系统过/欠压

// -------------------- 全通道故障掩码（快速判断是否有任意故障） --------------------
#define ALL_CAP_FAULT_MASK_SYS0   0xFF // g_sys_flag[0]所有过流故障
#define ALL_CAP_FAULT_MASK_SYS1   0xFF // g_sys_flag[1]所有速断故障
#define ALL_CAP_FAULT_MASK_SYS2   0xFF // g_sys_flag[2]所有零序/电压/拒投故障




// ==================== 位操作宏定义（STATE_ON=置1，STATE_OFF=置0） ====================
// 宏说明：
// XXX_STATE_ON ：将对应位设为1（开启/投入/故障/闭合等）
// XXX_STATE_OFF：将对应位设为0（关闭/切除/正常/断开等）

// -------------------------- 低8位（数据1）- 电容状态 --------------------------
// 电容1-4 投运切除状态（bit0~bit3）
#define CAP1_QUIT_STATE_ON()     (g_com_state |= (1 << 0))  // 电容1设为投运（置1）
#define CAP1_QUIT_STATE_OFF()    (g_com_state &= ~(1 << 0)) // 电容1设为切除（置0）
#define CAP2_QUIT_STATE_ON()     (g_com_state |= (1 << 1))  // 电容2设为投运（置1）
#define CAP2_QUIT_STATE_OFF()    (g_com_state &= ~(1 << 1)) // 电容2设为切除（置0）
#define CAP3_QUIT_STATE_ON()     (g_com_state |= (1 << 2))  // 电容3设为投运（置1）
#define CAP3_QUIT_STATE_OFF()    (g_com_state &= ~(1 << 2)) // 电容3设为切除（置0）
#define CAP4_QUIT_STATE_ON()     (g_com_state |= (1 << 3))  // 电容4设为投运（置1）
#define CAP4_QUIT_STATE_OFF()    (g_com_state &= ~(1 << 3)) // 电容4设为切除（置0）

// 电容1-4 故障状态（bit4~bit7）
#define CAP1_ERR_STATE_ON()     (g_com_state |= (1 << 4))  // 电容1设为故障（置1）
#define CAP1_ERR_STATE_OFF()    (g_com_state &= ~(1 << 4)) // 电容1设为正常（置0）
#define CAP2_ERR_STATE_ON()     (g_com_state |= (1 << 5))  // 电容2设为故障（置1）
#define CAP2_ERR_STATE_OFF()    (g_com_state &= ~(1 << 5)) // 电容2设为正常（置0）
#define CAP3_ERR_STATE_ON()     (g_com_state |= (1 << 6))  // 电容3设为故障（置1）
#define CAP3_ERR_STATE_OFF()    (g_com_state &= ~(1 << 6)) // 电容3设为正常（置0）
#define CAP4_ERR_STATE_ON()     (g_com_state |= (1 << 7))  // 电容4设为故障（置1）
#define CAP4_ERR_STATE_OFF()    (g_com_state &= ~(1 << 7)) // 电容4设为正常（置0）

// -------------------------- 高8位（数据2）- 系统/控制状态 --------------------------
// 电容1-4 投入退出状态（bit8~bit11）
#define CAP1_INPUT_STATE_ON()   (g_com_state |= (1 << 8))  // 电容1设为投入（置1）
#define CAP1_INPUT_STATE_OFF()  (g_com_state &= ~(1 << 8)) // 电容1设为退出（置0）
#define CAP2_INPUT_STATE_ON()   (g_com_state |= (1 << 9))  // 电容2设为投入（置1）
#define CAP2_INPUT_STATE_OFF()  (g_com_state &= ~(1 << 9)) // 电容2设为退出（置0）
#define CAP3_INPUT_STATE_ON()   (g_com_state |= (1 << 10)) // 电容3设为投入（置1）
#define CAP3_INPUT_STATE_OFF()  (g_com_state &= ~(1 << 10))// 电容3设为退出（置0）
#define CAP4_INPUT_STATE_ON()   (g_com_state |= (1 << 11)) // 电容4设为投入（置1）
#define CAP4_INPUT_STATE_OFF()  (g_com_state &= ~(1 << 11))// 电容4设为退出（置0）

// 控制器运行状态（bit12+bit13，组合值：00=自动，01=手动，10=调试）
#define CTRL_MODE_AUTO_ON()     (g_com_state &= ~(3 << 12)) // 设为自动模式（00）
#define CTRL_MODE_MANUAL_ON()   (g_com_state = (g_com_state & ~(3 << 12)) | (1 << 12)) // 设为手动模式（01）
#define CTRL_MODE_DEBUG_ON()    (g_com_state = (g_com_state & ~(3 << 12)) | (2 << 12)) // 设为调试模式（10）

// 电容器出线柜开关状态（bit14）
#define CABINET_SWITCH_ON()     (g_com_state |= (1 << 14)) // 开关设为闭合（置1）
#define CABINET_SWITCH_OFF()    (g_com_state &= ~(1 << 14))// 开关设为断开（置0）

// 预留位（bit15）
#define RESERVED_BIT_ON()       (g_com_state |= (1 << 15)) // 预留位置1
#define RESERVED_BIT_OFF()      (g_com_state &= ~(1 << 15))// 预留位置0


/***********************************************
*系统硬件监控结构体
*/







/***********************************************
 * 描述： 用户自定义结构体
 */

typedef struct  {
    uint32_t iic_err_times;  				/* iic错误次数  */
    uint32_t spi_err_times;         /* spi错误次数 */	
		
} SysHard_State;



//计算值一次值
typedef struct  {
    uint32_t ia;  /* 电流值扩大10倍  */
    uint32_t ic;  /* 电流值扩大10倍 */	
    uint32_t uo;  /* 零序电压值 */			
} Cap_Data;

typedef struct  {
    long q;  /* 无功 */
    long p;  /* 有功 */	
    long i;  /* 电流值 */	
    long u;  /* 电压值 */
    long c;  /* 功率因素 */	

} Sys_Data;










typedef union {
    float f_val;          // 浮点数
    unsigned char bytes[sizeof(float)]; // 对应的字节数组
} FloatToBytesUnion;
	
extern uint8_t g_cap_fail_flag[4] ; // 电容1~4对应索引0~3 投切失败标志

extern bit  err_state;//有故障标志
extern volatile bit com_led_flag ;             // 闪灯触发标志
extern volatile uint8_t com_led_timer ;       // 闪灯定时器（10ms级）
extern volatile bit tx_complete_flag;

extern volatile uint16_t  g_com_state;//系统状态

extern volatile uint16_t  g_relay_state;//继电器状态

extern volatile uint16_t g_all_io_state ;  // 存储所有IO的最终状态

extern  volatile  Cap_Data  g_cap_data[4];

extern volatile   Sys_Data  g_sys_data;

extern volatile uint8_t g_sys_flag[6];
extern  volatile uint8_t g_sys_flag_handled[6]; 
extern unsigned int g_adc_value ;  // 全局ADC值，由中断更新

extern Date_Struct  system_date;
extern Time_Struct  system_time;
extern  SysHard_State  g_hard_state;

extern  uint8_t  g_adjust;//校准状态 0：正常  1:在校准
extern bit g_adc_data_valid ;

#endif
