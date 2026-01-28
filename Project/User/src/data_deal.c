

#include "config.h"
#include "data_deal.h"
#include "data_save.h"
#include "math.h"
#include "HT7036.h"
#include <intrins.h>  // 包含_nop_()的声明
#include "time.h"




volatile uint8_t g_sys_flag[6]={0};//系统事件状态，
volatile uint16_t  g_relay_state=0;//继电器状态
  
uint8_t  g_adjust=0;//校准状态 0：正常  1:在校准

uint8_t g_cap_fail_flag[4] = {0}; // 电容1~4对应索引0~3 投切失败标志



/* 全局闭锁控制变量（4路电容各一个） */
Cap_Latch_Struct g_cap_latch[4] = {0}; 

/**
 * @brief 系统状态寄存器（16位），volatile确保每次读取最新值（适配硬件状态实时更新）
 * @note 16位结构拆分：
 *       - 低8位（bit0~bit7）：数据1
 *       - 高8位（bit8~bit15）：数据2
 * 位定义明细：
 * -------------------------- 低8位（数据1） --------------------------
 * bit0    ：电容1投运切除状态（0=切除，1=投运）
 * bit1    ：电容2投运切除状态（0=切除，1=投运）
 * bit2    ：电容3投运切除状态（0=切除，1=投运）
 * bit3    ：电容4投运切除状态（0=切除，1=投运）
 * bit4    ：电容1故障状态（0=正常，1=故障）
 * bit5    ：电容2故障状态（0=正常，1=故障）
 * bit6    ：电容3故障状态（0=正常，1=故障）
 * bit7    ：电容4故障状态（0=正常，1=故障）
 * -------------------------- 高8位（数据2） --------------------------
 * bit8    ：电容1投入退出状态（0=退出，1=投入）
 * bit9    ：电容2投入退出状态（0=退出，1=投入）
 * bit10   ：电容3投入退出状态（0=退出，1=投入）
 * bit11   ：电容4投入退出状态（0=退出，1=投入）
 * bit12   ：控制器运行状态bit0（配合bit13组成2位状态）
 * bit13   ：控制器运行状态bit1（bit13+bit12组合：00=自动，01=手动，10=调试）
 * bit14   ：电容器出线柜开关状态（0=断开，1=闭合）
 * bit15   ：预留位（暂未使用）
 */
volatile uint16_t g_com_state = 0; // 系统状态寄存器（初始值0） 

bit  err_state=0;//有故障标志




/* 单位转换宏：适配保护阈值的两位小数（如10.00kV=1000，对应sys_data.u的×100缩放） */
#define VOLT_SCALE      100    // 系统电压u：×100，单位kV（两位小数）
#define CURR_SCALE      10     // 电容电流ia/ic：×10，单位A（一位小数）
#define TIME_BASE       1     // 定时器中断周期：10ms（计时单位匹配）
#define MAX_CAP_GROUP   4      // 最大电容保护组数


#define AD_VOLTAGE  10000      // 
#define AD_CURRENT   500      // 
#define AD_POWER   50000      // 

// 全局状态初始化（C89顺序初始化）
Cap_Switch_State g_cap_switch_state = {
    CMD_NONE,        // pending_cmd
    {0, 0},          // pending_cap_idx
    0,               // pending_cap_count
    0,               // delay_remaining
    0,               // last_switch_time
    0,               // daily_switch_count
    0,               // switch_result
    0,               // feedback_timer
    0,                // cmd_sent_flag
		0
};



/* 全局变量声明 */


volatile Protect_Time_Struct g_pro_time;        // 保护计时变量


/* 函数声明 */
void Protect_Timer_10ms_Handler(void);             // 10ms定时器中断处理（核心计时）
void Relay_Action_Check(void);                      // 继电器动作判断
uint8_t Volt_Check(void);
void calculate_cap_requirement(void);
int8_t cap_switch_control(void);


/**
 * @brief  CRC8校验值计算函数（工业标准多项式）
 * @param  sdata：待校验数据的首地址指针
 * @param  len：待校验数据的长度（字节）
 * @return 计算后的CRC8校验值（uint8_t）
 * @note   多项式：0x31（x^8+x^5+x^4+1），初始值：0xFF，无反码输出
 */
 uint8_t CRC8_Calc(const uint8_t *sdata, uint16_t len) {
    uint8_t crc = CRC8_INIT;
    uint16_t i, j;  /* C89要求：循环变量必须在函数开头声明 */
    for (i = 0; i < len; i++) {
        crc ^= sdata[i];  /* 数据与CRC异或 */
        for (j = 0; j < 8; j++) {
            /* 按位计算CRC：最高位为1则左移并异或多项式，否则仅左移 */
            crc = (crc & 0x80) ? ((crc << 1) ^ CRC8_POLY) : (crc << 1);
        }
    }
    return crc;
}



//========================================================================
// 函数: CRC16_Calc(u8 *pucBuff, u8 unNum)
// 描述: 计算CRC16函数.
// 参数: *p: 要计算的数据指针.
//        n: 要计算的字节数.
// 返回: CRC16值.
// 版本: V1.0, 2022-3-18
//========================================================================


uint16_t Modbus_CRC16(u8 *pucBuff, u8 unNum)
{
    u8 uchCRCHi = 0xFF;
    u8 uchCRCLo = 0xFF;
    u8 uIndex;
unsigned char code auchCRCHi[256] = {

    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40};

unsigned char code auchCRCLo[256] = {
    0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06,
    0x07, 0xC7, 0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD,
    0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
    0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A,
    0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC, 0x14, 0xD4,
    0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
    0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3,
    0xF2, 0x32, 0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4,
    0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
    0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29,
    0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF, 0x2D, 0xED,
    0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
    0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60,
    0x61, 0xA1, 0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67,
    0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
    0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68,
    0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA, 0xBE, 0x7E,
    0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
    0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71,
    0x70, 0xB0, 0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92,
    0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
    0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B,
    0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89, 0x4B, 0x8B,
    0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
    0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42,
    0x43, 0x83, 0x41, 0x81, 0x80, 0x40};
    while (unNum--)
    {
        uIndex = uchCRCHi ^ *pucBuff++;
        uchCRCHi = uchCRCLo ^ auchCRCHi[uIndex];
        uchCRCLo = auchCRCLo[uIndex];
    }
    return (uint16_t)uchCRCHi << 8 | uchCRCLo;
}




void state_deal(void)
{
   uint16_t  a=g_all_io_state;
	 uint8_t  i=0;
	
	 if(a&IO1_STATE)//1组投入
	 {	 
	   g_cap[0].state=1;	   	 
	 }
	 else
	 {	 
	   g_cap[0].state=0;	 		 
	 }
	 if(a&IO2_STATE)//2组投入
	 {	 
	   g_cap[1].state=1;	   	 
	 }
	 else
	 {	 
	   g_cap[1].state=0;	 		 
	 }
	 if(a&IO3_STATE)//3组投入
	 {	 
	   g_cap[2].state=1;	   	 
	 }
	 else
	 {	 
	   g_cap[2].state=0;	 		 
	 }
	 if(a&IO4_STATE)//4组投入
	 {	 
	   g_cap[3].state=1;	   	 
	 }
	 else
	 {	 
	   g_cap[3].state=0;	 		 
	 }	
	
   
	 if(a&IO5_STATE)//1组电容器故障
	 {	 
	   g_cap[0].err=1;
		 if(g_cap[0].state==1)
			 
		 {
			 g_cap[0].state=0;
		 }	
		g_relay_state|=CAP1_OFF;	 
    g_sys_flag[3] |= CAP1_EXTERNAL_FAULT;		
  		 	 		CAP1_ERR_STATE_ON();     	 
	 }
	 else
	 {	 
	   g_cap[0].err=0;
    g_sys_flag[3] &=~ CAP1_EXTERNAL_FAULT;	
 		 	 		CAP1_ERR_STATE_OFF();  		 
	 }
	 if(a&IO6_STATE)//2组电容器故障
	 {
	   g_cap[1].err=1;	 
		 if(g_cap[1].state==1)
		 {
			 g_cap[1].state=0;	 
		 }
		g_relay_state|=(CAP2_OFF);		 
    g_sys_flag[3] |= CAP2_EXTERNAL_FAULT;
  		 	 		CAP2_ERR_STATE_ON();  		 
	 }
	 else
	 {
	   g_cap[1].err=0;	 
	    g_sys_flag[3] &=~ CAP2_EXTERNAL_FAULT;
 		 	 		CAP2_ERR_STATE_OFF();  		 
	 }
	 if(a&IO7_STATE)//3组电容器故障
	 {
	 
	   g_cap[2].err=1;	
		 if(g_cap[2].state==1)
		 {
			 g_cap[2].state=0;	
		 }			 
		g_relay_state|=(CAP3_OFF);	
    g_sys_flag[4] |= CAP3_EXTERNAL_FAULT;		
 		 	 		CAP3_ERR_STATE_ON();  
		 
	 }
	 else
	 {
	   g_cap[2].err=0;	 
	 
    g_sys_flag[4] &=~ CAP3_EXTERNAL_FAULT;	
   	 		CAP3_ERR_STATE_OFF();  		 
	 }
	 if(a&IO8_STATE)//4组电容器故障
	 {
	 
	   g_cap[3].err=1;	
		 if(g_cap[3].state==1)
			 g_cap[3].state=0;		 
		g_relay_state|=(CAP4_OFF);
    g_sys_flag[4] |= CAP4_EXTERNAL_FAULT;	
 		CAP4_ERR_STATE_ON(); 
	 }
	 else
	 {
	 		CAP4_ERR_STATE_OFF();  
	   g_cap[3].err=0;	
    g_sys_flag[4] &=~ CAP4_EXTERNAL_FAULT;			 
	 }

	 if(a&IO9_STATE)//手动
	 {
	   
 		 CTRL_MODE_MANUAL_ON(); 
		 g_adjust_cap.mode=0;
	 }
	 else
	 {
	  if(g_adjust_cap.mode)
			CTRL_MODE_DEBUG_ON();
		else
			 CTRL_MODE_AUTO_ON();
	 
	 }	 
	 
	 
	 
	 if(a&IO10_STATE)//总分闸
	 {
	 
	 
		 for(i=0;i<g_cap_num.cap_num;i++)
		  g_cap[i].state=0; 
		 
		g_relay_state|=(CAP1_OFF|CAP2_OFF|CAP3_OFF|CAP4_OFF);
    g_sys_flag[4] |= FRONT_MAIN_POWER_OFF;	
    CABINET_SWITCH_ON();	 		 
		 
	 }
   else
	 {
    g_sys_flag[4] &= ~FRONT_MAIN_POWER_OFF;		 
	  CABINET_SWITCH_OFF(); 
	 
	 }		 
 
	 
	 
	
}
 uint32_t g_beep_alarm_timer = 0; 
/**
 * @brief 蜂鸣器报警控制（适配1s调用的Timer_Handler，计时单位：秒）
 * @param enable：1=开启报警，0=强制关闭报警
 * @note  报警时长由g_alarm_minutes.value决定：
 *        - 0：无报警（即使enable=1也不响）
 *        - 1~60：对应报警N分钟（自动关闭）
 *        - 61：常开报警（不自动关闭，需手动enable=0关闭）
 */
void Beep_Control(uint8_t enable)
{
    if(enable)
    {

        switch(g_alarm_minutes.value)
        {
            case 0: // 无报警：强制关闭
                g_relay_state &= ~BEEP_ON;
                g_beep_alarm_timer = 0;
                break;

            case 61: // 常开报警：不自动关闭，计时置0标记
                g_relay_state |= BEEP_ON;
                g_beep_alarm_timer = 0; // 0表示常开状态
                break;

            default: // 1~60分钟：转换为【秒】（1分钟=60秒）
                if(g_alarm_minutes.value >= 1 && g_alarm_minutes.value <= 60)
                {
                g_relay_state |= BEEP_ON;
                    // 核心修改：分钟→秒（去掉*1000，适配1s调用）
                    g_beep_alarm_timer = (uint32_t)g_alarm_minutes.value * 60;
                }
                else // 非法值（<0或>61）：按无报警处理
                {
                g_relay_state &= ~BEEP_ON;
                    g_beep_alarm_timer = 0;
                }
                break;
        }
    }
    else
    {
        // 强制关闭报警：无论当前状态，直接停蜂鸣器+清计时
        g_relay_state &= ~BEEP_ON;
        g_beep_alarm_timer = 0;
        g_relay_state &= ~ERR_LED;
    }
}
/**
 * @brief 蜂鸣器报警计时处理（每1秒调用一次，核心适配）
 * @note  1. 仅对“1~60分钟”的定时报警生效，到时长自动关闭；
 *        2. “常开（61）”报警不会自动关闭，需手动调用Beep_Control(0)关闭；
 *        3. 1秒调用一次，每次递减1（计时单位：秒）
 */
void Beep_Timer_Handler(void)
{
    // 仅处理“定时报警”（计时值>0），常开报警（计时值=0且蜂鸣器开）不处理
    if(g_beep_alarm_timer > 0)
    {
        g_beep_alarm_timer--; // 1秒调用一次，每次减1（秒）
        // 计时到0：自动关闭蜂鸣器
        if(g_beep_alarm_timer == 0)
        {
            Beep_Control(0);
        }
    }
    // 常开报警（g_beep_alarm_timer=0且BEEP_PIN=1）：不做任何处理，保持开启
}

volatile uint8_t g_sys_flag_handled[6] = {0}; 
/**
 * @brief 系统事件检测与处理（优化版：不修改g_sys_flag，通过独立标记去重）
 */
void Sys_Event_Detect_Handler(void)
{
    uint8_t i, j;
    uint8_t curr_flag;
    uint8_t handled_flag; // 新增：当前位的处理状态
    Para_Op_Result_E err;
		Event_Type_E event_type;
    uint32_t event_data	;

    // 遍历g_sys_flag的7个字节（共56位）
    for(i = 0; i < 6; i++)
    {
        // 1. 原子读取当前标志位状态（避免中断修改）
        EA = 0;
        curr_flag = g_sys_flag[i];
        handled_flag = g_sys_flag_handled[i]; // 当前处理状态
        EA = 1;

        // 2. 遍历当前字节的8个位
        for(j = 0; j < 8; j++)
        {
            uint8_t bit_mask = (1 << j); // 当前位的掩码
            // 核心判断条件（双重去重）：
            // a. 原标志位当前为1（事件存在）；
            // b. 该位未被标记为“已处理”（避免重复）；
            // c. 可选：增加边沿检测（0→1），进一步确保首次触发才处理
            if((curr_flag & bit_mask) && !(handled_flag & bit_mask))
            {
                // 触发蜂鸣器报警
								  event_type=i*8+j+1;						
							
							  if(event_type<36)//投入切除之外的故障，需要报警
								{
                Beep_Control(1);
								err_state=1;	
								g_relay_state|=ERR_ON;	
			          g_relay_state |= ERR_LED;
								}
								else //需要清除相应标志位
								{
                    switch(event_type)
                    {
                        // ====== g_sys_flag[4] 投/切成功标志 ======
                        case 36: g_sys_flag[4] &= ~CAP1_CHARGE_OK;       break; // CAP1投成功
                        case 37: g_sys_flag[4] &= ~CAP2_CHARGE_OK;       break; // CAP2投成功
                        case 38: g_sys_flag[4] &= ~CAP3_CHARGE_OK;       break; // CAP3投成功
                        case 39: g_sys_flag[4] &= ~CAP4_CHARGE_OK;       break; // CAP4投成功
                        case 40: g_sys_flag[4] &= ~CAP1_DISCHARGE_OK;    break; // CAP1切成功
                        // ====== g_sys_flag[5] 切成功标志 ======
                        case 41: g_sys_flag[5] &= ~CAP2_DISCHARGE_OK;    break; // CAP2切成功
                        case 42: g_sys_flag[5] &= ~CAP3_DISCHARGE_OK;    break; // CAP3切成功
                        case 43: g_sys_flag[5] &= ~CAP4_DISCHARGE_OK;    break; // CAP4切成功
                        default: break; // 预留位无需处理
                    }			
								}
								
switch(event_type)
{
    // 原case 0 → 新case 1：1路IA过流
    case 1: event_data = g_cap_data[0].ia / 1000; break;
    // 原case 1 → 新case 2：2路IA过流
    case 2: event_data = g_cap_data[1].ia / 1000; break;
    // 原case 2 → 新case 3：3路IA过流
    case 3: event_data = g_cap_data[2].ia / 1000; break;
    // 原case 3 → 新case 4：4路IA过流
    case 4: event_data = g_cap_data[3].ia / 1000; break;
    // 原case 4 → 新case 5：1路IC过流
    case 5: event_data = g_cap_data[0].ic / 1000; break;
    // 原case 5 → 新case 6：2路IC过流
    case 6: event_data = g_cap_data[1].ic / 1000; break;
    // 原case 6 → 新case 7：3路IC过流
    case 7: event_data = g_cap_data[2].ic / 1000; break;
    // 原case 7 → 新case 8：4路IC过流
    case 8: event_data = g_cap_data[3].ic / 1000; break;
    // 原case 8 → 新case 9：1路IA速断
    case 9: event_data = g_cap_data[0].ia / 1000; break;
    // 原case 9 → 新case 10：2路IA速断
    case 10: event_data = g_cap_data[1].ia / 1000; break;
    // 原case 10 → 新case 11：3路IA速断
    case 11: event_data = g_cap_data[2].ia / 1000; break;
    // 原case 11 → 新case 12：4路IA速断
    case 12: event_data = g_cap_data[3].ia / 1000; break;
    // 原case 12 → 新case 13：1路IC速断
    case 13: event_data = g_cap_data[0].ic / 1000; break;
    // 原case 13 → 新case 14：2路IC速断
    case 14: event_data = g_cap_data[1].ic / 1000; break;
    // 原case 14 → 新case 15：3路IC速断
    case 15: event_data = g_cap_data[2].ic / 1000; break;
    // 原case 15 → 新case 16：4路IC速断
    case 16: event_data = g_cap_data[3].ic / 1000; break;
    // 原case 16 → 新case 17：1路零序过压
    case 17: event_data = g_cap_data[0].uo ; break;
    // 原case 17 → 新case 18：2路零序过压
    case 18: event_data = g_cap_data[1].uo ; break;
    // 原case 18 → 新case 19：3路零序过压
    case 19: event_data = g_cap_data[2].uo ; break;
    // 原case 19 → 新case 20：4路零序过压
    case 20: event_data = g_cap_data[3].uo ; break;
    // 原case 20 → 新case 21：系统过压
    case 21: event_data = g_sys_data.u; break;
    // 原case 21 → 新case 22：系统欠压（补充原逻辑遗漏的case）
    case 22: event_data = g_sys_data.u; break;
    // 默认值：防止未匹配的event_type导致数据异常
    default: event_data = 0; break;
}
                // ========== 事件类型映射（原有逻辑不变） ==========
//							  if(i<1)
//								{
//								 if(j<4)
//                 event_type = j;
//								 else
//                 event_type = j-4;									 
//								}
//								else if(i<2)
//								{							
//								 if(j<4)
//                 event_type = j+4;
//								 else
//                 event_type = j;								
//								
//								}
//								else
//								{
//								
//                 event_type = j+8*(i-1)+1;								
//								}

                // 3. 事件类型有效 → 写入日志
                if(event_type != EVENT_TYPE_NONE)
                {
                    err = Write_Event_Log(event_type, event_data);
                    if(err != PARA_OP_SUCCESS)
                    {
                        // 可选：FRAM写入失败的容错处理
                    }
                }

                // 4. 关键：标记该位为“已处理”（避免重复写入），不修改g_sys_flag！
                EA = 0;
                g_sys_flag_handled[i] |= bit_mask; // 置位处理状态位
                EA = 1;
            }
            // 5. 重要：当原标志位清零时，同步清零“已处理”标记（故障恢复后可重新检测）
            else if(!(curr_flag & bit_mask) && (handled_flag & bit_mask))
            {
                EA = 0;
                g_sys_flag_handled[i] &= ~bit_mask; // 清零处理状态位
                EA = 1;
            }
        }

        // 更新上一次的标志位状态（用于边沿检测，可选保留）
//        g_sys_flag_prev[i] = curr_flag;
    }
}




