#ifndef _LCD_H_
#define _LCD_H_

#include "config.h"

// 字符类型枚举
typedef enum {
    CHAR_TYPE_8x6,   // 8x6字符，输入6字节，输出24字节
    CHAR_TYPE_16x9,  // 16x9字符，输入24字节，输出80字节
    CHAR_TYPE_16x18  // 16x18字符，输入48字节，输出144字节
} CharType;




// UC1698 LCD接口
sbit LCD_CD = P1^1;   // ����/����ѡ�� (CD)
sbit LCD_CS0 = P1^2;  // Ƭѡ�ź� (/CS0) - ����Ч
sbit LCD_WR = P1^4;   // д�ź� (/WR) - ����Ч
sbit LCD_RST = P1^0;   // ���ź� (/RD) - ����Ч

// LCD (D0-D7)
#define LCD_DATA   P2







// LCD��������
#define LCD_WIDTH  240  // LCD����
#define LCD_HEIGHT 128  // LCD�߶�
#define LCD_PAGES  (LCD_HEIGHT / 8)  // ҳ��


   
#define LCD_CA_MAX       79     
#define LCD_RA_MAX       127  




void LCD_Darker(uint8_t dat) ;
// LCD初始化
void LCD_Init(void);


// LCD�函数
void LCD_Clear(void);


void LCD_DisplayChar(unsigned int x, unsigned char y, const unsigned char* ch, unsigned char disp_size, unsigned char inverse);


/**
 * @brief 显示中英文混合字符串（支持 ASCII + GB2312 双字节汉字）
 * @param x           起始X坐标
 * @param y           起始Y坐标
 * @param str         字符串指针（例如 "Hello自动"）
 * @param disp_size   字符大小: 1=8x6, 2=16x9（影响ASCII和汉字的高度匹配）
 * @param inverse     反显控制
 */
void LCD_DisplayString(unsigned int x, unsigned char y, const char* str, unsigned char disp_size, unsigned char inverse);
void LCD_DisplayChinese(unsigned int x, unsigned char y, const char* str, unsigned char disp_size, unsigned char inverse);


void LCD_DisplayNum(u8 x, u8 y, u32 num, u8 Size, u8 len, u8 offset, u8 Is_Reverse, u8 leading_zero);
void LCD_DisplayFixedPoint(u8 x, u8 y, u16 integer, u16 decimal,u8 int_digits, u8 dec_digits,u8 Size, u8 offset, u8 Is_Reverse);

void LCD_DisplayHex(u8 x, u8 y, u32 num, u8 Size, u8 len, u8 offset, u8 Is_Reverse);
void LCD_DisplayBpm(unsigned int x, unsigned char y, unsigned char rows,char* ch);
void LCD_DisplayLine(unsigned char x, unsigned char y,unsigned char x1,uint8_t type) ;

#endif /* _UC1698_LCD_H_ */