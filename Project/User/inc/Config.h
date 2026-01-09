
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



sbit BEEP=P1^6;//蜂鸣器


sbit LED_RUN  = P3^5;  // 运行指示灯
sbit LED_COM  = P3^6;  // 通讯指示灯
sbit LED_ERR  = P3^7;  // 故障指示灯



#define VERSION "20251118"
#define PASSWORD_DEFAULT 202320
#define PI 3.1415
#define UNUSED(expr) if ((expr) == 0)
	

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
    uint32_t i;  /* 电流值 */	
    uint32_t u;  /* 电压值 */
    uint16_t c;  /* 功率因素 */	

} Sys_Data;
extern Cap_Data  g_cap_data[4];

extern Sys_Data  g_sys_data;


extern unsigned int g_adc_value ;  // 全局ADC值，由中断更新

extern Date_Struct  system_date;
extern Time_Struct  system_time;
extern  SysHard_State  g_hard_state;


extern bit g_adc_data_valid ;

#endif