/* 复用你所有原有宏定义（CAP1_ON~COM_LED、g_sys_flag[0~5]等，无需修改） */
/* 全局闭锁控制变量（电容投切专用，不影响g_relay_state状态位） */


void relay(void)
{
    /* C89 强制：变量开头声明 */
    uint8_t i;
    uint8_t cap_state;
    uint16_t relay_state; // 临时存储状态位（避免操作中全局变量被修改）


  unsigned char SFRPAGE_SAVE = SFRPAGE; // 保存当前SFR页（C89兼容，char→unsigned char避免符号问题）
    SFRPAGE = CONFIG_PAGE; 
    /* ==================== 第一步：普通继电器（状态位直接同步硬件） ==================== */
    /* 核心逻辑：g_relay_state的位=1 → 引脚高/吸合；位=0 → 引脚低/断开 */
    /* 初始化变量：读取当前继电器状态位（核心：同步状态，不是触发） */
    relay_state = g_relay_state;
    cap_state = 0;
	
// 判断1路是否触发（仅用1路掩码）
if( (g_sys_flag[0] & CAP1_FAULT_MASK_SYS0) || 
    (g_sys_flag[1] & CAP1_FAULT_MASK_SYS1) || 
    (g_sys_flag[2] & CAP1_FAULT_MASK_SYS2) )
{
    // 1路过流(IA/IC)/速断(IA/IC)/零序任意一个触发，执行1路故障逻辑
    g_relay_state|=C1_ON;
}
else
{

    g_relay_state&=~C1_ON;
}

// 判断2路是否触发（仅用2路掩码）
if( (g_sys_flag[0] & CAP2_FAULT_MASK_SYS0) || 
    (g_sys_flag[1] & CAP2_FAULT_MASK_SYS1) || 
    (g_sys_flag[2] & CAP2_FAULT_MASK_SYS2) )
{
    // 2路故障触发，执行2路故障逻辑
      g_relay_state|=C2_ON;  
}
else
{

    g_relay_state&=~C2_ON;
}
// 判断3路是否触发（仅用3路掩码）
if( (g_sys_flag[0] & CAP3_FAULT_MASK_SYS0) || 
    (g_sys_flag[1] & CAP3_FAULT_MASK_SYS1) || 
    (g_sys_flag[2] & CAP3_FAULT_MASK_SYS2) )
{
       g_relay_state|=C3_ON;
}
else
{

    g_relay_state&=~C3_ON;
}
// 判断4路是否触发（仅用4路掩码）
if( (g_sys_flag[0] & CAP4_FAULT_MASK_SYS0) || 
    (g_sys_flag[1] & CAP4_FAULT_MASK_SYS1) || 
    (g_sys_flag[2] & CAP4_FAULT_MASK_SYS2) )
{
       g_relay_state|=C4_ON;
}
else
{

    g_relay_state&=~C4_ON;
}
// 判断过压欠压是否触发	
if( (g_sys_flag[2] & SYS_VOLTAGE_FAULT_MASK) )
{
       g_relay_state|=ERR_ON;
}	

    // ==================== 电容1 继电器控制 ====================
    // 电容1保护（C1_ON：0x0004，匹配CAP1_OP引脚）
    if(g_relay_state & C1_ON)
    {
        CAP1_OP_1 = 1;
        CAP1_OP_2 = 0;
    }
    else
    {
        CAP1_OP_1 = 0;
        CAP1_OP_2 = 0;
    }

 
    // 电容2保护（C2_ON：0x0020，匹配CAP2_OP引脚）
    if(g_relay_state & C2_ON)
    {
        CAP2_OP_1 = 1;
        CAP2_OP_2 = 0;
    }
    else
    {
        CAP2_OP_1 = 0;
        CAP2_OP_2 = 0;
    }

    // 电容3保护（C3_ON：0x0100，匹配CAP3_OP引脚）
    if(g_relay_state & C3_ON)
    {
        CAP3_OP_1 = 1;
        CAP3_OP_2 = 0;
    }
    else
    {
        CAP3_OP_1 = 0;
        CAP3_OP_2 = 0;
    }

    // 电容4保护（C4_ON：0x0800，匹配CAP4_OP引脚）
    if(g_relay_state & C4_ON)
    {
        CAP4_OP_1 = 1;
        CAP4_OP_2 = 0;
    }
    else
    {
        CAP4_OP_1 = 0;
        CAP4_OP_2 = 0;
    }
		
    // ==================== 故障继电器控制 ====================
    // 故障输出（ERR_ON：0x1000）
    if(g_relay_state & ERR_ON)
    {
        ERR_ON_1 = 1;
        ERR_ON_2 = 0;
    }
    else
    {
        ERR_ON_1 = 0;
        ERR_ON_2 = 0;
    }
		
		
    // ==================== 蜂鸣器控制 ====================
    // 蜂鸣器输出（BEEP_ON：0x2000）
    if(g_relay_state & BEEP_ON)
    {
        BEEP = 1;
    }
    else
    {
				BEEP=0;
    }
				
    // ==================== 故障灯控制 ====================
    // 故障灯输出（ERR_LED：0x4000）
    if(g_relay_state & ERR_LED)
    {
        LED_ERR = 0;
    }
    else
    {
				LED_ERR=1;
    }		
    // ==================== 通讯灯控制 ====================
    // 通讯灯输出（COM_LED：08000）
//    if(g_relay_state & COM_LED)
//    {
//        LED_COM = 0;
//    }
//    else
//    {
//				LED_COM=1;
//    }			
//			
	
	
	

    /* ==================== 第二步：电容投切继电器（闭锁+状态位兼容） ==================== */
    /* 核心：电容投切的“闭锁-2秒关断-反馈”逻辑，不修改g_relay_state状态位，仅临时控制硬件 */
    /* 2.1 下发投/切命令（检测g_relay_state的CAPx_ON/OFF位，标记闭锁） */
		
    // CAP1_ON（投1路）
    if((relay_state & CAP1_ON) && (g_cap_latch[0].cmd_state == 0))
    {	

        CAP1_ON_1 = 1; // 动作：置1
        CAP1_ON_2 = 0;  
        // 2. 标记闭锁状态（开始2秒计时）
        g_cap_latch[0].cmd_state = 1;
        g_cap_latch[0].cap_ch = 1;
        g_cap_latch[0].cmd_type = 1; // 投命令
        g_cap_latch[0].timer = 0;
			  CAP1_INPUT_STATE_ON();
    }

    // CAP1_OFF（切1路）
    if((relay_state & CAP1_OFF) && (g_cap_latch[0].cmd_state == 0))
    {
        // 1. 临时硬件输出（切命令）
        CAP1_OF_1 = 1;
        CAP1_OF_2 = 0;
        
        // 2. 标记闭锁状态
        g_cap_latch[0].cmd_state = 1;
        g_cap_latch[0].cap_ch = 1;
        g_cap_latch[0].cmd_type = 2; // 切命令
        g_cap_latch[0].timer = 0;
			  CAP1_QUIT_STATE_ON();
    }

    // CAP2/CAP3/CAP4 ON/OFF 逻辑同上（替换索引和引脚）
    // CAP2_ON（索引1，引脚P2_2）
    if((relay_state & CAP2_ON) && (g_cap_latch[1].cmd_state == 0))
    {
        CAP2_ON_1 = 1;
        CAP2_ON_2 = 0;
        g_cap_latch[1].cmd_state = 1;
        g_cap_latch[1].cap_ch = 2;
        g_cap_latch[1].cmd_type = 1;
        g_cap_latch[1].timer = 0;
						  CAP2_INPUT_STATE_ON();
    }
    // CAP2_OFF（索引1，引脚P2_3）
    if((relay_state & CAP2_OFF) && (g_cap_latch[1].cmd_state == 0))
    {
        CAP2_OF_1 = 1;
        CAP2_OF_2 = 0;
        g_cap_latch[1].cmd_state = 1;
        g_cap_latch[1].cap_ch = 2;
        g_cap_latch[1].cmd_type = 2;
        g_cap_latch[1].timer = 0;
						  CAP2_QUIT_STATE_ON();
    }

    if((relay_state & CAP3_ON) && (g_cap_latch[2].cmd_state == 0))
    {
        CAP3_ON_1 = 1;
        CAP3_ON_2 = 0;
        g_cap_latch[2].cmd_state = 1;
        g_cap_latch[2].cap_ch = 3;
        g_cap_latch[2].cmd_type = 1;
        g_cap_latch[2].timer = 0;
			  CAP3_INPUT_STATE_ON();
    }

    if((relay_state & CAP3_OFF) && (g_cap_latch[2].cmd_state == 0))
    {
        CAP3_OF_1 = 1;
        CAP3_OF_2 = 0;
        g_cap_latch[2].cmd_state = 1;
        g_cap_latch[2].cap_ch = 3;
        g_cap_latch[2].cmd_type = 2;
        g_cap_latch[2].timer = 0;
						  CAP3_QUIT_STATE_ON();

    }
    if((relay_state & CAP4_ON) && (g_cap_latch[3].cmd_state == 0))
    {
        CAP4_ON_1 = 1;
        CAP4_ON_2 = 0;
        g_cap_latch[3].cmd_state = 1;
        g_cap_latch[3].cap_ch = 4;
        g_cap_latch[3].cmd_type = 1;
        g_cap_latch[3].timer = 0;
			  CAP4_INPUT_STATE_ON();			
    }

    if((relay_state & CAP4_OFF) && (g_cap_latch[3].cmd_state == 0))
    {
        CAP4_OF_1 = 1;
        CAP4_OF_2 = 0;
        g_cap_latch[3].cmd_state = 1;
        g_cap_latch[3].cap_ch = 4;
        g_cap_latch[3].cmd_type = 2;
        g_cap_latch[3].timer = 0;
				CAP4_QUIT_STATE_ON();
    }
		
		
		

    /* 2.2 2秒到→关断电容投切继电器（闭锁逻辑） */
    for(i=0; i<4; i++)
    {
        if(g_cap_latch[i].cmd_state == 2) // 2秒计时到，标记为关断
        {
            // 第一步：临时关断硬件引脚（不修改g_relay_state）
            if(g_cap_latch[i].cmd_type == 1) // 投命令→关断投引脚
            {
                if(g_cap_latch[i].cap_ch == 1) {  CAP1_ON_1 = 0; CAP1_ON_2 = 0; CAP1_INPUT_STATE_OFF(); g_relay_state &= ~CAP1_ON;  } // CAP1_ON引脚置低
                if(g_cap_latch[i].cap_ch == 2) { CAP2_ON_1 = 0; CAP2_ON_2 = 0;CAP2_INPUT_STATE_OFF();  g_relay_state &= ~CAP2_ON; } // CAP2_ON引脚置低
                if(g_cap_latch[i].cap_ch == 3) { CAP3_ON_1 = 0; CAP3_ON_2 = 0;CAP3_INPUT_STATE_OFF();  g_relay_state &= ~CAP3_ON; } // CAP3_ON引脚置低
                if(g_cap_latch[i].cap_ch == 4) { CAP4_ON_1 = 0; CAP4_ON_2 = 0; CAP4_INPUT_STATE_OFF(); g_relay_state &= ~CAP4_ON; } // CAP4_ON引脚置低
            }
            else if(g_cap_latch[i].cmd_type == 2) // 切命令→关断切引脚
            {
                if(g_cap_latch[i].cap_ch == 1) { CAP1_OF_1 = 0;CAP1_OF_2 = 0; CAP1_QUIT_STATE_OFF();g_relay_state &= ~CAP1_OFF; } // CAP1_OFF引脚置低
                if(g_cap_latch[i].cap_ch == 2) {CAP2_OF_1 = 0;CAP2_OF_2 = 0; CAP2_QUIT_STATE_OFF();g_relay_state &= ~CAP2_OFF;} // CAP2_OFF引脚置低
                if(g_cap_latch[i].cap_ch == 3) { CAP3_OF_1 = 0;CAP3_OF_2 = 0; CAP3_QUIT_STATE_OFF();g_relay_state &= ~CAP3_OFF; } // CAP3_OFF引脚置低
                if(g_cap_latch[i].cap_ch == 4) { CAP4_OF_1 = 0;CAP4_OF_2 = 0; CAP4_QUIT_STATE_OFF();g_relay_state &= ~CAP4_OFF;} // CAP4_OFF引脚置低
            }

            // 第二步：反馈检测（关断后读取状态）
            switch(g_cap_latch[i].cap_ch)
            {
                case 1: // 1路电容
                    cap_state = (g_all_io_state & IO1_STATE) ? 1 : 0;
                    if(g_cap_latch[i].cmd_type == 1) // 投命令
                    {
                        if(cap_state == 1) { 
												g_sys_flag[4] |= CAP1_CHARGE_OK; g_sys_flag[2] &= ~CAP1_REFUSE_CHARGE;
												  g_cap_fail_flag[0] = 0;	
												}
                        else { g_sys_flag[2] |= CAP1_REFUSE_CHARGE; g_sys_flag[4] &= ~CAP1_CHARGE_OK; 
																								  g_cap_fail_flag[0] = 1;	
												}
                    }
                    else // 切命令
                    {
                        if(cap_state == 0) { g_sys_flag[4] |= CAP1_DISCHARGE_OK; g_sys_flag[3] &= ~CAP1_REFUSE_DISCHARGE; 
												  g_cap_fail_flag[0] = 0;	
												}
                        else { g_sys_flag[3] |= CAP1_REFUSE_DISCHARGE; g_sys_flag[4] &= ~CAP1_DISCHARGE_OK;
												  g_cap_fail_flag[0] = 1;	
												}
                    }
                    break;
                case 2: // 2路电容（同1路逻辑）
                    cap_state = (g_all_io_state & IO2_STATE) ? 1 : 0;
                    if(g_cap_latch[i].cmd_type == 1)
                    {
                        if(cap_state == 1) { g_sys_flag[4] |= CAP2_CHARGE_OK; g_sys_flag[2] &= ~CAP2_REFUSE_CHARGE;  
																								  g_cap_fail_flag[1] = 0;	
												}
                        else { g_sys_flag[2] |= CAP2_REFUSE_CHARGE; g_sys_flag[4] &= ~CAP2_CHARGE_OK; 
																								  g_cap_fail_flag[1] = 1;	
												}
                    }
                    else
                    {
                        if(cap_state == 0) { g_sys_flag[5] |= CAP2_DISCHARGE_OK; g_sys_flag[3] &= ~CAP2_REFUSE_DISCHARGE;
												  g_cap_fail_flag[1] = 0;	
												}
                        else { g_sys_flag[3] |= CAP2_REFUSE_DISCHARGE; g_sys_flag[5] &= ~CAP2_DISCHARGE_OK;
												  g_cap_fail_flag[1] = 0;	
												}
                    }
                    break;
                case 3: // 3路电容
                    cap_state = (g_all_io_state & IO3_STATE) ? 1 : 0;
                    if(g_cap_latch[i].cmd_type == 1)
                    {
                        if(cap_state == 1) { g_sys_flag[4] |= CAP3_CHARGE_OK; g_sys_flag[3] &= ~CAP3_REFUSE_CHARGE;  
																								  g_cap_fail_flag[2] = 0;	
												}
                        else { g_sys_flag[3] |= CAP3_REFUSE_CHARGE; g_sys_flag[4] &= ~CAP3_CHARGE_OK;  
																								  g_cap_fail_flag[2] = 1;	
												}
                    }
                    else
                    {
                        if(cap_state == 0) { g_sys_flag[5] |= CAP3_DISCHARGE_OK; g_sys_flag[3] &= ~CAP3_REFUSE_DISCHARGE; 
		  g_cap_fail_flag[2] = 0;	
												}
                        else { g_sys_flag[3] |= CAP3_REFUSE_DISCHARGE; g_sys_flag[5] &= ~CAP3_DISCHARGE_OK; 
														  g_cap_fail_flag[2] = 1;	
												}
                    }
                    break;									
								
								
                case 4: // 4路电容
                    cap_state = (g_all_io_state & IO4_STATE) ? 1 : 0;
                    if(g_cap_latch[i].cmd_type == 1)
                    {
                        if(cap_state == 1) { g_sys_flag[4] |= CAP4_CHARGE_OK; g_sys_flag[3] &= ~CAP4_REFUSE_CHARGE;  
														  g_cap_fail_flag[3] = 0;	
												}
                        else { g_sys_flag[3] |= CAP4_REFUSE_CHARGE; g_sys_flag[4] &= ~CAP4_CHARGE_OK; 
														  g_cap_fail_flag[3] = 1;	
												}
                    }
                    else
                    {
                        if(cap_state == 0) { g_sys_flag[5] |= CAP4_DISCHARGE_OK; g_sys_flag[3] &= ~CAP4_REFUSE_DISCHARGE; 
																										  g_cap_fail_flag[3] = 0;	
												}
                        else { g_sys_flag[3] |= CAP4_REFUSE_DISCHARGE; g_sys_flag[5] &= ~CAP4_DISCHARGE_OK;
																										  g_cap_fail_flag[3] = 1;	
												}
                    }
                    break;
                    break;
            }

            // 第三步：重置闭锁状态
            g_cap_latch[i].cmd_state = 0;
            g_cap_latch[i].cap_ch = 0;
            g_cap_latch[i].cmd_type = 0;
        }
    }
		    SFRPAGE = SFRPAGE_SAVE; 
		
}






