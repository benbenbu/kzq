




#include "config.h"
#include "system.h"
#include "fm31256.h"
#include "menu.h"
#include "adc_key.h"
#include "bsp.h"
#include "menu.h"
#include "lcd.h"
#include "data_save.h"
#include "HT7036.h"
#include "time.h"
#include "delay.h"

//参数定义，系统用参数



//参数定义，用户参数

 Date_Struct  system_date;
 Time_Struct  system_time;
 
 Cap_Data  g_cap_data[4];
 Sys_Data  g_sys_data;
 SysHard_State  g_hard_state;

void system_int()
{
  char state=0;	
  bsp_int();
  state=Sys_Param_Init();	
  switch(state)
	{
	  case 0: FM31256_RTC_Read_Date(&system_date); FM31256_RTC_Read_Time(&system_time);		break;

		case 1: BEEP=1; break;//iic错
		case 2: BEEP=1; break;//无数据		
	}
	LCD_Init();
	Bsp_InitKey();		
  Ht7036_init()	;
  menu_init();	
	EA=1;
}


// 每隔1s读取一次时间
void Read_Time_Per_1s(void) {
		static uint32_t last_read_time = 0;    // 【关键】记录上一次读取时间的时间戳，初始化为0
    uint32_t current_time = Get_SystemTime();  // 安全读取当前时间戳
    uint32_t time_diff = current_time - last_read_time;  // 无符号数自动处理溢出

    // 当时间差≥1000ms时，执行读取操作
    if (time_diff >= 1000) {

			FM31256_RTC_Read_Date(&system_date);
			FM31256_RTC_Read_Time(&system_time);			
      last_read_time = current_time;
			menu_state.timer_1s_flag = 1;
    }
}


unsigned int g_adc_raw_values[ADC_SAMPLE_BUF_SIZE] = {0};
unsigned int g_adc_value = 0;
bit g_adc_data_valid  = 0;
static uint8_t s_adc_buf_index = 0;
uint8_t g_key_press_flag = 0;  // 1=有按键按下，0=无键
unsigned int s_LastAdcValue = 0;  // 新增：记录上一次有效ADC值
/*
*********************************************************************************************************
*   函 数 名: UpdateAdcFilter
*   功能说明: 优化的中值滤波（5点中值，抗干扰更强）
*********************************************************************************************************
*/
static void UpdateAdcFilter(void) {
    unsigned int temp;
    // 核心修改：缓冲区改为static，避免栈溢出（51栈空间小）
    static unsigned int buf[ADC_SAMPLE_BUF_SIZE];
    uint8_t i,j;

    // 1. 复制数据（临界区：仅复制时关中断，排序不关）
    EA = 0;
    for ( i = 0; i < ADC_SAMPLE_BUF_SIZE; i++) {
        buf[i] = g_adc_raw_values[i];
    }
    EA = 1; // 立即开中断，避免丢失ADC采样

    // 2. 冒泡排序（开中断执行，不影响采样）
    for ( i = 0; i < ADC_SAMPLE_BUF_SIZE - 1; i++) {
        for ( j = 0; j < ADC_SAMPLE_BUF_SIZE - i - 1; j++) {
            if (buf[j] > buf[j + 1]) {
                temp = buf[j];
                buf[j] = buf[j + 1];
                buf[j + 1] = temp;
            }
        }
    }

    // 3. 更新有效值（仅更新时关中断）
    EA = 0;
    g_adc_value = buf[ADC_SAMPLE_BUF_SIZE / 2];
    g_adc_data_valid = 1;
    EA = 1;
}

void ADC0_ISR(void) interrupt 15 {
    unsigned int val;
     uint8_t is_valid_key_val = 0;
    if (AD0INT) {
        AD0INT = 0;
        val = (unsigned int)ADC0H << 8 | ADC0L;

        // 新增：先判断当前值是否在任意按键阈值窗口内（快速筛选有效值）

        if ((val >= THRESHOLD_KEY0 - THRESHOLD_WINDOW && val <= THRESHOLD_KEY0 + THRESHOLD_WINDOW) ||
            (val >= THRESHOLD_KEY1 - THRESHOLD_WINDOW && val <= THRESHOLD_KEY1 + THRESHOLD_WINDOW) ||
            (val >= THRESHOLD_KEY2 - THRESHOLD_WINDOW && val <= THRESHOLD_KEY2 + THRESHOLD_WINDOW) ||
            (val >= THRESHOLD_KEY3 - THRESHOLD_WINDOW && val <= THRESHOLD_KEY3 + THRESHOLD_WINDOW) ||
            (val >= THRESHOLD_KEY4 - THRESHOLD_WINDOW && val <= THRESHOLD_KEY4 + THRESHOLD_WINDOW) ||
            (val >= THRESHOLD_KEY5 - THRESHOLD_WINDOW && val <= THRESHOLD_KEY5 + THRESHOLD_WINDOW)) {
            is_valid_key_val = 1;
            g_key_press_flag = 1;
        } else {
            // 无键值：仅当按键标志已置1（松开阶段），才更新缓冲区（避免混值）
            if (g_key_press_flag == 0) {
                is_valid_key_val = 1;
            } else {
                // 按键松开：清空缓冲区，重置标志
                s_adc_buf_index = 0;
                g_key_press_flag = 0;
                return; // 丢弃松开阶段的过渡值
            }
        }

        // 仅有效值才存入缓冲区
        if (is_valid_key_val) {
            EA = 0;
            g_adc_raw_values[s_adc_buf_index] = val;
            s_adc_buf_index++;
            if (s_adc_buf_index >= ADC_SAMPLE_BUF_SIZE) {
                s_adc_buf_index = 0;
                UpdateAdcFilter();  // 缓冲区满则滤波
            }
            EA = 1;
        }
    }
}

