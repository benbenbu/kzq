#ifndef _LCD_H_
#define _LCD_H_

#include "config.h"





// UC1698 LCD接口
sbit LCD_CD = P1^1;   // ����/����ѡ�� (CD)
sbit LCD_CS0 = P1^2;  // Ƭѡ�ź� (/CS0) - ����Ч
sbit LCD_WR = P1^4;   // д�ź� (/WR) - ����Ч

sbit LCD_RST = P1^0;   // ���ź� (/RD) - ����Ч

// LCD (D0-D7)
#define LCD_DATA   P2



sbit LVC245_OE = P1^7;//LVC245  OE









// UC1698�����
#define UC1698_SYSTEM_RESET      0xE2  // ϵͳ��λ
#define UC1698_SET_LCD_BIAS      0xE8  // ����LCDƫѹ��
#define UC1698_POWER_CONTROL     0x28  // ����LCDƫѹ��
#define UC1698_SET_VBIAS         0x81  // ���öԱȶ�
#define UC1698_LCD_MAP           0xC0  // LCDӳ����� LC(BIT0-BIT2)
#define UC1698_LCD_LINE_RATE     0xA0  // LCD ��Ƶ  LC(BIT3-BIT4)
#define UC1698_LCD_RGB           0xD0  // LCD RGBģʽ LC(BIT5)
#define UC1698_LCD_COLOR         0xD4  // LCD ɫ������ LCD(BIT6-BIT7)
#define UC1698_LCD_DISPLAY       0x84  // LCD ��ʾ���� bit8
#define UC1698_SET_COM           0xD8  // ����COM����
#define UC1698_WINDOWS_MODE      0xF8  // ���ڲ���ģʽ
#define UC1698_RAM_ADDRESS       0x88  // RAM ��ַ����



#define UC1698_DISP_ON           0xA4  // ��ʾ��
#define UC1698_DISP_MODE         0xA6  // ���Ի���
#define UC1698_SET_TEMP          0X24  // �²�����

// LCD��������
#define LCD_WIDTH  240  // LCD����
#define LCD_HEIGHT 128  // LCD�߶�
#define LCD_PAGES  (LCD_HEIGHT / 8)  // ҳ��







// LCD定位

void LCD_SetPosition(unsigned char page, unsigned int column);

// LCD初始化
void LCD_Init(void);


// LCD�函数
void LCD_Clear(void);
void LCD_DrawPixel(unsigned int x, unsigned char y, unsigned char color);
void LCD_DrawLine(unsigned int x1, unsigned char y1, unsigned int x2, unsigned char y2, unsigned char color);
void LCD_DrawRect(unsigned int x, unsigned char y, unsigned int width, unsigned char height, unsigned char color);
void LCD_FillRect(unsigned int x, unsigned char y, unsigned int width, unsigned char height, unsigned char color);

// LCD字符显示函数 - 支持反显功能
// disp_size: 1(6×8), 2(16×8)
// inverse: 0(正常显示), 1(反显)
void LCD_DisplayChar(unsigned int x, unsigned char y, char ch, unsigned char disp_size, unsigned char inverse);
void LCD_DisplayString(unsigned int x, unsigned char y, char *str, unsigned char disp_size, unsigned char inverse);


// LCD测试函数
void LCD_Test(void);
void LCD_Test_Pixel(unsigned char color);
void LCD_Test_Text(void);
void LCD_Test_Graphics(void);



#endif /* _UC1698_LCD_H_ */