/**
 * @brief  正确的32位有符号×16位无符号硬件乘法（MAC0实现）
 * @param  a: 32位有符号数（如Preg）
 * @param  b: 16位无符号系数（如g_K_W）
 * @retval 32位有符号乘积结果（正确合并高/低16位运算）
 * 核心逻辑：a×b = (a_high×b)<<16 + (a_low×b)，利用MAC0乘累加完成
 */
static int32_t mac0_mul_16x32(int32_t a, uint16_t b) {
    int32_t result = 0;
    uint8_t sign = (a < 0) ? 1 : 0;       // 记录符号位
    uint32_t abs_a = sign ? (uint32_t)-a : (uint32_t)a; // 取绝对值（避免负数运算）
    uint16_t a_low = (uint16_t)(abs_a & 0x0000FFFF);    // 低16位
    uint16_t a_high = (uint16_t)((abs_a >> 16) & 0x0000FFFF); // 高16位
    uint8_t SFRPAGE_SAVE = SFRPAGE;

    // 1. 切换到MAC0页面，初始化MAC0
    SFRPAGE = MAC0_PAGE;
    MAC0CF = 0x09;    // 清零累加器 + 整数模式（MAC0CA=1, MAC0FM=0）
    MAC0STA = 0x00;   // 清零状态位
    MAC0CF = 0x00;    // 切换到乘累加（MAC）模式（关键：累加器保留结果）

    // 2. 第一步：计算低16位×b → 累加器 = a_low×b
    MAC0AH = (uint8_t)((a_low >> 8) & 0xFF); // 加载a_low高8位
    MAC0AL = (uint8_t)(a_low & 0xFF);        // 加载a_low低8位
    MAC0BH = (uint8_t)((b >> 8) & 0xFF);     // 加载b高8位
    MAC0BL = (uint8_t)(b & 0xFF);            // 写BL触发乘法，累加器= a_low×b

    // 3. 第二步：计算高16位×b → 累加器 = a_low×b + a_high×b（MAC0自动累加）
    MAC0AH = (uint8_t)((a_high >> 8) & 0xFF);// 加载a_high高8位
    MAC0AL = (uint8_t)(a_high & 0xFF);       // 加载a_high低8位
    MAC0BL = (uint8_t)(b & 0xFF);            // 重写BL触发乘法，累加器= a_low×b + a_high×b

    // 4. 等待MAC0运算完成（流水线等待，避免读错）
     _nop_();
   _nop_();
  _nop_();

    // 5. 读取40位累加器的低32位（MAC0ACC3~ACC0）
    result  = (uint32_t)MAC0ACC3 << 24;
    result |= (uint32_t)MAC0ACC2 << 16;
    result |= (uint32_t)MAC0ACC1 << 8;
    result |= (uint32_t)MAC0ACC0;

    // 6. 恢复SFR页面，还原符号位
    SFRPAGE = SFRPAGE_SAVE;
    if (sign) {
        result = -result; // 负数则取反
    }

    return result;
}
/**
 * @brief  24位补码寄存器值转32位有符号Preg
 * @param  reg_val: 24位原始寄存器值
 * @retval 32位有符号Preg（匹配你提供的转换规则）
 */
int32_t reg24_to_preg(uint32_t reg_val) {
    const uint32_t UINT23_MAX = 0x00800000UL; // 2^23
    const uint32_t UINT24_MAX = 0x01000000UL; // 2^24
    uint32_t reg_24 = reg_val & 0x00FFFFFF;   // 仅保留24位有效数据

    if (reg_24 >= UINT23_MAX) {
        return (int32_t)(reg_24 - UINT24_MAX); // 负数：PowerP1 - 2^24
    } else {
        return (int32_t)reg_24;                // 正数：直接赋值
    }
}

/* 宏定义：功率因数放大倍数（×1000，对应0~1000，精度0.001） */
#define COS_SCALE    1000
/* 宏定义：开方近似最大迭代次数（平衡精度和速度） */
#define SQRT_ITER    8

/* 整数开方（牛顿迭代法，C89标准，无浮点，适配51） */
static unsigned int int_sqrt(unsigned long num)
{
    unsigned int res;        /* 迭代结果 */
    unsigned int last;       /* 上一次迭代值 */
    unsigned char i;         /* 迭代计数器（C89要求变量开头声明） */

    if (num == 0)
    {
        return 0; /* 避免除0 */
    }

    res = 1;        /* 初始值 */
    last = 0;       /* 初始化上一次迭代值 */

    /* 牛顿迭代法：res = (res + num/res) / 2 */
    for (i = 0; i < SQRT_ITER; i++)
    {
        last = res;
        /* 避免溢出：num/res 拆分为32位÷16位，显式类型转换 */
        res = (res + (unsigned int)(num / res)) / 2;
        if (res == last)
        {
            break; /* 收敛，提前退出 */
        }
    }
    return res;
}

/* 带符号有功/无功计算功率因数（C89标准，纯整数，适配51） */
/* 参数：P-有功功率（带符号），Q-无功功率（带符号） */
/* 返回：功率因数×1000（如0.9876→9876，负数表示超前  和无功） */
int calc_power_factor(long P, long Q)
{
    unsigned long abs_P;     /* 有功功率绝对值 */
    unsigned long abs_Q;     /* 无功功率绝对值 */
    unsigned long P_sq;      /* 有功功率平方 */
    unsigned long Q_sq;      /* 无功功率平方 */
    unsigned long S_sq;      /* 视在功率平方 */
    unsigned int S;          /* 视在功率（开方结果） */
    int cos_val;             /* 功率因数结果（放大10000倍） */

    /* 步骤1：取P/Q的绝对值（避免负数平方，C89显式判断） */
    if (P >= 0)
    {
        abs_P = (unsigned long)P;
    }
    else
    {
        abs_P = (unsigned long)(-P);
    }

    if (Q >= 0)
    {
        abs_Q = (unsigned long)Q;
    }
    else
    {
        abs_Q = (unsigned long)(-Q);
    }

    /* 步骤2：计算P2+Q2（视在功率平方，32位避免溢出） */
    P_sq = abs_P * abs_P;
    Q_sq = abs_Q * abs_Q;
    S_sq = P_sq + Q_sq;

    /* 步骤3：整数开方求视在功率S=√(P2+Q2) */
    S = int_sqrt(S_sq);
    if (S == 0)
    {
        return 0; /* 避免除0 */
    }

    /* 步骤4：计算cosφ = (P/S) × COS_SCALE（整数运算，显式类型转换） */
    cos_val = (int)(((long)abs_P * COS_SCALE) / S);

    /* 步骤5：还原符号（cosφ符号与有功功率P一致） */
    if (Q > 0)
    {
        cos_val = -cos_val;
    }

    /* 步骤6：范围限制（0~10000，避免异常值） */
    if (cos_val > COS_SCALE)
    {
        cos_val = COS_SCALE;
    }
    if (cos_val < -COS_SCALE)
    {
        cos_val = -COS_SCALE;
    }

    return cos_val;
}



/**
 * @brief 计量数据解析（替换乘法器为普通乘法，保留原有逻辑）
 * @param meter_data 原始计量数据（24位）
 * @param type 数据类型：1=无功(Q) 2=有功(P) 3=电压(U) 4=电流(I) 5=功率因数(COS)
 * @param actual_value 输出解析后的实际值（指针）
 */
void meter_data(uint32_t meter_data, uint8_t type, int32_t *actual_value) {
    int32_t preg = reg24_to_preg(meter_data); // 24位转有符号Preg
    float temp = 0;
    long p, q; // C89变量前置声明
    // 初始化输出参数，避免野指针
    if (actual_value == NULL) {
        return;
    }
    *actual_value = 0;

    switch(type) {
        // ========== 1=无功(Q) 2=有功(P)：原始值（W） 扩大了100倍  单位kvar kw==========
        case 1:
        case 2: {
					  if((preg>=0)&&(preg<3))
							preg=0;
					  if((preg<0)&&(preg>-3))
							preg=0;						
            p=g_sys_pt_ct.pt_ratio;
					  p=p* g_sys_pt_ct.ct_ratio;
            if (g_sys_pt_ct.ct_ratio1 == 5) {
                // 1/5 = 2/10 → 先×2（硬件乘法无精度损失），再÷10
                p = p * 2;
                p = p / 10;
            }					  					
            temp = preg * g_meter_chip[3].g_w*p*3/10.0 ;
            p=(long)temp;
						if(type==1)
							p=-p;
            *actual_value = p;
            break;
        }

        // ========== 3=电压(U)：原始值（V）扩大了100倍  单位kv   ==========
        case 3: {
		  					
            temp = preg * g_meter_chip[3].g_u*10;
					  if(((long)temp)<10)
							temp=0;
					
					 temp=temp*g_sys_pt_ct.pt_ratio/100.0 ;
            p=(long)temp;
            *actual_value = p;
            break;
        }

        // ========== 4=电流(I)：原始值（A）扩大了10倍  单位a ==========
        case 4: {
					  
            p=g_sys_pt_ct.ct_ratio;
            if (g_sys_pt_ct.ct_ratio1 == 5) {
                // 1/5 = 2/10 → 先×2（硬件乘法无精度损失），再÷10
                p = p * 2;
                p = p / 10;
            }					  					
            temp = preg * g_meter_chip[3].g_i*100;
					  if(((long)temp)<2)
							temp=0;						
						temp=temp*p/10.0 ;
            p=(long)temp;
            *actual_value = p;
            break;
        }

        // ========== 5=功率因数(COS)：原始值（0~1000） ==========
        case 5: {                    
            p = reg24_to_preg(g_meter_chip[3].rq);
            q = reg24_to_preg(g_meter_chip[3].rp);
            *actual_value = calc_power_factor(p, q);
            break;
        }

        default:
            *actual_value = 0;
            break;
    }
}

/*
 * 功能：计量数据格式化显示（功率自动切换K/M级，电压固定KV）
 * 参数：
 *   dat - meter_data输出的long型actual_value（缩放规则：
 *         Q/P=×100(kvar/kw)、U=×100(kV)、I=×10(A)、COS=0~1000）
 *   type - 数据类型：1=无功(Q) 2=有功(P) 3=电压(U) 4=电流(I) 5=功率因数(COS)
 *   buf - 输出缓冲区（至少16字节）
 * 说明：
 *   1. 功率（P/Q）：actual_value≥100000（即≥1000 kw/kvar）→ M级，数值÷1000；否则K级；
 *   2. 电压固定KV，电流固定A，功率因数固定0.000格式；
 */
