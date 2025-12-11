#ifndef __FONT_DATE_H_
#define __FONT_DATE_H_


#include "config.h"


/*中文字符字节宽度*/
#define CHN_CHAR_WIDTH 1   // UTF-8编码格式给3，GB2312编码格式给2

/*字模基本单元*/
typedef struct 
{
    char txt[CHN_CHAR_WIDTH + 1];          // 汉字索引
    unsigned char dat[32];                      // 字模数据

} FONT_DATA;



/*ASCII字模数据声明*/
extern const u8 code F8X6[][6];
extern const u8 code F16X9[][18];

/*汉字字模数据声明*/

extern const FONT_DATA code Hzk[];

/*图像数据声明*/



#endif