//static void UpdateAdcFilter(void) {
//    unsigned int temp;
//    static unsigned int buf[ADC_SAMPLE_BUF_SIZE];
//    uint8_t i,j;
//    unsigned long sum = 0;
//	  unsigned int mid_avg;
//    // 1. 复制数据（临界区：仅复制时关中断）
//    EA = 0;
//    for ( i = 0; i < ADC_SAMPLE_BUF_SIZE; i++) {
//        buf[i] = g_adc_raw_values[i];
//    }
//    EA = 1; // 立即开中断，避免丢失10ms一次的ADC采样

//    // 2. 冒泡排序（中值滤波基础）
//    for ( i = 0; i < ADC_SAMPLE_BUF_SIZE - 1; i++) {
//        for ( j = 0; j < ADC_SAMPLE_BUF_SIZE - i - 1; j++) {
//            if (buf[j] > buf[j + 1]) {
//                temp = buf[j];
//                buf[j] = buf[j + 1];
//                buf[j + 1] = temp;
//            }
//        }
//    }

//    // 优化：中值+均值滤波（去掉最大/最小各1个，取中间6个均值，抗噪更强）
//    // 适配你的80窗口，过滤ADC随机噪声（±10~20LSB）

//    for (i = 1; i < ADC_SAMPLE_BUF_SIZE - 1; i++) {
//        sum += buf[i];
//    }
//     mid_avg = sum / (ADC_SAMPLE_BUF_SIZE - 2);

//    // 3. 更新有效值（仅更新时关中断）
//    EA = 0;
//    g_adc_value = mid_avg;  // 用均值替代单纯中值，避免单点噪声影响
//    s_LastAdcValue = g_adc_value; // 同步更新历史值
//    g_adc_data_valid = 1;
//    EA = 1;
//}
//void ADC0_ISR(void) interrupt 15 {
//    unsigned int val;
//    uint8_t is_valid_key_val = 0;

//    if (AD0INT) {
//        AD0INT = 0;
//        val = (unsigned int)ADC0H << 8 | ADC0L;

//        // 优化1：优先判断无键值（你的硬件无键值为4095，避免松开过渡值误判）
//        if (val < (NO_KEY_THRESHOLD )) {
//            // 无键值：清空缓冲区+重置标志，直接返回
//            s_adc_buf_index = 0;
//            g_key_press_flag = 0;
//            return;
//        }

//        // 优化2：判断是否在你的6个按键阈值窗口内（保留你的原始逻辑）
//        if ((val >= THRESHOLD_KEY0 - THRESHOLD_WINDOW && val <= THRESHOLD_KEY0 + THRESHOLD_WINDOW) ||
//            (val >= THRESHOLD_KEY1 - THRESHOLD_WINDOW && val <= THRESHOLD_KEY1 + THRESHOLD_WINDOW) ||
//            (val >= THRESHOLD_KEY2 - THRESHOLD_WINDOW && val <= THRESHOLD_KEY2 + THRESHOLD_WINDOW) ||
//            (val >= THRESHOLD_KEY3 - THRESHOLD_WINDOW && val <= THRESHOLD_KEY3 + THRESHOLD_WINDOW) ||
//            (val >= THRESHOLD_KEY4 - THRESHOLD_WINDOW && val <= THRESHOLD_KEY4 + THRESHOLD_WINDOW) ||
//            (val >= THRESHOLD_KEY5 - THRESHOLD_WINDOW && val <= THRESHOLD_KEY5 + THRESHOLD_WINDOW)) {
//            is_valid_key_val = 1;
//            g_key_press_flag = 1;
//        } else {
//            // 非按键值：仅空闲状态才标记有效（避免混值）
//            if (g_key_press_flag == 0) {
//                is_valid_key_val = 1;
//            } else {
//                // 按键松开阶段：清空缓冲区，重置标志
//                s_adc_buf_index = 0;
//                g_key_press_flag = 0;
//                return; // 丢弃松开过渡值
//            }
//        }

//        // 仅有效值才存入缓冲区
//        if (is_valid_key_val) {
//            EA = 0;
//            g_adc_raw_values[s_adc_buf_index] = val;
//            s_adc_buf_index++;
//            if (s_adc_buf_index >= ADC_SAMPLE_BUF_SIZE) {
//                s_adc_buf_index = 0;
//                UpdateAdcFilter();  // 缓冲区满则滤波（80ms一次）
//            }
//            EA = 1;
//        }
//    }
//}