void data_disp(long dat, uint8_t type, char* buf) {
    int idx;          // 缓冲区索引
    int i;            // 循环变量
    uint8_t f;        // 符号标志：1=负，0=正
    uint32_t abs_dat;     // 数据绝对值（用于格式化）
    uint8_t is_m_unit;// 功率单位标志：1=M级，0=K级
    long disp_dat;    // 最终用于显示的数值（缩放后）
    long decimal;
	  uint8_t is_overflow;
    uint8_t is_3digit_dec;
    uint8_t is_4digit_int;


    // 1. 初始化缓冲区（清空前16字节）
    for (i = 0; i < 16; i++) {
        buf[i] = '\0';
    }

    // 2. 初始化变量
    idx = 0;
    f = 0;

    is_m_unit = 0;
    disp_dat = 0;

    // 3. 判断符号（仅Q/P有符号，U/I/COS无符号）
    if ((type == 1 || type == 2 || type == 5) && dat < 0) {
        f = 1;
        abs_dat = -dat;  // 取绝对值用于格式化
    }
		else
    abs_dat = dat;
   /* 4. 功率量级判断（仅P/Q）+ 溢出预处理 */
    if (type == 1 || type == 2)
    {
        /* C89：变量初始化在声明后 */
        is_overflow = 0;
        is_3digit_dec = 0;
        is_4digit_int = 0;
        is_m_unit = 0;
        disp_dat = 0;

        if (abs_dat >= 10000) /* abs_dat≥10000 → 实际功率≥100K → 显示M单位 */
        {
            is_m_unit = 1;
            disp_dat = abs_dat; /* 保留原始放大值，拆位时再处理缩放 */

            /* 溢出判断（基于abs_dat的阈值） */
            if (abs_dat > 1000000000UL)
            {
                is_overflow = 1; /* 超过9999MW，固定显示9999M */
            }
            else if (abs_dat > 100000000UL)
            {
							  is_4digit_int = 1; /* 显示9999M */
            }
						else if(abs_dat > 10000000UL)
						{
						
                 is_3digit_dec = 1;	/* 显示999.9M */					
						}
        }
        else /* abs_dat<10000 → 实际功率<100K → 显示K单位 */
        {
            is_m_unit = 0;
            disp_dat = abs_dat; /* 保留放大100倍的K值 */
        }
    }
    else
    {
        /* 非功率类型，disp_dat直接取绝对值 */
        disp_dat = abs_dat;
    }

    /* 5. 按类型格式化显示（C89 switch-case规范） */
    switch(type)
    {
        /* ========== 1=无功(Q)：Q=±00.00 KVar/MVar → 溢出显示±000.0M/±0000M/9999M ========== */
        case 1:
            /* C89：case内定义变量需加{}，此处提前声明val，避免报错 */
            buf[idx++] = 'Q';
            buf[idx++] = '=';
            /* 符号位（- / 空格） */
            buf[idx++] = (f == 1) ? '-' : ' ';

            /* 溢出处理：固定显示9999MVar */
            if (is_overflow)
            {
                buf[idx++] = '9';
                buf[idx++] = '9';
                buf[idx++] = '9';
                buf[idx++] = '9';
                buf[idx++] = 'M';
                buf[idx++] = 'V';
                buf[idx++] = 'a';
                buf[idx++] = 'r';
                buf[idx++] = ' ';	
                buf[idx++] = ' ';								
            }
            /* 99.99MVar < 数值 ≤ 999.9MVar → 000.0MVar格式 */
            else if (is_3digit_dec)
            {
                buf[idx++] = (disp_dat / 10000000UL) % 10UL + '0';
                buf[idx++] = (disp_dat / 1000000UL) % 10UL + '0';
                buf[idx++] = (disp_dat / 100000UL) % 10UL + '0';
                buf[idx++] = '.';
                buf[idx++] = (disp_dat / 10000UL) % 10UL + '0';
                buf[idx++] = ' ';							
                buf[idx++] = 'M';
                buf[idx++] = 'V';
                buf[idx++] = 'a';
                buf[idx++] = 'r';
            }
            /* 999.9MVar < 数值 ≤ 9999MVar → 0000MVar格式 */
            else if (is_4digit_int)
            {
                buf[idx++] = (disp_dat / 100000000UL) % 10UL + '0';
                buf[idx++] = (disp_dat / 10000000UL) % 10UL + '0';
                buf[idx++] = (disp_dat / 1000000UL) % 10UL + '0';
                buf[idx++] = (disp_dat / 100000UL) % 10UL  + '0';
                buf[idx++] = ' ';	
                buf[idx++] = ' ';							
                buf[idx++] = 'M';
                buf[idx++] = 'V';
                buf[idx++] = 'a';
                buf[idx++] = 'r';
            }
            /* 正常范围（≤99.99MVar/KVar）→ 原有00.00格式 */
            else
            {
                if (is_m_unit)
                {
                    /* M单位：拆分为00.00格式 */
                    buf[idx++] = (disp_dat / 100000UL) % 10UL + '0';
                    buf[idx++] = (disp_dat / 10000UL) % 10UL + '0';
                    buf[idx++] = '.';
                    buf[idx++] = (disp_dat / 1000UL) % 10UL + '0';
                    buf[idx++] = (disp_dat / 100UL) % 10UL + '0';
                    buf[idx++] = ' ';
                    buf[idx++] = 'M';
                }
                else
                {
                    /* K单位：拆分为00.00格式 */
                    buf[idx++] = (disp_dat / 1000UL) % 10UL + '0';
                    buf[idx++] = (disp_dat / 100UL) % 10UL + '0';
                    buf[idx++] = '.';
                    buf[idx++] = (disp_dat / 10UL) % 10UL + '0';
                    buf[idx++] = disp_dat % 10UL + '0';
                    buf[idx++] = ' ';
                    buf[idx++] = 'K';
                }
                buf[idx++] = 'V';
                buf[idx++] = 'a';
                buf[idx++] = 'r';
            }
            break;

        /* ========== 2=有功(P)：P=±00.00 KW/MW → 溢出显示±000.0M/±0000M/9999M ========== */
        case 2:
            buf[idx++] = 'P';
            buf[idx++] = '=';
            /* 符号位 */
            buf[idx++] = (f == 1) ? '-' : ' ';

            /* 溢出处理：固定显示9999MW */
            if (is_overflow)
            {
                buf[idx++] = '9';
                buf[idx++] = '9';
                buf[idx++] = '9';
                buf[idx++] = '9';
                buf[idx++] = 'M';
                buf[idx++] = 'W';
                buf[idx++] = ' ';	
                buf[idx++] = ' ';									
							
            }
            /* 99.99MW < 数值 ≤ 999.9MW → 000.0MW格式 */
            else if (is_3digit_dec)
            {

                buf[idx++] = (disp_dat / 10000000UL) % 10UL + '0';
                buf[idx++] = (disp_dat / 1000000UL) % 10UL + '0';
                buf[idx++] = (disp_dat / 100000UL) % 10UL + '0';
                buf[idx++] = '.';
                buf[idx++] = (disp_dat / 10000UL) % 10UL + '0';
                buf[idx++] = ' ';							
                buf[idx++] = 'M';
                buf[idx++] = 'W';
            }
            /* 999.9MW < 数值 ≤ 9999MW → 0000MW格式 */
            else if (is_4digit_int)
            {

                buf[idx++] = (disp_dat / 100000000UL) % 10UL + '0';
                buf[idx++] = (disp_dat / 10000000UL) % 10UL + '0';
                buf[idx++] = (disp_dat / 1000000UL) % 10UL + '0';
                buf[idx++] = (disp_dat / 100000UL) % 10UL  + '0';
                buf[idx++] = ' ';	
                buf[idx++] = ' ';										
                buf[idx++] = 'M';
                buf[idx++] = 'W';
            }
            /* 正常范围（≤99.99MW/KW）→ 原有00.00格式 */
            else
            {
                if (is_m_unit)
                {
                    /* M单位 */
                    buf[idx++] = (disp_dat / 1000000UL) % 10UL + '0';
                    buf[idx++] = (disp_dat / 100000UL) % 10UL + '0';
                    buf[idx++] = '.';
                    buf[idx++] = (disp_dat / 10000UL) % 10UL + '0';
                    buf[idx++] = (disp_dat / 1000UL) % 10UL + '0';
                    buf[idx++] = ' ';
                    buf[idx++] = 'M';
                }
                else
                {
                    /* K单位 */
                    buf[idx++] = (disp_dat / 1000UL) % 10UL + '0';
                    buf[idx++] = (disp_dat / 100UL) % 10UL + '0';
                    buf[idx++] = '.';
                    buf[idx++] = (disp_dat / 10UL) % 10UL + '0';
                    buf[idx++] = disp_dat % 10UL + '0';
                    buf[idx++] = ' ';
                    buf[idx++] = 'K';
                }
                buf[idx++] = 'W';
            }
            break;

        // ========== 3=电压(U)：U=00.00 KV（固定KV） ==========
        case 3: {
            buf[idx++] = 'U';
            buf[idx++] = '=';
            buf[idx++] = ' '; // 无符号位
            // 00.00格式（×100后的KV值）
            buf[idx++] = (disp_dat / 1000) % 10 + '0';
            buf[idx++] = (disp_dat / 100) % 10 + '0';
            buf[idx++] = '.';
            buf[idx++] = (disp_dat / 10) % 10 + '0';
            buf[idx++] = (disp_dat) % 10 + '0';
            // 单位（固定KV）
            buf[idx++] = ' ';
            buf[idx++] = 'K';
            buf[idx++] = 'V';
            break;
        }

        // ========== 4=电流(I)：分三级显示 ==========
        case 4: {
            long show_dat; // 临时显示用数值
            buf[idx++] = 'I';
            buf[idx++] = '=';
            buf[idx++] = ' '; // 无符号位

            // 分级判断（disp_dat=实际电流×10）
            if (disp_dat < 1000) {
                // 区间1：实际电流 < 100.0A → 000.0 A
                buf[idx++] = (disp_dat / 1000) % 10 + '0'; // 百位
                buf[idx++] = (disp_dat / 100) % 10 + '0';  // 十位
                buf[idx++] = (disp_dat / 10) % 10 + '0';   // 个位
                buf[idx++] = '.';
                buf[idx++] = disp_dat % 10 + '0';          // 十分位
                buf[idx++] = ' ';
					
                buf[idx++] = 'A';
                buf[idx++] = ' ';									
            } else if (disp_dat >= 1000 && disp_dat < 9999) {
                // 区间2：100.0A ≤ 实际电流 < 999.9A → 0000 A
                buf[idx++] = (disp_dat / 1000) % 10 + '0'; // 千位（实际电流百位）
                buf[idx++] = (disp_dat / 100) % 10 + '0';  // 百位（实际电流十位）
                buf[idx++] = (disp_dat / 10) % 10 + '0';   // 十位（实际电流个位）
                buf[idx++] = disp_dat % 10 + '0';          // 个位（实际电流十分位）
                buf[idx++] = ' ';
                buf[idx++] = ' ';								
							
                buf[idx++] = 'A';
                buf[idx++] = ' ';

							
            } else {
                // 区间3：实际电流 ≥ 999.9A → 00.00 KA（缩放为KA级）
                show_dat = disp_dat / 10; // 先还原为实际电流（disp_dat=实际×10 → 实际=disp_dat/10）
                show_dat = show_dat / 1000; // 转为KA级（实际电流÷1000）
                // 补充：保留两位小数，需计算到百分位
                 decimal = (disp_dat % 10000) / 100; // 提取百分位数值
                // 00.00格式
                buf[idx++] = (show_dat / 10) % 10 + '0';  // 十位
                buf[idx++] = show_dat % 10 + '0';         // 个位
                buf[idx++] = '.';
                buf[idx++] = (decimal / 10) % 10 + '0';   // 十分位
                buf[idx++] = decimal % 10 + '0';          // 百分位
                buf[idx++] = ' ';
                buf[idx++] = 'K';
                buf[idx++] = 'A';
            }
            break;
        }

        // ========== 5=功率因数(COS)：COS#=0.000 ==========
        case 5: {
            buf[idx++] = 'C';
            buf[idx++] = 'O';
            buf[idx++] = 'S';
            buf[idx++] = '#';
            buf[idx++] = '=';
            buf[idx++] = f ? '-' : ' ';
            // 0.000格式（0~1000）
            buf[idx++] = (disp_dat / 1000) % 10 + '0'; // 整数位（0/1）
            buf[idx++] = '.';
            buf[idx++] = (disp_dat / 100) % 10 + '0';  // 百分位
            buf[idx++] = (disp_dat / 10) % 10 + '0';   // 千分位
            buf[idx++] = disp_dat % 10 + '0';          // 万分位
            break;
        }

        default:
            break;
    }
}


void cap_data_disp(char num,char type,uint32_t calc_val, char unit, char *out_str) {
    // C89要求：所有变量必须在代码块开头声明
    uint8_t unit_char;
    uint32_t val;
    uint8_t integer_part;
    uint8_t decimal_part;
    uint16_t int_val;
    uint32_t temp; // 新增临时变量（C89需提前声明）

    // 入参校验：确保输出字符串指针非空
    if (out_str == (char *)0) { // C89推荐用(char*)0代替NULL
        return;
    }

    val = calc_val;
    int_val = 0; // 初始化（C89建议显式初始化）

    // 确定单位字符（0=电流A，非0=电压V）
    if (unit == 0) {
        unit_char = 'A';
			  out_str[1]='I';
    } else {
        unit_char = 'V';
				out_str[1]='U';
    }

    // 步骤1：根据类型还原实际物理量
    if (unit == 0) {
        // 电流：传入值 = 实际值 × 10000 → 实际值 = 传入值 / 10000
        temp = (val * 10) / 10000;
        // 步骤2：判断格式切换条件（还原后的值 < 100 时用00.0格式）
        if (temp < 1000) { // temp是实际值×10，<1000等价于实际值<100
            integer_part = (uint8_t)(temp / 10);  // 实际值的整数部分
            decimal_part = (uint8_t)(temp % 10);  // 实际值的小数部分
            int_val = 0; // 标记走00.0格式
        } else {
            int_val = (uint16_t)(val / 10000);    // 还原为实际整数值
        }
    } else {
        // 电压：传入值 = 实际值 × 100 → 实际值 = 传入值 / 100
        temp = val; // 传入值本身就是实际值×10
        if (temp >= 10000) {   // temp<1000等价于实际值<100	
        					

            int_val = (uint16_t)(val / 100);    // 还原为实际整数值
				}
				else
				{
            integer_part = (uint8_t)(temp / 100);  // 实际值的整数部分
            decimal_part = (uint8_t)((temp % 100) / 10);  // 实际值的小数部分
            int_val = 0; // 标记走00.0格式
				}
        }
    
     out_str[0]='0'+num;
		 switch(type)
		 {
		   case 1:
				 	out_str[2]='a';
			 break;
			 
			 case 2:
					 out_str[2]='c';			 
			 break;
			 
			 case 3:
					 	out_str[2]='o';			 
			 break;	 
		 }
				 	out_str[3]='=';		

    // 步骤3：拼接输出字符串
    if (int_val == 0) { // 走00.0格式
        out_str[4] = (integer_part / 10) + '0';  // 十位（不足补0）
        out_str[5] = (integer_part % 10) + '0';  // 个位（不足补0）
        out_str[6] = '.';                         // 小数点
        out_str[7] = decimal_part + '0';          // 小数位
        out_str[8] = unit_char;                   // 单位
        out_str[9] = '\0';                        // 字符串结束符
    } else { // 走4位整数格式
        // 范围限制：避免超出4位数字
        if (int_val > 9999) {
            int_val = 9999;
        }
        // 拆分4位数字（不足补0）
        out_str[4] = (int_val / 1000) + '0';      // 千位
        out_str[5] = (int_val / 100 % 10) + '0';  // 百位
        out_str[6] = (int_val / 10 % 10) + '0';   // 十位
        out_str[7] = (int_val % 10) + '0';        // 个位
        out_str[8] = unit_char;                   // 单位
        out_str[9] = '\0';                        // 字符串结束符
    }
}
static uint32_t g_ct_precalc[4] = {0};
static uint32_t g_cap_over_thr[4] = {0};
static uint32_t g_cap_quick_thr[4] = {0};
static uint32_t g_cap_zero_thr[4] = {0};

// ------------- 预计算函数（保留）-------------
void Protect_Param_Precalc(void)
{
    uint8_t i;
    g_ct_precalc[0] = 100 * (uint32_t)g_cap_ratio[0].ct_ratio / g_cap_ratio[0].ct_ratio1;
    g_ct_precalc[1] = 100 * (uint32_t)g_cap_ratio[1].ct_ratio / g_cap_ratio[1].ct_ratio1;
    g_ct_precalc[2] = 100 * (uint32_t)g_cap_ratio[2].ct_ratio / g_cap_ratio[2].ct_ratio1;
    g_ct_precalc[3] = 100 * (uint32_t)g_cap_ratio[3].ct_ratio / g_cap_ratio[3].ct_ratio1;
    
    for (i = 0; i < g_cap_num.pro_num; i++)
    {
        g_cap_over_thr[i] = (uint32_t)g_cap_protect[i].over_value * g_ct_precalc[i];
        g_cap_quick_thr[i] = (uint32_t)g_cap_protect[i].quick_value * g_ct_precalc[i];
        g_cap_zero_thr[i] = (uint32_t)g_cap_protect[i].zero_value ;			
    }
}


void  Data_Get(void)
{
  Cap_Data  cap_data[4];
  Sys_Data  sys_data;	
	
  if(g_adjust==0)
	{		
	Ht7036_read(1);
	if(g_cap_num.cap_num>2)	
	Ht7036_read(2);	
	Ht7036_read(4);
  }
  state_deal();	
	Protect_Param_Precalc();
  if(g_meter_chip[0].data_ready)
	{
	
   cap_data[0].ia=g_meter_chip[0].i_a*1000/8192/6;
	 if(cap_data[0].ia<20)
		cap_data[0].ia=0;
	 else 
		 cap_data[0].ia*=(uint32_t)g_cap_ratio[0].ct_ratio*10/g_cap_ratio[0].ct_ratio1;
		
   cap_data[0].ic=g_meter_chip[0].i_b*1000/8192/6;
	 
	 if(cap_data[0].ic<20)
		cap_data[0].ic=0;
	 else 
		 cap_data[0].ic*=(uint32_t)g_cap_ratio[0].ct_ratio*10/g_cap_ratio[0].ct_ratio1;	 
	 
   cap_data[0].uo=g_meter_chip[0].u_a*100/8192;

	 
   cap_data[1].ia=g_meter_chip[0].i_c*1000/8192/6;
	 
	 if(cap_data[1].ia<20)
		cap_data[1].ia=0;
	 else 
		 cap_data[1].ia*=(uint32_t)g_cap_ratio[1].ct_ratio*10/g_cap_ratio[1].ct_ratio1;	 
	 
   cap_data[1].ic=g_meter_chip[0].u_c*1000/8192/6;
	 if(cap_data[1].ic<20)
		cap_data[1].ic=0;
	 else 
		 cap_data[1].ic*=(uint32_t)g_cap_ratio[1].ct_ratio*10/g_cap_ratio[1].ct_ratio1;	 	 
	 
	
   cap_data[1].uo=g_meter_chip[0].u_b*100/8192;
	 
	 EA=0;
	 
	 g_cap_data[0]=cap_data[0];
	 g_cap_data[1]=cap_data[1];	 
	 EA=1;
	
		
	 g_meter_chip[0].data_ready=0;
	}
	
	
  if(g_meter_chip[1].data_ready)
	{
   cap_data[2].ia=g_meter_chip[0].i_a*1000/8192/6;
	 if(cap_data[2].ia<20)
		cap_data[2].ia=0;
	 else 
		 cap_data[2].ia*=(uint32_t)g_cap_ratio[2].ct_ratio*10/g_cap_ratio[2].ct_ratio1;
		
   cap_data[2].ic=g_meter_chip[1].i_b*1000/8192/6;
	 
	 if(cap_data[2].ic<20)
		cap_data[2].ic=0;
	 else 
		 cap_data[2].ic*=(uint32_t)g_cap_ratio[2].ct_ratio*10/g_cap_ratio[2].ct_ratio1;	 
	 
   cap_data[2].uo=g_meter_chip[1].u_a*100/8192;

	 
   cap_data[3].ia=g_meter_chip[1].i_c*1000/8192/6;
	 
	 if(cap_data[3].ia<20)
		cap_data[3].ia=0;
	 else 
		 cap_data[3].ia*=(uint32_t)g_cap_ratio[3].ct_ratio*10/g_cap_ratio[3].ct_ratio1;	 
	 
   cap_data[3].ic=g_meter_chip[1].u_c*1000/8192/6;
	 if(cap_data[3].ic<20)
		cap_data[3].ic=0;
	 else 
		 cap_data[3].ic*=(uint32_t)g_cap_ratio[3].ct_ratio*10/g_cap_ratio[3].ct_ratio1;	 	 
	 
	
   cap_data[3].uo=g_meter_chip[1].u_b*100/8192;	
	 
	 EA=0;
	 
	 g_cap_data[2]=cap_data[2];
	 g_cap_data[3]=cap_data[3];	 
	 EA=1;
	 
	 g_meter_chip[1].data_ready=0;
	}
  if(g_meter_chip[3].data_ready)//系统计量数据更新
	{
	  meter_data(g_meter_chip[3].rq,2,&sys_data.p);
	  meter_data(g_meter_chip[3].rp,1,&sys_data.q);
	  meter_data(g_meter_chip[3].u_a,3,&sys_data.u);
	  meter_data(g_meter_chip[3].i_a,4,&sys_data.i);
	  meter_data(g_meter_chip[3].rq,5, &sys_data.c);	
		
	 EA=0;	 
	 g_sys_data=sys_data;
	 EA=1;		
		
    calculate_cap_requirement();
	  if(!(g_all_io_state&IO9_STATE)&&(g_adjust_cap.mode==0))	
	  cap_switch_control();	
	  g_meter_chip[3].data_ready=0;	
	}
	  if(!(g_all_io_state&IO9_STATE))	
	     relay();
	Sys_Event_Detect_Handler();
}


/**
 * @brief 10ms定时器中断处理函数（核心：IA/IC分开计时+溢出保护）
 * @note  1. 过流/速断IA、IC独立计时，4组×2通道=8个过流/8个速断计时
 *        2. 计时逻辑完全参照过压/欠压：达标锁死，未达标清零
 */
