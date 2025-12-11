
#ifndef __CONFIG_H
#define __CONFIG_H

//========================================================================
//                               主时钟定义
//========================================================================

// #define MAIN_Fosc 30000000L // 定义主时钟
  #define MAIN_Fosc	 22118400L	//定义主时钟
//  #define MAIN_Fosc		12000000L	//定义主时钟
//  #define MAIN_Fosc		11059200L	//定义主时钟
//  #define MAIN_Fosc		 5529600L	//定义主时钟
//#define MAIN_Fosc 24000000L // 定义主时钟

//========================================================================
//                                头文件
//========================================================================

#include "Type_def.h"
#include "C8051F120.h"
#include <stdlib.h>
#include <stdio.h>
#include "math.h"
#include <string.h>
#include <intrins.h>
#include "uart.h"
#include "delay.h"
#include "modbus.h"
#include  "bsp.h"
#include "system.h"
#include "lcd.h"
#include "font_date.h"

sbit TF2H= TMR2CN^7;//定时器2标志

sbit BEEP=P1^6;//蜂鸣器


 



#define VERSION "20251118"
#define PASSWORD_DEFAULT 202320
#define PI 3.1415
#define UNUSED(expr) if ((expr) == 0)


typedef unsigned char bool;





/***********************************************
 * 描述： 用户自定义结构体
 */


#endif
