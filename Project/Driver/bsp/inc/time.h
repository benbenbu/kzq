#ifndef _TIMER_H
#define _TIMER_H

#include "Type_def.h"


// 函数声明

/**
 * @brief 定时器初始化函数
 * @param 无
 * @return 无
 */
void Timer0_Init(void);
void Timer2_Init(void);
void Timer3_Init(void);

/**
 * @brief 启动指定定时器
 * @param timer_num 定时器编号 (0-4)
 * @return 无
 */
void Timer_Start(uint8_t timer_num);

uint32_t Get_SystemTime(void) ;



#endif // _C8051F120_TIMER_H