void Protect_Timer_10ms_Handler(void)
{
    uint8_t i;
    /* 1. 系统过压/欠压计时更新（完全保留你的逻辑） */
    if (g_vol_h.onf && (g_sys_data.u > g_vol_h.value))
    {
        if (g_pro_time.volt_h_time < g_vol_h.time)
        {
            g_pro_time.volt_h_time += TIME_BASE;
            if (g_pro_time.volt_h_time > g_vol_h.time)
            {
                g_pro_time.volt_h_time = g_vol_h.time;
            }
        }
    }
    else
    {
        g_pro_time.volt_h_time = 0;
    }

    if (g_vol_l.onf && (g_sys_data.u < g_vol_l.value)&&(g_sys_data.u > (g_vol_l.value/2)))
    {
        if (g_pro_time.volt_l_time < g_vol_l.time)
        {
            g_pro_time.volt_l_time += TIME_BASE;
            if (g_pro_time.volt_l_time > g_vol_l.time)
            {
                g_pro_time.volt_l_time = g_vol_l.time;
            }
        }
    }
    else
    {
        g_pro_time.volt_l_time = 0;
    }

    /* 2. 电容组保护计时更新（IA/IC分开计时，溢出保护） */
    for (i = 0; i < g_cap_num.pro_num; i++)
    {
        uint16_t over_time_ms = g_cap_protect[i].over_time;  // 该组过流动作时间
        uint16_t quick_time_ms = g_cap_protect[i].quick_time;// 该组速断动作时间
        uint16_t zero_time_ms = g_cap_protect[i].zero_time;  // 该组零序动作时间

        /* -------------------- 过流计时：IA通道独立 -------------------- */
        if (g_cap_protect[i].over_onf && (g_cap_data[i].ia > g_cap_over_thr[i]))
        {
            // 参照过压逻辑：未达动作时间则累加，达到后锁死
            if (g_pro_time.cap_over_time[i][0] < over_time_ms)
            {
                g_pro_time.cap_over_time[i][0] += TIME_BASE;
                if (g_pro_time.cap_over_time[i][0] > over_time_ms)
                {
                    g_pro_time.cap_over_time[i][0] = over_time_ms;
                }
            }
        }
        else
        {
            g_pro_time.cap_over_time[i][0] = 0;  // IA过流不满足，清零计时
        }

        /* -------------------- 过流计时：IC通道独立 -------------------- */
        if (g_cap_protect[i].over_onf && (g_cap_data[i].ic > g_cap_over_thr[i]))
        {
            // 参照过压逻辑：未达动作时间则累加，达到后锁死
            if (g_pro_time.cap_over_time[i][1] < over_time_ms)
            {
                g_pro_time.cap_over_time[i][1] += TIME_BASE;
                if (g_pro_time.cap_over_time[i][1] > over_time_ms)
                {
                    g_pro_time.cap_over_time[i][1] = over_time_ms;
                }
            }
        }
        else
        {
            g_pro_time.cap_over_time[i][1] = 0;  // IC过流不满足，清零计时
        }

        /* -------------------- 速断计时：IA通道独立 -------------------- */
        if (g_cap_protect[i].quick_onf && (g_cap_data[i].ia > g_cap_quick_thr[i]))
        {
            // 参照过压逻辑：未达动作时间则累加，达到后锁死
            if (g_pro_time.cap_quick_time[i][0] < quick_time_ms)
            {
                g_pro_time.cap_quick_time[i][0] += TIME_BASE;
                if (g_pro_time.cap_quick_time[i][0] > quick_time_ms)
                {
                    g_pro_time.cap_quick_time[i][0] = quick_time_ms;
                }
            }
        }
        else
        {
            g_pro_time.cap_quick_time[i][0] = 0;  // IA速断不满足，清零计时
        }

        /* -------------------- 速断计时：IC通道独立 -------------------- */
        if (g_cap_protect[i].quick_onf && (g_cap_data[i].ic > g_cap_quick_thr[i]))
        {
            // 参照过压逻辑：未达动作时间则累加，达到后锁死
            if (g_pro_time.cap_quick_time[i][1] < quick_time_ms)
            {
                g_pro_time.cap_quick_time[i][1] += TIME_BASE;
                if (g_pro_time.cap_quick_time[i][1] > quick_time_ms)
                {
                    g_pro_time.cap_quick_time[i][1] = quick_time_ms;
                }
            }
        }
        else
        {
            g_pro_time.cap_quick_time[i][1] = 0;  // IC速断不满足，清零计时
        }

        /* -------------------- 零序电压计时（保留你的逻辑） -------------------- */
        if (g_cap_protect[i].zero_onf && (g_cap_data[i].uo > g_cap_zero_thr[i]))
        {
            if (g_pro_time.cap_zero_time[i] < zero_time_ms)
            {
                g_pro_time.cap_zero_time[i] += TIME_BASE;
                if (g_pro_time.cap_zero_time[i] > zero_time_ms)
                {
                    g_pro_time.cap_zero_time[i] = zero_time_ms;
                }
            }
        }
        else
        {
            g_pro_time.cap_zero_time[i] = 0;
        }
    }

    /* 3. 判断是否触发继电器动作 */
    Relay_Action_Check();
}

/**
 * @brief 继电器动作判断（核心：速断优先于过流，速断触发则跳过对应过流置位）
 * @note  10ms中断已完成计时累加，本函数仅做“计时达标→置标志→继电器动作”
 */
void Relay_Action_Check(void)
{
    uint8_t i;
    // 新增：标记每一路IA/IC速断是否触发（优先级控制）
    uint8_t ia_quick_triggered, ic_quick_triggered;
//    uint8_t relay=0;
//    uint8_t cap_relay=0;	
    unsigned char SFRPAGE_SAVE = SFRPAGE; 
    SFRPAGE = CONFIG_PAGE;    	
    
    /* 1. 系统过压/欠压动作判断（完全保留你的原始逻辑） */
    if (g_vol_h.onf && (g_pro_time.volt_h_time >= g_vol_h.time))
    {
        g_sys_flag[2] |= SYS_OVER_VOLTAGE;  
//        relay=1;
    }
//    else
//    {
//        g_sys_flag[2]  &= ~SYS_OVER_VOLTAGE;
//    }

    if (g_vol_l.onf && (g_pro_time.volt_l_time >= g_vol_l.time))
    {
        g_sys_flag[2] |= SYS_UNDER_VOLTAGE; 
//        relay=1;
    }
//    else
//    {
//        g_sys_flag[2] &= ~SYS_UNDER_VOLTAGE;
//    }

//    if(relay)
//    {
//       g_relay_state |=ERR_ON;
//    }
//    else
//    {		
//       g_relay_state &=~ERR_ON;			
//    }
		
    /* 2. 电容组保护动作判断（核心：速断优先，触发则跳过对应过流置位） */
    for (i = 0; i < g_cap_num.pro_num; i++)
    {
        // 初始化：默认速断未触发
        ia_quick_triggered = 0;
        ic_quick_triggered = 0;

        /* -------------------- 速断：IA通道（优先判断） -------------------- */
        if (g_cap_protect[i].quick_onf && (g_pro_time.cap_quick_time[i][0] >= g_cap_protect[i].quick_time))
        {
            // IA速断计时达标 → 置标志 + 标记速断触发
            switch(i)
            {
                case 0: g_sys_flag[1] |= CAP1_IA_QUICK_CURRENT; break;
                case 1: g_sys_flag[1] |= CAP2_IA_QUICK_CURRENT; break;
                case 2: g_sys_flag[1] |= CAP3_IA_QUICK_CURRENT; break;
                case 3: g_sys_flag[1] |= CAP4_IA_QUICK_CURRENT; break;
            }
            ia_quick_triggered = 1;  // 标记IA速断触发，后续跳过IA过流
//            cap_relay = 1;
        }
//        else
//        {
//            // IA速断计时未达标/保护关闭 → 清标志
//            switch(i)
//            {
//                case 0: g_sys_flag[1] &= ~CAP1_IA_QUICK_CURRENT; break;
//                case 1: g_sys_flag[1] &= ~CAP2_IA_QUICK_CURRENT; break;
//                case 2: g_sys_flag[1] &= ~CAP3_IA_QUICK_CURRENT; break;
//                case 3: g_sys_flag[1] &= ~CAP4_IA_QUICK_CURRENT; break;
//            }
//        }

        /* -------------------- 速断：IC通道（优先判断） -------------------- */
        if (g_cap_protect[i].quick_onf && (g_pro_time.cap_quick_time[i][1] >= g_cap_protect[i].quick_time))
        {
            // IC速断计时达标 → 置标志 + 标记速断触发
            switch(i)
            {
                case 0: g_sys_flag[1] |= CAP1_IC_QUICK_CURRENT; break;
                case 1: g_sys_flag[1] |= CAP2_IC_QUICK_CURRENT; break;
                case 2: g_sys_flag[1] |= CAP3_IC_QUICK_CURRENT; break;
                case 3: g_sys_flag[1] |= CAP4_IC_QUICK_CURRENT; break;
            }
            ic_quick_triggered = 1;  // 标记IC速断触发，后续跳过IC过流
//            cap_relay = 1;
        }
//        else
//        {
//            // IC速断计时未达标/保护关闭 → 清标志
//            switch(i)
//            {
//                case 0: g_sys_flag[1] &= ~CAP1_IC_QUICK_CURRENT; break;
//                case 1: g_sys_flag[1] &= ~CAP2_IC_QUICK_CURRENT; break;
//                case 2: g_sys_flag[1] &= ~CAP3_IC_QUICK_CURRENT; break;
//                case 3: g_sys_flag[1] &= ~CAP4_IC_QUICK_CURRENT; break;
//            }
//        }

        /* -------------------- 过流：IA通道（速断未触发才处理） -------------------- */
        // 核心：仅当IA速断未触发时，才判断过流是否达标
        if (!ia_quick_triggered && g_cap_protect[i].over_onf && (g_pro_time.cap_over_time[i][0] >= g_cap_protect[i].over_time))
        {
            // IA过流计时达标 → 置标志 + 触发电容继电器
            switch(i)
            {
                case 0: g_sys_flag[0] |= CAP1_IA_OVER_CURRENT; break;
                case 1: g_sys_flag[0] |= CAP2_IA_OVER_CURRENT; break;
                case 2: g_sys_flag[0] |= CAP3_IA_OVER_CURRENT; break;
                case 3: g_sys_flag[0] |= CAP4_IA_OVER_CURRENT; break;
            }
//            cap_relay = 1;
        }
//        else
//        {
//            // IA过流计时未达标/保护关闭/速断已触发 → 清标志
//            switch(i)
//            {
//                case 0: g_sys_flag[0] &= ~CAP1_IA_OVER_CURRENT; break;
//                case 1: g_sys_flag[0] &= ~CAP2_IA_OVER_CURRENT; break;
//                case 2: g_sys_flag[0] &= ~CAP3_IA_OVER_CURRENT; break;
//                case 3: g_sys_flag[0] &= ~CAP4_IA_OVER_CURRENT; break;
//            }
//        }

        /* -------------------- 过流：IC通道（速断未触发才处理） -------------------- */
        // 核心：仅当IC速断未触发时，才判断过流是否达标
        if (!ic_quick_triggered && g_cap_protect[i].over_onf && (g_pro_time.cap_over_time[i][1] >= g_cap_protect[i].over_time))
        {
            // IC过流计时达标 → 置标志 + 触发电容继电器
            switch(i)
            {
                case 0: g_sys_flag[0] |= CAP1_IC_OVER_CURRENT; break;
                case 1: g_sys_flag[0] |= CAP2_IC_OVER_CURRENT; break;
                case 2: g_sys_flag[0] |= CAP3_IC_OVER_CURRENT; break;
                case 3: g_sys_flag[0] |= CAP4_IC_OVER_CURRENT; break;
            }
//            cap_relay = 1;
        }
//        else
//        {
//            // IC过流计时未达标/保护关闭/速断已触发 → 清标志
//            switch(i)
//            {
//                case 0: g_sys_flag[0] &= ~CAP1_IC_OVER_CURRENT; break;
//                case 1: g_sys_flag[0] &= ~CAP2_IC_OVER_CURRENT; break;
//                case 2: g_sys_flag[0] &= ~CAP3_IC_OVER_CURRENT; break;
//                case 3: g_sys_flag[0] &= ~CAP4_IC_OVER_CURRENT; break;
//            }
//        }

        /* -------------------- 零序电压（参照欠压逻辑） -------------------- */
        if (g_cap_protect[i].zero_onf && (g_pro_time.cap_zero_time[i] >= g_cap_protect[i].zero_time))
        {
            // 零序计时达标 → 置标志（需补充零序标志位宏定义） + 触发电容继电器
            switch(i)
            {
                case 0: g_sys_flag[2] |= CAP1_ZERO_SEQUENCE; break; // 示例：零序标志位
                case 1: g_sys_flag[2] |= CAP2_ZERO_SEQUENCE; break;
                case 2: g_sys_flag[2] |= CAP3_ZERO_SEQUENCE; break;
                case 3: g_sys_flag[2] |= CAP4_ZERO_SEQUENCE; break;
            }
//            cap_relay = 1;
        }
//        else
//        {
//            // 零序计时未达标/保护关闭 → 清标志
//            switch(i)
//            {
//                case 0: g_sys_flag[2] &= ~CAP1_ZERO_SEQUENCE; break;
//                case 1: g_sys_flag[2] &= ~CAP2_ZERO_SEQUENCE; break;
//                case 2: g_sys_flag[2] &= ~CAP3_ZERO_SEQUENCE; break;
//                case 3: g_sys_flag[2] &= ~CAP4_ZERO_SEQUENCE; break;
//            }
//        }
				/* 电容继电器动作（和系统继电器逻辑一致） */
//				if(cap_relay)
//				{
//       
//            switch(i)
//            {
//                case 0:        g_relay_state |=C1_ON; break;
//                case 1:        g_relay_state |=C2_ON;  break;
//                case 2:       g_relay_state |=C3_ON;  break;
//                case 3:        g_relay_state |=C4_ON;  break;
//            }
//				}
//				else
//				{
//       
//            switch(i)
//            {
//                case 0:        g_relay_state &=~C1_ON;	break;
//                case 1:        g_relay_state &=~C2_ON;	break;
//                case 2:        g_relay_state &=~C3_ON;	break;
//                case 3:        g_relay_state &=~C4_ON;	break;
//            }
//				}				
				
    }

    SFRPAGE = SFRPAGE_SAVE; 
}
/**
 * @brief 电压有效性检查（区分失压/欠压/正常/过压）
 * @return uint8_t：
 *         1 = 电压正常（投切均可）；
 *         2 = 欠压（失压阈值 < U < 电压下限，禁止切，允许投）；
 *         3 = 过压（U > 电压上限，禁止投，允许切）；
 *         4 = 失压（U ≤ 失压阈值，禁止切，禁止投，优先保护）；
 */
uint8_t Volt_Check(void)
{
    /* 系统电压u：×10存储（如115=11.5kV），与vol_up/vol_down/vol_zero单位一致，直接对比 */
    // 第一步：判断失压（优先级最高）
    if (g_sys_data.u <= 100)
    {
        return 4; // 失压：禁止投+禁止切，优先触发保护
    }
    // 第二步：判断过压
    else if (g_sys_data.u > g_control_para.vol_up*10)
    {
        return 3; // 过压：禁止投，允许切
    }
    // 第三步：判断欠压（非失压+低于下限）
    else if (g_sys_data.u < g_control_para.vol_down*10)
    {
        return 2; // 欠压：禁止切，允许投
    }
    // 第四步：电压正常
    else
    {
        return 1; // 正常：投切均可
    }
}

// 2. 定义Cap_Requirement结构体（原封不动）
typedef struct {
    long qc_min;   /* 最小需投（>0投，<0切） */
    long qc_max;   /* 最大可投（>=0） */
    long qc_best;  /* 推荐值（可负） */
} Cap_Requirement;

// 3. 新增全局的Cap_Requirement变量（核心修改点1）
Cap_Requirement g_cap_requirement; // 全局电容需求变量，替代函数返回值

/* 将目标功率因数转换为目标无功（单位与P/Q一致，×100） */
/* P: 有功（>0，×100），cos100: 1～100，is_lead: 0=滞后, 1=超前 */
static long pf_cos_to_q(long P, unsigned char cos100, unsigned char is_lead)
{
    unsigned int c;
    unsigned int sq;
    unsigned int S;
    long numerator;
    long Q_val;
    
    if (P <= 0L || cos100 == 0U || cos100 > 100U) {
        return 0L;
    }
    if (cos100 >= 100U) {
        return 0L; /* PF=1.0 */
    }
    
    c = (unsigned int)cos100;
    sq = 10000U - c * c;          /* 100^2 - cos^2 */
    S = int_sqrt((unsigned long)sq); /* sqrt(10000 - c^2) */
    
    /* Q = P * S / c */
    /* 假设 P <= 2000000 (20,000kW*100), S<=100 => P*S <= 200,000,000 < 2.15e9 */
    numerator = P * (long)S;
    Q_val = numerator / (long)c;
    
    if (is_lead) {
        Q_val = -Q_val;
    }
    
    return Q_val;
}

// 
void calculate_cap_requirement(void)
{
    long P;
    long Q;
    long Q_upper;
    long Q_lower;
    long Q_range;
    long Q_best;
    
    /* 初始化全局电容需求变量（替代原有局部req的初始化） */
    g_cap_requirement.qc_min = 0L;
    g_cap_requirement.qc_max = 0L;
    g_cap_requirement.qc_best = 0L;
    
    /* 获取系统数据 */
    P = g_sys_data.p/100;   /* ×100，如 10050 = 100.5 kW */
    Q = g_sys_data.q/100;   /* ×100，如 8050 = 80.5 kvar */
    
    /* 仅处理有效负载（P > 0） */
    if (P <= 0L) {
        return; // 直接返回，全局变量已初始化
    }
    
    /* 计算目标Q区间 */
    Q_upper = pf_cos_to_q(P, g_control_para.cos_down, 0U);          /* PF下限 → Q上限 */
    Q_lower = pf_cos_to_q(P, g_control_para.cos_up, g_control_para.cos_up_f); /* PF上限 → Q下限 */
    
    /* 确保 Q_lower <= Q_upper（数值上） */
    if (Q_lower > Q_upper) {
        long tmp = Q_lower;
        Q_lower = Q_upper;
        Q_upper = tmp;
    }
    
    /* 最小需投：若当前Q > Q_upper，至少投 Q - Q_upper */
    if (Q > Q_upper) {
        g_cap_requirement.qc_min = Q - Q_upper;
    } else {
        g_cap_requirement.qc_min = 0L;
    }
    
    /* 最大可投：Qc_max = Q - Q_lower（防止过补偿） */
    g_cap_requirement.qc_max = Q - Q_lower;
    if (g_cap_requirement.qc_max < 0L) {
        g_cap_requirement.qc_max = 0L;
    }
    
    /* 最佳电容：Q_best = Q_lower + (Q_upper - Q_lower) / 3 */
    Q_range = Q_upper - Q_lower;
    if (Q_range <= 0L) {
        Q_best = Q_upper;
    } else {
        Q_best = Q_lower + (Q_range / 3L);
    }
    g_cap_requirement.qc_best = Q - Q_best;
    
    /* 过补偿处理：若 Q < Q_lower，需切除 */
    if (Q < Q_lower) {
        g_cap_requirement.qc_min = Q - Q_upper;   /* 负值 */
        g_cap_requirement.qc_max = 0L;
        g_cap_requirement.qc_best = Q - Q_best;   /* 负值 */
    }
    
    /* 限幅 best 到 [0, qc_max]（若未过补偿） */
    if (Q >= Q_lower) {
        if (g_cap_requirement.qc_best < 0L) {
            g_cap_requirement.qc_best = 0L;
        }
        if (g_cap_requirement.qc_best > g_cap_requirement.qc_max) {
            g_cap_requirement.qc_best = g_cap_requirement.qc_max;
        }
    }
    
    // 无需return，结果已写入全局变量g_cap_requirement
}



