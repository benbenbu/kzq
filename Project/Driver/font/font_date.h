#ifndef __FONT_DATE_H_
#define __FONT_DATE_H_


#include "config.h"





// F8X6 
typedef struct 
{
    char txt;          
    unsigned char dat[24];                     

} FONT_DATA_F8x6;
extern const FONT_DATA_F8x6 code F8X6[];
extern const unsigned char F8x6_COUNT;  // 声明为外部常量


// F16X9 
typedef struct 
{
    char txt;          
    unsigned char dat[72];                     

} FONT_DATA_F16x9;
extern const FONT_DATA_F16x9 code F16X9[];
extern const unsigned char F16x9_COUNT;  // 声明为外部常量


typedef struct 
{
    char txt[2];          
    unsigned char dat[144];                     

} FONT_DATA;
extern const FONT_DATA code Hzk[];


extern const unsigned int HZK_COUNT;  // 声明为外部常量



extern const u8  code  PICTURE[];  //互感器图	

extern const u8  code  PICTURE1[];//电容无填充	
extern const u8  code  PICTURE2[];//电容无填充	
#endif