/**
 * @brief 筛选电容组（投：未投+启用+无失败标记+无故障；切：已投+启用+无失败标记+无故障）
 * @param is_charge 1=投（筛选未投），0=切（筛选已投）
 * @param out_index 输出符合条件的电容索引数组（1~4）
 * @return 符合条件的电容数量
 * @note 1. 核心筛选条件：启用(onf=1) + 无失败标记(g_cap_fail_flag=0) + 无故障(err=0) + 投切状态匹配
 *       2. 全量顺序遍历（从电容1到电容num），确保无遗漏（解决人工复位后漏掉的问题）
 *       3. 移除环形遍历/静态索引，逻辑更简单，适配人工复位场景
 *       4. 严格遵循C89规范
 */
uint8_t cap_filter(uint8_t is_charge, uint8_t out_index[4])
{
    // C89强制：所有变量在代码块开头声明
    uint8_t count = 0;
    uint8_t num = g_cap_num.cap_num;
    uint8_t curr_idx = 0; // 当前遍历的电容索引（0~3）

    // 初始化输出数组，避免残留旧值导致误判
    for (curr_idx = 0; curr_idx < 4; curr_idx++) {
        out_index[curr_idx] = 0;
    }

    // ========== 全量顺序遍历：从电容1（索引0）到电容num，逐个检查 ==========
    // 重置遍历索引，从第一个电容开始
    curr_idx = 0;
    while (curr_idx < num)
    {
        // 核心筛选条件（完整校验，无指针版，更易理解）
        if (g_cap[curr_idx].onf == 1 &&            // 电容启用
            g_cap_fail_flag[curr_idx] == 0 &&       // 无投切失败标记（人工复位后会置0）
            g_cap[curr_idx].err == 0 &&            // 无硬件故障
            ((is_charge && g_cap[curr_idx].state == 0) || // 投：未投状态
             (!is_charge && g_cap[curr_idx].state == 1))) // 切：已投状态
        {
            // 转换为1~4的电容编号，存入输出数组
            out_index[count++] = curr_idx + 1;
            // 最多筛选4个电容，避免数组越界
            if (count >= 4) {
                break;
            }
        }
        curr_idx++; // 遍历下一个电容
    }

    return count;
}
/**
 * @brief 等容策略选择电容（投：选次数最少；切：选次数最多）【C89版本】
 * @param avail_index 可用电容编号（1~4，对应电容1-4）
 * @param avail_count 可用数量
 * @param is_charge 1=投，0=切
 * @return 选中的电容编号（1~4）；无可用电容/非法值返回0
 * @note 核心修正：适配g_cap索引（0-3）和avail_index编号（1-4）的映射关系
 */
uint8_t cap_select_equal(uint8_t avail_index[], uint8_t avail_count, uint8_t is_charge)
{
    // C89：所有变量前置声明并初始化（避免未定义行为）
    uint8_t select_idx = 0;   // 默认返回0（无选中）
    uint16_t ref_count = 0;   // 参考次数（初始化）
    uint8_t i = 0;
    uint8_t cap_num = 0;      // 电容编号（1~4）
    uint8_t cap_idx = 0;      // g_cap数组索引（0~3）
    uint8_t num = g_cap_num.cap_num; // 实际电容总数（适配非4个的情况）

    // ========== 核心边界条件：无可用电容/空指针，直接返回0 ==========
    if (avail_count == 0 || avail_index == NULL) {
        return 0;
    }

    // ========== 初始化：选第一个有效电容，修复索引映射 ==========
    cap_num = avail_index[0];
    // 校验电容编号有效性（1~num）
    if (cap_num < 1 || cap_num > num) {
        return 0;
    }
    cap_idx = cap_num - 1; // 编号→索引转换（核心修正）
    select_idx = cap_num;  // 选中的是电容编号（1~4），而非索引
    ref_count = g_cap[cap_idx].use_count; // 正确访问g_cap

    // ========== 遍历剩余可用电容，筛选目标（投少切多） ==========
    for (i = 1; i < avail_count; i++) {
        cap_num = avail_index[i];
        // 跳过非法编号，避免数组越界
        if (cap_num < 1 || cap_num > num) {
            continue;
        }
        cap_idx = cap_num - 1; // 编号→索引转换

        if (is_charge) {
            // 投：选投切次数最少的（均衡使用）
            if (g_cap[cap_idx].use_count < ref_count) {
                ref_count = g_cap[cap_idx].use_count;
                select_idx = cap_num; // 始终返回电容编号
            }
        } else {
            // 切：选投切次数最多的（均衡损耗）
            if (g_cap[cap_idx].use_count > ref_count) {
                ref_count = g_cap[cap_idx].use_count;
                select_idx = cap_num; // 始终返回电容编号
            }
        }
    }

    // 最终校验：确保返回值是1~num的有效编号
    return (select_idx >= 1 && select_idx <= num) ? select_idx : 0;
}
/**
 * @brief 差容策略选择电容（选最接近目标容量的）【C89版本】
 * @param avail_index 可用电容编号（1~4，对应电容1-4）
 * @param avail_count 可用数量
 * @param target 目标容量（×100）
 * @param is_charge 1=投，0=切
 * @return 选中的电容编号（1~4）；无可用电容返回0（新增边界处理）
 * @note 关键适配：avail_index是1-4，g_cap索引是0-3，需转换为idx-1访问
 */
uint8_t cap_select_diff(uint8_t avail_index[], uint8_t avail_count, long target, uint8_t is_charge)
{
    // C89：变量全部前置声明并初始化（避免未定义行为）
    uint8_t select_idx = 0;  // 默认返回0（无选中）
    long min_diff = 0;
    uint8_t i = 0;
    uint8_t cap_num = 0;     // 电容编号（1~4）
    uint8_t cap_idx = 0;     // g_cap数组索引（0~3）
    long diff = 0;

    // ========== 核心边界条件：无可用电容/空指针，直接返回0 ==========
    if (avail_count == 0 || avail_index == NULL) {
        return 0;
    }

    // ========== 初始化：选第一个可用电容，计算初始差值（修复索引匹配） ==========
    cap_num = avail_index[0];          // 取第一个电容编号（1~4）
    if (cap_num < 1 || cap_num > 4) {  // 校验编号有效性
        return 0;
    }
    cap_idx = cap_num - 1;             // 转换为g_cap索引（0~3）
    select_idx = cap_num;              // 选中的是电容编号（1~4）
    min_diff = labs(g_cap[cap_idx].value  - target); // 修正索引访问

    // ========== 遍历剩余可用电容，找差值最小的（修复索引匹配） ==========
    for (i = 1; i < avail_count; i++) {
        cap_num = avail_index[i];
        if (cap_num < 1 || cap_num > 4) { // 跳过无效编号
            continue;
        }
        cap_idx = cap_num - 1; // 编号→索引转换（核心修正）
        diff = labs(g_cap[cap_idx].value- target);
        
        if (diff < min_diff) {
            min_diff = diff;
            select_idx = cap_num; // 始终返回电容编号（1~4）
        }
    }

    return select_idx;
}
#include <limits.h> // 引入LONG_MAX的定义（需确保编译器支持）

/**
 * @brief 组合策略选择两个电容（最接近目标容量的组合）【C89版本】
 * @param avail_index 可用电容编号（1~4，对应电容1-4）
 * @param avail_count 可用数量
 * @param target 目标容量（×100）
 * @param idx1 输出第一个选中的电容编号（1~4）
 * @param idx2 输出第二个选中的电容编号（1~4）
 * @return 1=选到两个有效电容，-2=不足两个可用，-1=空指针/非法参数
 * @note 核心修正：适配g_cap索引（0-3）和avail_index编号（1-4）的映射关系
 */
int8_t cap_select_combine(uint8_t avail_index[], uint8_t avail_count, long target, uint8_t *idx1, uint8_t *idx2)
{
    // C89：变量全部前置声明并初始化
    long min_diff = LONG_MAX; // 初始化为long类型最大值（适配32位差值）
    uint8_t i = 0;
    uint8_t j = 0;
    uint8_t cap_num1 = 0;     // 第一个电容编号（1~4）
    uint8_t cap_num2 = 0;     // 第二个电容编号（1~4）
    uint8_t cap_idx1 = 0;     // 第一个电容的g_cap索引（0~3）
    uint8_t cap_idx2 = 0;     // 第二个电容的g_cap索引（0~3）
    long total = 0;
    long diff = 0;
    uint8_t num = g_cap_num.cap_num; // 实际电容总数（适配非4个的情况）

    // ========== 第一步：校验关键参数（空指针+可用数量） ==========
    if (idx1 == NULL || idx2 == NULL) {
        return -1; // 空指针错误
    }
    if (avail_count < 2) {
        return -2; // 不足两个可用电容
    }
    if (avail_index == NULL) {
        return -1; // 可用列表为空
    }

    // ========== 第二步：遍历所有两两组合（避免重复，j从i+1开始） ==========
    for (i = 0; i < avail_count; i++) {
        cap_num1 = avail_index[i];
        // 校验第一个电容编号有效性
        if (cap_num1 < 1 || cap_num1 > num) {
            continue; // 跳过非法编号
        }
        cap_idx1 = cap_num1 - 1; // 编号→索引转换（核心修正）

        for (j = i + 1; j < avail_count; j++) {
            cap_num2 = avail_index[j];
            // 校验第二个电容编号有效性
            if (cap_num2 < 1 || cap_num2 > num) {
                continue; // 跳过非法编号
            }
            cap_idx2 = cap_num2 - 1; // 编号→索引转换（核心修正）

            // 计算组合容量（×100匹配target单位）
            total = (g_cap[cap_idx1].value + g_cap[cap_idx2].value) * 100L;
            diff = labs(total - target);

            // 记录差值最小的组合
            if (diff < min_diff) {
                min_diff = diff;
                *idx1 = cap_num1; // 输出电容编号（1~4），而非索引
                *idx2 = cap_num2;
            }
        }
    }

    // 兜底：若所有组合都是非法编号，返回-1
    if (min_diff == LONG_MAX) {
        return -1;
    }

    return 1; // 成功选到两个有效电容
}
/**
 * @brief 滤波策略选择电容（仅判断state状态）【C89版本】
 * @param avail_index 可用电容编号数组（1~4，cap_filter筛选结果）
 * @param avail_count 可用电容数量
 * @param is_charge 1=投，0=切
 * @return 选中的电容编号（1~4）；对比不一致/无符合条件电容返回0
 * @note 1. 投电容核心逻辑：
 *          - 步骤1：按1→2→3→4顺序找第一个state==0（未投）的电容（find_cap）；
 *          - 步骤2：对比find_cap与avail_index[0]，相同则返回，不同则返回0；
 *       2. 切电容核心逻辑：仅判断state==1（已投），取avail_index[0]校验后返回；
 *       3. 仅判断state状态，不校验err、失败标记、启用状态等。
 */
uint8_t cap_select_filter(uint8_t avail_index[], uint8_t avail_count, uint8_t is_charge)
{
    // C89强制：所有变量在代码块开头声明并初始化
    uint8_t select_idx = 0;  // 默认返回0（对比不一致/无符合条件）
    uint8_t curr_cap_idx = 0;// 当前遍历的g_cap索引（0~3）
    uint8_t find_cap_num = 0;// 按顺序找到的电容编号（1~4），而非索引
    uint8_t num = g_cap_num.cap_num;

    // ========== 边界条件：无可用电容/空指针，直接返回0 ==========
    if (avail_count == 0 || avail_index == NULL) {
        return 0;
    }

    // ========== 投电容：仅判断state==0，对比avail_index[0] ==========
    if (is_charge) {
        // 步骤1：按0→1→2→3（对应电容1→2→3→4）找第一个state==0的电容
        for (curr_cap_idx = 0; curr_cap_idx < num; curr_cap_idx++) {
            if (g_cap[curr_cap_idx].state == 0) { // 仅判断state==0（未投）
                find_cap_num = curr_cap_idx + 1; // 转换为电容编号（1~4）
                break; // 找到第一个符合的，立即终止遍历
            }
        }

        // 步骤2：对比find_cap_num（1~4）与avail_index[0]（1~4），相同则返回
        if (find_cap_num != 0 && find_cap_num == avail_index[0]) {
            select_idx = find_cap_num;
        }
    }
    // ========== 切电容：仅判断state==1（已投）（原有逻辑正确，保留） ==========
    else {
        // 取avail_index[0]（电容编号1~4），转换为g_cap索引后校验state
        uint8_t cap_num = avail_index[0];
        if (cap_num >= 1 && cap_num <= num) { // 适配num（不一定是4），更通用
            if (g_cap[cap_num - 1].state == 1) { // 仅判断state==1（已投）
                select_idx = cap_num;
            }
        }
    }

    // 最终校验：确保返回值是1~num的有效编号（适配不同电容数量）
    return (select_idx >= 1 && select_idx <= num) ? select_idx : 0;
}
/**
 * @brief 投电容准备（选电容，记录到全局状态，不立即执行）
 * @return 1=选电容成功，-2=无符合条件电容
 */
int8_t cap_prepare_charge(long qc_best)
{
    uint8_t avail_index[4] = {0};
		uint8_t idx1 = 0, idx2 = 0;
    uint8_t avail_count = cap_filter(1, avail_index);
    uint8_t strategy=g_control_para.type;		
    if (avail_count == 0) return -2;

    g_cap_switch_state.pending_cap_count = 1; // 默认操作1个


    switch (strategy) {
        case CAP_STRATEGY_EQUAL:
            idx1 = cap_select_equal(avail_index, avail_count, 1);
            break;
        case CAP_STRATEGY_DIFF:
            idx1 = cap_select_diff(avail_index, avail_count, qc_best, 1);
            break;
        case CAP_STRATEGY_FILTER:
            idx1 = avail_index[0];
            break;
        case CAP_STRATEGY_COMBINE:
            if (cap_select_combine(avail_index, avail_count, qc_best, &idx1, &idx2) == 1) {
                g_cap_switch_state.pending_cap_idx[1] = idx2;
                g_cap_switch_state.pending_cap_count = 2;
            } else {
                return -2;
            }
            break;
        default:
            return -2;
    }

    g_cap_switch_state.pending_cap_idx[0] = idx1;
    g_cap_switch_state.switch_result = 0; // 重置反馈结果
    return 1;
}

/**
 * @brief 切电容准备（选电容，记录到全局状态，不立即执行）【修正+C89版本】
 * @param qc_min_abs 需切除的容量绝对值（×100）
 * @param strategy 投切策略（1=差容，2=组合，3=滤波，4=等容）
 * @param max_cap_num 最大电容数（1~4）
 * @return 1=选电容成功，-2=无符合条件电容/策略错误
 */
int8_t cap_prepare_discharge(long qc_min_abs)
{
    // C89：所有变量前置声明，按“数组→基础类型→指针”顺序
    uint8_t avail_index[4];    // 可用电容索引（已投+启用）
    uint8_t avail_count;       // 可用电容数量
    uint8_t idx1 = 0;          // 选中的第一个电容索引
    uint8_t idx2 = 0;          // 组合策略备用索引
    int8_t ret = -2;           // 默认返回失败
    uint8_t strategy=g_control_para.type;
    // 初始化可用索引数组（C89手动清零，避免随机值）
    avail_index[0] = 0;
    avail_index[1] = 0;
    avail_index[2] = 0;
    avail_index[3] = 0;

    // 1. 筛选已投+启用的电容（is_charge=0表示切）
    avail_count = cap_filter(0,avail_index);
    if (avail_count == 0) {
        return -2; // 无符合条件的电容
    }

    // 2. 初始化待操作电容数量为1（默认单电容操作）
    g_cap_switch_state.pending_cap_count = 1;

    // 3. 按策略选择要切的电容（核心修正：滤波策略调用专用函数）
    switch (strategy) {
        case CAP_STRATEGY_EQUAL: // 等容：切次数最多的
            idx1 = cap_select_equal(avail_index, avail_count, 0);
            ret = 1;
            break;

        case CAP_STRATEGY_DIFF: // 差容：切最接近目标容量的
            idx1 = cap_select_diff(avail_index, avail_count, qc_min_abs, 0);
            ret = 1;
            break;

        case CAP_STRATEGY_FILTER: // 滤波：先投的先切（调用专用函数）
            idx1 = cap_select_filter(avail_index, avail_count, 0); // 0=切
            ret = 1;
            break;

        case CAP_STRATEGY_COMBINE: // 组合：选两个最接近的
            if (cap_select_combine(avail_index, avail_count, qc_min_abs, &idx1, &idx2) == 1) {
                g_cap_switch_state.pending_cap_idx[1] = idx2;
                g_cap_switch_state.pending_cap_count = 2; // 组合策略操作2个
                ret = 1;
            } else {
                ret = -2; // 不足2个电容，组合失败
            }
            break;

        default: // 无效策略
            ret = -2;
            break;
    }

    // 4. 仅策略执行成功时，更新全局状态
    if (ret == 1) {
        g_cap_switch_state.pending_cap_idx[0] = idx1;
        g_cap_switch_state.switch_result = 0; // 重置反馈结果
    }

    return ret;
}


/**
 * @brief 检测投切反馈（读取限位开关/辅助触点）
 * @param cap_idx 电容索引
 * @param is_charge 1=投，0=切
 * @return 1=成功，0=失败
 */
//uint8_t cap_check_switch_feedback(uint8_t cap_idx, uint8_t is_charge)
//{
//    // 示例：读取GPIO反馈信号，投成功=高电平，切成功=低电平
//    uint8_t feedback = 1;
//    return (is_charge == feedback) ? 1 : 0;
//}

/**
 * @brief 撤销所有未完成的投切指令（循环判断需求变化时调用，C89兼容）
 * @param reason 撤销原因（0=主动撤销，1=需求变化，2=电压超限，3=手动停止）
 */
void cap_cancel_pending_cmd(uint8_t reason)
{

//    uint8_t is_charge;
//    uint8_t cap_idx;

    // 1. 若已发送命令但未完成反馈，先发送“取消”指令（硬件层面）
//    if (g_cap_switch_state.pending_cmd != CMD_NONE && g_cap_switch_state.delay_remaining == 0) {
//        // C89：条件判断后赋值，避免混合声明和代码
//        is_charge = (g_cap_switch_state.pending_cmd == CMD_CHARGE) ? 1 : 0;
//        
//        // 发送取消命令（反向操作，避免硬件卡滞）
//        for (i = 0; i < g_cap_switch_state.pending_cap_count; i++) {
//            cap_idx = g_cap_switch_state.pending_cap_idx[i];
//            // 反向命令：投→切，切→投，取消未完成操作
//            cap_send_switch_cmd(cap_idx, !is_charge);
//        }
//    }

    // 2. 清空所有待执行状态（核心：撤销旧指令）
    g_cap_switch_state.pending_cmd = CMD_NONE;
    g_cap_switch_state.pending_cap_idx[0] = 0;
    g_cap_switch_state.pending_cap_idx[1] = 0;
    g_cap_switch_state.pending_cap_count = 0;
    g_cap_switch_state.delay_remaining = 0;
    g_cap_switch_state.switch_result = 0; // 重置反馈结果
}


/**
 * @brief 电容投切1秒计时函数（C89 兼容）
 * @功能 每1秒调用1次，负责：
 *       1. 投切延时阶段（CMD_CHARGE/DISCHARGE）：递减delay_remaining
 *       2. 反馈等待阶段（CMD_WAIT_FEEDBACK）：累加feedback_timer
 *       3. 空闲阶段（CMD_NONE）：无动作
 * @返回值 状态码：
 *         0：无计时动作/空闲状态
 *         1：投切延时结束（delay_remaining 减至0）
 *         2：反馈等待4秒结束（feedback_timer 累加到4）
 * @note  1. 必须由1秒定时器中断或主循环定时调用；
 *        2. 仅更新计时参数，不处理任何逻辑（逻辑由cap_switch_control处理）；
 *        3. 反馈等待超时时间为4秒，适配relay侧3秒反馈检测。
 */
uint8_t cap_switch_delay_handler(void)
{
    /* C89 变量开头声明 */
    uint8_t ret_val = 0;
    uint8_t i;

    /* ========== 原有流程计时逻辑（完全保留） ========== */
    switch (g_cap_switch_state.pending_cmd)
    {
        case CMD_NONE: ret_val = 0; break;
        case CMD_CHARGE:
        case CMD_DISCHARGE:
            if (g_cap_switch_state.delay_remaining > 0)
            {
                g_cap_switch_state.delay_remaining--;
                ret_val = 0;
            }
            else ret_val = 1;
            break;
        case CMD_WAIT_FEEDBACK:
//            if (g_cap_switch_state.feedback_timer < 4)
//            {
//                g_cap_switch_state.feedback_timer++;
//                ret_val = 0;
//            }
//            else ret_val = 2;
				                 ret_val = 0; 
            break;
        default:
            g_cap_switch_state.pending_cmd = CMD_NONE;
            ret_val = 0;
            break;
    }

    /* ========== 新增：电容投切闭锁计时（1秒递增） ========== */
    for(i=0; i<4; i++)  // 遍历4路电容
    {
        if(g_cap_latch[i].cmd_state == 1)  // 已下发命令，未到2秒
        {
            g_cap_latch[i].timer++;
            if(g_cap_latch[i].timer >= 2)  // 2秒到
            {
                g_cap_latch[i].cmd_state = 2;  // 标记为“已关断”
                g_cap_latch[i].timer = 0;      // 重置计时
            }
        }
    }

    return ret_val;
}


/**
 * @brief 重置投切状态（辅助函数，C89）
 * @功能 清空所有投切状态，保证流程闭环后可启动新流程
 */
void cap_reset_switch_state(void)
{
    g_cap_switch_state.pending_cmd = CMD_NONE;
    g_cap_switch_state.delay_remaining = 0;
    g_cap_switch_state.feedback_timer = 0;
    g_cap_switch_state.last_cmd = CMD_NONE;
    g_cap_switch_state.pending_cap_count = 0;
    g_cap_switch_state.pending_cap_idx[0] = 0;

}











/**
 * @brief 电容投切控制（完整流程总控，严格C89）
 * @功能 完整投切流程：
 *       1. 投切合法性校验 → 2. 选目标电容 → 3. 初始化计时 → 4. 读取1s计时结果 → 
 *       5. 倒计时结束置继电器标志 → 6. 反馈检测（成功/失败）→ 7. 流程闭环（重置状态）
 * @返回值 状态码：
 *         -8：手动模式，禁止投切
 *         -6：电压异常，禁止投切
 *         -3：每日投切次数达上限
 *         -4：投切间隔未满足
 *         -2：无符合条件的电容
 *         -1：电容数量参数异常
 *         -7：投切反馈失败（4秒超时/状态不符）
 *          0：流程中（计时中/反馈等待中）
 *         11：投电容成功
 *         12：切电容成功
 *         10：继电器标志已置位（进入反馈等待）
 * @note  1. 依赖1s计时函数更新 g_cap_switch_state.delay_remaining/feedback_timer；
 *        2. 严格单轮流程管控，未完成时不启动新流程；
 *        3. 仅置位继电器标志位，实际引脚控制由relay函数处理；
 *        4. 一轮流程完成（成功/失败）后才重置状态，允许下一轮；
 *        5. 反馈检测超时时间为4秒（适配继电器3秒反馈检测）；
 *        6. 滤波模式下投切失败后，终止后续投切流程。
 */
//int8_t cap_switch_control(void)
//{
//    /* C89 强制：所有变量在代码块开头声明 */
//    uint8_t volt_status;    
//    long qc_min;            // 最新补偿需求（每轮必取）
//    long qc_best;           // 最新最优补偿容量（每轮必取）
//    uint8_t max_cap_num;    
//    uint32_t curr_time;     
//    uint8_t is_charge;      // 最新投切类型（每轮计算）
//    uint8_t target_cap;     // 最新目标电容序号（每轮计算）
//    uint8_t target_cap_idx; 
//    int8_t ret = 0;         // 默认返回值
//    // 反馈检测相关变量
//    uint8_t charge_ok_flag;    
//    uint8_t refuse_charge_flag;
//    uint8_t discharge_ok_flag; 
//    uint8_t refuse_discharge_flag;

//    /* ==================== 第一步：每轮必检 - 获取最新需求 ==================== */
//    // 1. 读取最新补偿需求（核心：每一轮都重新获取，确保实时性）
//    qc_min = g_cap_requirement.qc_min;
//    qc_best = g_cap_requirement.qc_best;
//    max_cap_num = g_cap_num.cap_num;

//    // 2. 无补偿需求：直接重置流程，返回0
//    if (qc_min == 0) {
//        cap_reset_switch_state();
//        return 0;
//    }

//    // 3. 计算最新投切类型 + 目标电容（每轮都重新选，确保序号最新）
//    is_charge = (qc_min > 0) ? 1 : 0;
//    if (is_charge) {
//        ret = cap_prepare_charge(qc_best); // 重新选投的电容
//    } else {
//        ret = cap_prepare_discharge(qc_best); // 重新选切的电容
//    }
//    // 无符合条件的电容：重置流程，返回-2
//    if (ret <= 0) {
//        cap_reset_switch_state();
//        return -2;
//    }
//    // 记录最新目标电容序号（每轮更新）
//    target_cap = g_cap_switch_state.pending_cap_idx[0];
//    target_cap_idx = target_cap - 1;

//    /* ==================== 第二步：对比最新需求与当前流程，不一致则重置 ==================== */
//    // 1. 检查当前是否有未完成的流程
//    if ((g_cap_switch_state.pending_cmd != CMD_NONE)&&((g_cap_switch_state.pending_cmd != CMD_WAIT_FEEDBACK))) {
//        // 2. 提取当前流程的投切类型 + 序号
//        uint8_t curr_pending_is_charge = (g_cap_switch_state.pending_cmd == CMD_CHARGE) ? 1 : 0;
//        uint8_t curr_pending_cap = g_cap_switch_state.pending_cap_idx[0];

//        // 3. 核心判断：投切类型 或 目标序号 不一致 → 重置旧流程
//        if ( (curr_pending_is_charge != is_charge) || (curr_pending_cap != target_cap) ) {
//            cap_reset_switch_state(); // 重置所有状态，放弃旧流程
//        }
//    }

//    /* ==================== 第三步：流程管控 - 按最新需求执行 ==================== */
//    if (g_cap_switch_state.pending_cmd != CMD_NONE) {
//        switch (g_cap_switch_state.pending_cmd) {
//            case CMD_CHARGE:
//            case CMD_DISCHARGE:
//                // 倒计时未结束：返回0（已确保是最新需求的计时）
//                if (g_cap_switch_state.delay_remaining > 0) {
//                    return 0;
//                }
//                // 倒计时结束：置继电器标志 + 进入反馈等待
//                else {
//                    goto SET_RELAY_FLAG;
//                }
//                break;

//            case CMD_WAIT_FEEDBACK:
//                // 反馈等待中：检测反馈结果（此阶段不重置，避免指令混乱）
//                goto CHECK_FEEDBACK;
//                break;

//            default:
//                g_cap_switch_state.pending_cmd = CMD_NONE;
//                return 0;
//        }
//    }

//    /* ==================== 第四步：新流程初始化 - 合法性校验 + 计时初始化 ==================== */
//    // 1. 手动模式拦截
//    if ((g_all_io_state & 0x01) == 0x01) {
//        cap_reset_switch_state();
//        return -8;
//    }

//    // 2. 电压合法性校验
//    volt_status = Volt_Check();
//    if (volt_status != 1) { // 非正常电压（2/3/4/默认）
//        cap_reset_switch_state();
//        return -6;
//    }

//    // 3. 电容数量参数校验
//    if (max_cap_num == 0 || max_cap_num > 4) {
//        return -1;
//    }

//    // 4. 次数/间隔校验
//    if (g_cap_switch_state.daily_switch_count >= g_control_para.times) {
//        return -3;
//    }
//    curr_time = Get_SystemTime();
//    if ((curr_time - g_cap_switch_state.last_switch_time) < g_control_para.time_interval) {
//        return -4;
//    }

//    // 5. 初始化最新需求的投切计时状态（核心：记录最新序号+类型）
//    g_cap_switch_state.pending_cmd = is_charge ? CMD_CHARGE : CMD_DISCHARGE;
//    g_cap_switch_state.delay_remaining = is_charge ? g_control_para.delay_time_on : g_control_para.delay_time_off;
//    g_cap_switch_state.last_cmd = g_cap_switch_state.pending_cmd;
//    g_cap_switch_state.feedback_timer = 0;
//    // 关键：记录本轮的目标电容序号（用于后续对比）
//    g_cap_switch_state.pending_cap_idx[0] = target_cap;

//    return 0; // 进入计时阶段

//    /* ==================== 第五步：倒计时结束 - 置继电器标志（复用原逻辑） ==================== */
//SET_RELAY_FLAG:
//		if(g_cap_switch_state.pending_cap_count>0)
//		{
//    target_cap = g_cap_switch_state.pending_cap_idx[0];
//    target_cap_idx = target_cap - 1;
//    if (g_cap_switch_state.last_cmd == CMD_CHARGE) {
//        switch (target_cap) {
//            case 1: g_relay_state |= CAP1_ON; break;
//            case 2: g_relay_state |= CAP2_ON; break;
//            case 3: g_relay_state |= CAP3_ON; break;
//            case 4: g_relay_state |= CAP4_ON; break;
//            default: break;
//        }
//    } else {
//        switch (target_cap) {
//            case 1: g_relay_state |= CAP1_OFF; break;
//            case 2: g_relay_state |= CAP2_OFF; break;
//            case 3: g_relay_state |= CAP3_OFF; break;
//            case 4: g_relay_state |= CAP4_OFF; break;
//            default: break;
//        }
//    }
//	}
//		if(g_cap_switch_state.pending_cap_count>1)
//		{
//    target_cap = g_cap_switch_state.pending_cap_idx[1];
//    target_cap_idx = target_cap - 1;
//    if (g_cap_switch_state.last_cmd == CMD_CHARGE) {
//        switch (target_cap) {
//            case 1: g_relay_state |= CAP1_ON; break;
//            case 2: g_relay_state |= CAP2_ON; break;
//            case 3: g_relay_state |= CAP3_ON; break;
//            case 4: g_relay_state |= CAP4_ON; break;
//            default: break;
//        }
//    } else {
//        switch (target_cap) {
//            case 1: g_relay_state |= CAP1_OFF; break;
//            case 2: g_relay_state |= CAP2_OFF; break;
//            case 3: g_relay_state |= CAP3_OFF; break;
//            case 4: g_relay_state |= CAP4_OFF; break;
//            default: break;
//        }
//    }
//	}		
//    g_cap_switch_state.pending_cmd = CMD_WAIT_FEEDBACK;
//    g_cap_switch_state.feedback_timer = 0;
//    return 10;

//    /* ==================== 第六步：反馈检测（复用原逻辑，仅流程闭环） ==================== */
//CHECK_FEEDBACK:
//charge_ok_flag = 0;
//refuse_charge_flag = 0;
//discharge_ok_flag = 0;
//refuse_discharge_flag = 0;

//target_cap = g_cap_switch_state.pending_cap_idx[0];
//target_cap_idx = target_cap - 1;

//// 映射目标电容到标志位（复用原逻辑）
//switch (target_cap) {
//    case 1:
//        charge_ok_flag = CAP1_CHARGE_OK;
//        refuse_charge_flag = CAP1_REFUSE_CHARGE;
//        discharge_ok_flag = CAP1_DISCHARGE_OK;
//        refuse_discharge_flag = CAP1_REFUSE_DISCHARGE;
//        break;
//    case 2:
//        charge_ok_flag = CAP2_CHARGE_OK;
//        refuse_charge_flag = CAP2_REFUSE_CHARGE;
//        discharge_ok_flag = CAP2_DISCHARGE_OK;
//        refuse_discharge_flag = CAP2_REFUSE_DISCHARGE;
//        break;
//    case 3:
//        charge_ok_flag = CAP3_CHARGE_OK;
//        refuse_charge_flag = CAP3_REFUSE_CHARGE;
//        discharge_ok_flag = CAP3_DISCHARGE_OK;
//        refuse_discharge_flag = CAP3_REFUSE_DISCHARGE;
//        break;
//    case 4:
//        charge_ok_flag = CAP4_CHARGE_OK;
//        refuse_charge_flag = CAP4_REFUSE_CHARGE;
//        discharge_ok_flag = CAP4_DISCHARGE_OK;
//        refuse_discharge_flag = CAP4_REFUSE_DISCHARGE;
//        break;
//    default:
//        cap_reset_switch_state();
//        return -7;
//}

//// 仅检测软件置的标志位（循环执行，没检测到就返回0等待）
//if (g_cap_switch_state.last_cmd == CMD_CHARGE) {
//    if ((g_sys_flag[4] & charge_ok_flag) != 0) {  // 投入成功（软件置位）
//        g_cap_fail_flag[target_cap_idx] = 0;
//        cap_reset_switch_state();
//        g_cap_switch_state.daily_switch_count++;
//			  g_stat.day_times_on++;
//			  g_stat.month_times_on++;
//			  Set_Stat_Para(&g_stat);
//        g_cap_switch_state.last_switch_time = Get_SystemTime();
//        Set_Stat_Para(&g_stat);
//        return 11;
//    } else if ( (target_cap == 1 || target_cap == 2) && (g_sys_flag[2] & refuse_charge_flag) != 0 ) {//1,2拒投（软件置位）
//        g_cap_fail_flag[target_cap_idx] = 1;
//        if (g_control_para.type == CAP_STRATEGY_FILTER) {
//            cap_reset_switch_state();
//            g_cap_switch_state.daily_switch_count = g_control_para.times;
//        }
//        cap_reset_switch_state();
//        g_cap_switch_state.daily_switch_count++;
//        g_cap_switch_state.last_switch_time = Get_SystemTime();
//        return -7;
//    } else if ( (target_cap == 3 || target_cap == 4) && (g_sys_flag[3] & refuse_charge_flag) != 0 ) {//3,4拒投（软件置位）
//        g_cap_fail_flag[target_cap_idx] = 1;
//        if (g_control_para.type == CAP_STRATEGY_FILTER) {
//            cap_reset_switch_state();
//            g_cap_switch_state.daily_switch_count = g_control_para.times;
//        }
//        cap_reset_switch_state();
//        g_cap_switch_state.daily_switch_count++;
//        g_cap_switch_state.last_switch_time = Get_SystemTime();
//        return -7;
//    } else {
//        // 未检测到标志位（继电器2秒计时还没到）→ 返回0，等待下一次循环检测
//        return 0;
//    }
//} else { // 切电容逻辑
//    if ((g_sys_flag[5] & discharge_ok_flag) != 0) {  // 切除成功（软件置位）
//        g_cap_fail_flag[target_cap_idx] = 0;
//        cap_reset_switch_state();
//        g_cap_switch_state.daily_switch_count++;
//			  g_stat.day_times_on++;
//			  g_stat.month_times_on++;
//          Set_Stat_Para(&g_stat);			
//        g_cap_switch_state.last_switch_time = Get_SystemTime();
//        return 12;
//    } else if ((g_sys_flag[3] & refuse_discharge_flag) != 0) {  // 拒切（软件置位）
//        g_cap_fail_flag[target_cap_idx] = 1;
//        if (g_control_para.type == CAP_STRATEGY_FILTER) {
//            cap_reset_switch_state();
//            g_cap_switch_state.daily_switch_count = g_control_para.times;
//        }
//        cap_reset_switch_state();
//        g_cap_switch_state.daily_switch_count++;
//        g_cap_switch_state.last_switch_time = Get_SystemTime();
//        return -7;
//    } else {
//        // 未检测到标志位（继电器2秒计时还没到）→ 返回0，等待下一次循环检测
//        return 0;
//    }
//}
//}  //不支持组合模式。

int8_t cap_switch_control(void)
{
    /* C89 强制：所有变量在代码块开头声明（新增双电容相关变量） */
    uint8_t volt_status;    
    long qc_min;            // 最新补偿需求（每轮必取）
    long qc_best;           // 最新最优补偿容量（每轮必取）
    uint8_t max_cap_num;    
    uint32_t curr_time;     
    uint8_t is_charge;      // 最新投切类型（每轮计算）
//    uint8_t target_cap;     // 最新目标电容序号（每轮计算）
//    uint8_t target_cap_idx; 
    int8_t ret = 0;         // 默认返回值
    
    // 反馈检测相关变量（扩展双电容）
    uint8_t charge_ok_flag1;    
    uint8_t refuse_charge_flag1;
    uint8_t discharge_ok_flag1; 
    uint8_t refuse_discharge_flag1;
    uint8_t charge_ok_flag2;    
    uint8_t refuse_charge_flag2;
    uint8_t discharge_ok_flag2; 
    uint8_t refuse_discharge_flag2;
    uint8_t target_cap1;
    uint8_t target_cap2;
    uint8_t target_cap_idx1;
    uint8_t target_cap_idx2;
    uint8_t cap1_success;
    uint8_t cap2_success;
    uint8_t cap1_fail;
    uint8_t cap2_fail;
    uint8_t is_double_cap;
    uint8_t curr_pending_is_charge;
    uint8_t curr_pending_cap1;
    uint8_t curr_pending_cap2;

    /* ==================== 第一步：每轮必检 - 获取最新需求 ==================== */
    // 1. 读取最新补偿需求（核心：每一轮都重新获取，确保实时性）
    qc_min = g_cap_requirement.qc_min;
    qc_best = g_cap_requirement.qc_best;
    max_cap_num = g_cap_num.cap_num;

    // 2. 无补偿需求：直接重置流程，返回0
    if (qc_min == 0) {
        cap_reset_switch_state();
        return 0;
    }
   if(g_cap_switch_state.pending_cmd != CMD_WAIT_FEEDBACK)
	 {
    // 3. 计算最新投切类型 + 目标电容（每轮都重新选，确保序号最新）
    is_charge = (qc_min > 0) ? 1 : 0;
    if (is_charge) {
        ret = cap_prepare_charge(qc_best); // 重新选投的电容（支持返回双电容）
    } else {
        ret = cap_prepare_discharge(qc_best); // 重新选切的电容（支持返回双电容）
    }
    // 无符合条件的电容：重置流程，返回-2
    if (ret <= 0) {
        cap_reset_switch_state();
        return -2;
    }
    // 记录最新目标电容序号（支持双电容）
    target_cap1 = g_cap_switch_state.pending_cap_idx[0];
    target_cap_idx1 = target_cap1 - 1;
    is_double_cap = (g_cap_switch_state.pending_cap_count > 1) ? 1 : 0;
    if (is_double_cap) {
        target_cap2 = g_cap_switch_state.pending_cap_idx[1];
        target_cap_idx2 = target_cap2 - 1;
    }
	}
    /* ==================== 第二步：对比最新需求与当前流程，不一致则重置 ==================== */
    // 1. 检查当前是否有未完成的流程
    if ((g_cap_switch_state.pending_cmd != CMD_NONE) && ((g_cap_switch_state.pending_cmd != CMD_WAIT_FEEDBACK))) {
        // 2. 提取当前流程的投切类型 + 序号（支持双电容）
        curr_pending_is_charge = (g_cap_switch_state.pending_cmd == CMD_CHARGE) ? 1 : 0;
        curr_pending_cap1 = g_cap_switch_state.pending_cap_idx[0];
        curr_pending_cap2 = g_cap_switch_state.pending_cap_idx[1];

        // 3. 核心判断：投切类型 或 目标序号 不一致 → 重置旧流程
        // 双电容：类型/任意一个电容序号不一致则重置；单电容：类型/第一个电容不一致则重置
        if (curr_pending_is_charge != is_charge || 
            curr_pending_cap1 != target_cap1 || 
            (is_double_cap && curr_pending_cap2 != target_cap2)) {
            cap_reset_switch_state(); // 重置所有状态，放弃旧流程
        }
    }

    /* ==================== 第三步：流程管控 - 按最新需求执行 ==================== */
    if (g_cap_switch_state.pending_cmd != CMD_NONE) {
        switch (g_cap_switch_state.pending_cmd) {
            case CMD_CHARGE:
            case CMD_DISCHARGE:
                // 倒计时未结束：返回0（已确保是最新需求的计时）
                if (g_cap_switch_state.delay_remaining > 0) {
                    return 0;
                }
                // 倒计时结束：置继电器标志 + 进入反馈等待
                else {
                    goto SET_RELAY_FLAG;
                }
                break;

            case CMD_WAIT_FEEDBACK:
                // 反馈等待中：检测反馈结果（此阶段不重置，避免指令混乱）
                goto CHECK_FEEDBACK;
                break;

            default:
                g_cap_switch_state.pending_cmd = CMD_NONE;
                return 0;
        }
    }

    /* ==================== 第四步：新流程初始化 - 合法性校验 + 计时初始化 ==================== */
    // 1. 手动模式拦截
    if ((g_all_io_state & 0x01) == 0x01) {
        cap_reset_switch_state();
        return -8;
    }

    // 2. 电压合法性校验
    volt_status = Volt_Check();
    if (volt_status != 1) { // 非正常电压（2/3/4/默认）
        cap_reset_switch_state();
        return -6;
    }

    // 3. 电容数量参数校验
    if (max_cap_num == 0 || max_cap_num > 4) {
        return -1;
    }

    // 4. 次数/间隔校验
    if (g_cap_switch_state.daily_switch_count >= g_control_para.times) {
        return -3;
    }
    curr_time = Get_SystemTime();
    if ((curr_time - g_cap_switch_state.last_switch_time) < g_control_para.time_interval) {
        return -4;
    }

    // 5. 初始化最新需求的投切计时状态（核心：记录最新序号+类型，支持双电容）
    g_cap_switch_state.pending_cmd = is_charge ? CMD_CHARGE : CMD_DISCHARGE;
    g_cap_switch_state.delay_remaining = is_charge ? g_control_para.delay_time_on : g_control_para.delay_time_off;
    g_cap_switch_state.last_cmd = g_cap_switch_state.pending_cmd;
    g_cap_switch_state.feedback_timer = 0;
    // 关键：记录本轮的目标电容序号（支持双电容）
    g_cap_switch_state.pending_cap_idx[0] = target_cap1;
    if (is_double_cap) {
        g_cap_switch_state.pending_cap_idx[1] = target_cap2;
    }

    return 0; // 进入计时阶段

    /* ==================== 第五步：倒计时结束 - 置继电器标志（支持双电容） ==================== */
SET_RELAY_FLAG:
    if(g_cap_switch_state.pending_cap_count>0)
    {
        target_cap1 = g_cap_switch_state.pending_cap_idx[0];
        target_cap_idx1 = target_cap1 - 1;
        if (g_cap_switch_state.last_cmd == CMD_CHARGE) {
            switch (target_cap1) {
                case 1: g_relay_state |= CAP1_ON; break;
                case 2: g_relay_state |= CAP2_ON; break;
                case 3: g_relay_state |= CAP3_ON; break;
                case 4: g_relay_state |= CAP4_ON; break;
                default: break;
            }
        } else {
            switch (target_cap1) {
                case 1: g_relay_state |= CAP1_OFF; break;
                case 2: g_relay_state |= CAP2_OFF; break;
                case 3: g_relay_state |= CAP3_OFF; break;
                case 4: g_relay_state |= CAP4_OFF; break;
                default: break;
            }
        }
    }
    if(g_cap_switch_state.pending_cap_count>1)
    {
        target_cap2 = g_cap_switch_state.pending_cap_idx[1];
        target_cap_idx2 = target_cap2 - 1;
        if (g_cap_switch_state.last_cmd == CMD_CHARGE) {
            switch (target_cap2) {
                case 1: g_relay_state |= CAP1_ON; break;
                case 2: g_relay_state |= CAP2_ON; break;
                case 3: g_relay_state |= CAP3_ON; break;
                case 4: g_relay_state |= CAP4_ON; break;
                default: break;
            }
        } else {
            switch (target_cap2) {
                case 1: g_relay_state |= CAP1_OFF; break;
                case 2: g_relay_state |= CAP2_OFF; break;
                case 3: g_relay_state |= CAP3_OFF; break;
                case 4: g_relay_state |= CAP4_OFF; break;
                default: break;
            }
        }
    }        
    g_cap_switch_state.pending_cmd = CMD_WAIT_FEEDBACK;
    g_cap_switch_state.feedback_timer = 0;
    return 10;

    /* ==================== 第六步：反馈检测（适配单/双电容组合模式） ==================== */
CHECK_FEEDBACK:
    // 初始化状态标记
    charge_ok_flag1 = 0;
    refuse_charge_flag1 = 0;
    discharge_ok_flag1 = 0;
    refuse_discharge_flag1 = 0;
    charge_ok_flag2 = 0;
    refuse_charge_flag2 = 0;
    discharge_ok_flag2 = 0;
    refuse_discharge_flag2 = 0;
    cap1_success = 0;
    cap2_success = 0;
    cap1_fail = 0;
    cap2_fail = 0;

    // 1. 读取当前待投切电容信息（支持双电容）
    target_cap1 = g_cap_switch_state.pending_cap_idx[0];
    target_cap_idx1 = target_cap1 - 1;
    is_double_cap = (g_cap_switch_state.pending_cap_count > 1) ? 1 : 0;
    if (is_double_cap) {
        target_cap2 = g_cap_switch_state.pending_cap_idx[1];
        target_cap_idx2 = target_cap2 - 1;
    }

    // 2. 映射第一个电容的标志位
    switch (target_cap1) {
        case 1:
            charge_ok_flag1 = CAP1_CHARGE_OK;
            refuse_charge_flag1 = CAP1_REFUSE_CHARGE;
            discharge_ok_flag1 = CAP1_DISCHARGE_OK;
            refuse_discharge_flag1 = CAP1_REFUSE_DISCHARGE;
            break;
        case 2:
            charge_ok_flag1 = CAP2_CHARGE_OK;
            refuse_charge_flag1 = CAP2_REFUSE_CHARGE;
            discharge_ok_flag1 = CAP2_DISCHARGE_OK;
            refuse_discharge_flag1 = CAP2_REFUSE_DISCHARGE;
            break;
        case 3:
            charge_ok_flag1 = CAP3_CHARGE_OK;
            refuse_charge_flag1 = CAP3_REFUSE_CHARGE;
            discharge_ok_flag1 = CAP3_DISCHARGE_OK;
            refuse_discharge_flag1 = CAP3_REFUSE_DISCHARGE;
            break;
        case 4:
            charge_ok_flag1 = CAP4_CHARGE_OK;
            refuse_charge_flag1 = CAP4_REFUSE_CHARGE;
            discharge_ok_flag1 = CAP4_DISCHARGE_OK;
            refuse_discharge_flag1 = CAP4_REFUSE_DISCHARGE;
            break;
        default:
            cap_reset_switch_state();
            return -7;
    }

    // 3. 双电容模式：映射第二个电容的标志位
    if (is_double_cap) {
        switch (target_cap2) {
            case 1:
                charge_ok_flag2 = CAP1_CHARGE_OK;
                refuse_charge_flag2 = CAP1_REFUSE_CHARGE;
                discharge_ok_flag2 = CAP1_DISCHARGE_OK;
                refuse_discharge_flag2 = CAP1_REFUSE_DISCHARGE;
                break;
            case 2:
                charge_ok_flag2 = CAP2_CHARGE_OK;
                refuse_charge_flag2 = CAP2_REFUSE_CHARGE;
                discharge_ok_flag2 = CAP2_DISCHARGE_OK;
                refuse_discharge_flag2 = CAP2_REFUSE_DISCHARGE;
                break;
            case 3:
                charge_ok_flag2 = CAP3_CHARGE_OK;
                refuse_charge_flag2 = CAP3_REFUSE_CHARGE;
                discharge_ok_flag2 = CAP3_DISCHARGE_OK;
                refuse_discharge_flag2 = CAP3_REFUSE_DISCHARGE;
                break;
            case 4:
                charge_ok_flag2 = CAP4_CHARGE_OK;
                refuse_charge_flag2 = CAP4_REFUSE_CHARGE;
                discharge_ok_flag2 = CAP4_DISCHARGE_OK;
                refuse_discharge_flag2 = CAP4_REFUSE_DISCHARGE;
                break;
            default:
                cap_reset_switch_state();
                return -7;
        }
    }

    // 4. 投电容逻辑（适配单/双电容）
    if (g_cap_switch_state.last_cmd == CMD_CHARGE) {
        // 4.1 检测第一个电容状态
        if ((g_sys_flag[4] & charge_ok_flag1) != 0) {
            cap1_success = 1;
//            g_cap_fail_flag[target_cap_idx1] = 0;
        } else if ( (target_cap1 == 1 || target_cap1 == 2) && (g_sys_flag[2] & refuse_charge_flag1) != 0 ) {
            cap1_fail = 1;
//            g_cap_fail_flag[target_cap_idx1] = 1;
        } else if ( (target_cap1 == 3 || target_cap1 == 4) && (g_sys_flag[3] & refuse_charge_flag1) != 0 ) {
            cap1_fail = 1;
//            g_cap_fail_flag[target_cap_idx1] = 1;
        }

        // 4.2 双电容模式：检测第二个电容状态
        if (is_double_cap) {
            if ((g_sys_flag[4] & charge_ok_flag2) != 0) {
                cap2_success = 1;
//                g_cap_fail_flag[target_cap_idx2] = 0;
            } else if ( (target_cap2 == 1 || target_cap2 == 2) && (g_sys_flag[2] & refuse_charge_flag2) != 0 ) {
                cap2_fail = 1;
                g_cap_fail_flag[target_cap_idx2] = 1;
            } else if ( (target_cap2 == 3 || target_cap2 == 4) && (g_sys_flag[3] & refuse_charge_flag2) != 0 ) {
                cap2_fail = 1;
//                g_cap_fail_flag[target_cap_idx2] = 1;
            }
        }

        // 4.3 结果判定
        if (cap1_fail || (is_double_cap && cap2_fail)) {
            // 只要有一个电容拒投 → 整体失败
            if (g_control_para.type == CAP_STRATEGY_FILTER) {
                cap_reset_switch_state();
                g_cap_switch_state.daily_switch_count = g_control_para.times;
            }
            cap_reset_switch_state();
            g_cap_switch_state.daily_switch_count++;
            g_cap_switch_state.last_switch_time = Get_SystemTime();
            return -7;
        } else if (cap1_success && (!is_double_cap || cap2_success)) {
            // 单电容成功 / 双电容都成功 → 整体成功
            g_cap_switch_state.daily_switch_count++;
            g_cap_switch_state.last_switch_time = Get_SystemTime();
            
            // 统计次数：单电容+1，双电容+2（仅写一次存储）
            g_stat.day_times_on += (is_double_cap ? 2 : 1);
            g_stat.month_times_on += (is_double_cap ? 2 : 1);
            Set_Stat_Para(&g_stat);

            cap_reset_switch_state();
            return 11;
        } else {
            // 未检测到标志位 → 返回0等待下一次循环
            return 0;
        }
    } 
    // 5. 切电容逻辑（适配单/双电容，修复统计笔误）
    else {
        // 5.1 检测第一个电容状态
        if ((g_sys_flag[5] & discharge_ok_flag1) != 0) {
            cap1_success = 1;
//            g_cap_fail_flag[target_cap_idx1] = 0;
        } else if ((g_sys_flag[3] & refuse_discharge_flag1) != 0) {
            cap1_fail = 1;
//            g_cap_fail_flag[target_cap_idx1] = 1;
        }

        // 5.2 双电容模式：检测第二个电容状态
        if (is_double_cap) {
            if ((g_sys_flag[5] & discharge_ok_flag2) != 0) {
                cap2_success = 1;
//                g_cap_fail_flag[target_cap_idx2] = 0;
            } else if ((g_sys_flag[3] & refuse_discharge_flag2) != 0) {
                cap2_fail = 1;
//                g_cap_fail_flag[target_cap_idx2] = 1;
            }
        }

        // 5.3 结果判定
        if (cap1_fail || (is_double_cap && cap2_fail)) {
            // 只要有一个电容拒切 → 整体失败
            if (g_control_para.type == CAP_STRATEGY_FILTER) {
                cap_reset_switch_state();
                g_cap_switch_state.daily_switch_count = g_control_para.times;
            }
            cap_reset_switch_state();
            g_cap_switch_state.daily_switch_count++;
            g_cap_switch_state.last_switch_time = Get_SystemTime();
            return -7;
        } else if (cap1_success && (!is_double_cap || cap2_success)) {
            // 单电容成功 / 双电容都成功 → 整体成功
            g_cap_switch_state.daily_switch_count++;
            g_cap_switch_state.last_switch_time = Get_SystemTime();
            
            // 修复统计笔误：切电容累加off次数，双电容+2（仅写一次存储）
            g_stat.day_times_off += (is_double_cap ? 2 : 1);
            g_stat.month_times_off += (is_double_cap ? 2 : 1);
            Set_Stat_Para(&g_stat);

            cap_reset_switch_state();
            return 12;
        } else {
            // 未检测到标志位 → 返回0等待下一次循环
            return 0;
        }
    }
}