#include "lcd.h"
#include "system.h"

/**
 * @brief LCD显示函数测试主函数
 * @note 此函数包含所有LCD显示函数的测试用例
 */
void LCD_Test(void)
{
	    unsigned char i;
    // 初始化LCD
    LCD_Init();
    Delay_ms(100);
    
    // 1. 测试LCD_Clear函数
    LCD_Clear();
    Delay_ms(500);
    
    // 2. 测试LCD_DrawPixel函数 - 绘制网格
    LCD_Test_Pixel(1);  // 绘制白色网格
    Delay_ms(1500);
    
//    // 2. 测试LCD_DrawPixel函数
//    // 绘制一个矩形边框的像素点

//    for (i = 0; i < 100; i++) {
//        LCD_DrawPixel(20 + i, 20, 1);      // 上边
//        LCD_DrawPixel(20 + i, 70, 1);      // 下边
//        LCD_DrawPixel(20, 20 + i, 1);      // 左边
//        LCD_DrawPixel(120, 20 + i, 1);     // 右边
//    }
//    Delay_ms(1000);
//    
//    // 3. 测试LCD_DrawLine函数
//    // 绘制不同方向的直线
//    LCD_DrawLine(0, 0, 239, 0, 1);         // 顶部水平线
//    LCD_DrawLine(0, 127, 239, 127, 1);     // 底部水平线
//    LCD_DrawLine(0, 0, 0, 127, 1);         // 左侧垂直线
//    LCD_DrawLine(239, 0, 239, 127, 1);     // 右侧垂直线
//    LCD_DrawLine(0, 0, 239, 127, 1);       // 对角线
//    LCD_DrawLine(239, 0, 0, 127, 1);       // 对角线
//    Delay_ms(1000);
//    
//    // 4. 测试LCD_DrawRect函数
//    LCD_Clear();
//    LCD_DrawRect(50, 30, 100, 50, 1);      // 普通矩形
//    LCD_DrawRect(10, 10, 220, 100, 1);     // 大矩形
//    LCD_DrawRect(110, 55, 20, 10, 1);      // 小矩形
//    Delay_ms(1000);
//    
//    // 5. 测试LCD_FillRect函数
//    LCD_Clear();
//    LCD_FillRect(50, 30, 100, 50, 1);      // 填充矩形
//    Delay_ms(500);
//    LCD_FillRect(70, 40, 60, 30, 0);       // 清除部分区域
//    Delay_ms(1000);
//    
    // 6. 测试LCD_DisplayChar函数
    LCD_Clear();
    // 测试不同字体大小和反显
    LCD_DisplayChar(20, 20, 'A', 1, 0);    // 6x8正常显示
    LCD_DisplayChar(40, 20, 'B', 1, 1);    // 6x8反显
    LCD_DisplayChar(20, 40, 'C', 2, 0);    // 16x8正常显示
    LCD_DisplayChar(60, 40, 'D', 2, 1);    // 16x8反显
    LCD_DisplayChar(20, 60, 'E', 3, 0);    // 16x16正常显示
    LCD_DisplayChar(60, 60, 'F', 3, 1);    // 16x16反显
    Delay_ms(1500);
    
    // 7. 测试LCD_DisplayString函数
    LCD_Clear();
    // 测试不同字体大小和反显的字符串
    LCD_DisplayString(20, 20, "Hello 6x8", 1, 0);      // 6x8正常显示
    LCD_DisplayString(20, 35, "Hello 16x8", 2, 0);     // 16x8正常显示
    LCD_DisplayString(20, 55, "Hello 16x16", 3, 0);    // 16x16正常显示
    LCD_DisplayString(20, 80, "Inverse 6x8", 1, 1);    // 6x8反显
    LCD_DisplayString(20, 95, "Inverse 16x8", 2, 1);   // 16x8反显
    Delay_ms(2000);
    
    // 8. 综合测试
    LCD_Clear();
    // 绘制背景
    LCD_FillRect(0, 0, 240, 128, 0);
    
    // 绘制图形
    LCD_FillRect(10, 10, 220, 20, 1);
    LCD_DrawRect(10, 40, 100, 70, 1);
    LCD_FillRect(120, 40, 100, 70, 1);
    
    // 显示文字
    LCD_DisplayString(50, 15, "LCD Display Test", 2, 0);
    LCD_DisplayString(20, 50, "Rectangle", 1, 1);
    LCD_DisplayString(140, 50, "Filled Rect", 1, 0);
    LCD_DisplayString(20, 110, "Test Complete", 2, 1);
    
    // 绘制装饰线条
    LCD_DrawLine(0, 35, 239, 35, 1);
    LCD_DrawLine(0, 115, 239, 115, 1);
}

/**
 * @brief 测试LCD像素绘制功能
 * @param color 像素颜色 (0: 黑色, 非0: 白色)
 */
void LCD_Test_Pixel(unsigned char color)
{
	    unsigned int x;
    unsigned char y;
    LCD_Clear();
    
    // 绘制网格

    
    // 绘制垂直线
    for (x = 0; x < LCD_WIDTH; x += 10) {
        for (y = 0; y < LCD_HEIGHT; y++) {
            LCD_DrawPixel(x, y, color);
        }
    }
    
    // 绘制水平线
    for (y = 0; y < LCD_HEIGHT; y += 10) {
        for (x = 0; x < LCD_WIDTH; x++) {
            LCD_DrawPixel(x, y, color);
        }
    }
    
    // 在网格交点绘制点
    for (x = 0; x < LCD_WIDTH; x += 20) {
        for (y = 0; y < LCD_HEIGHT; y += 20) {
            LCD_DrawPixel(x, y, color);
        }
    }
}

/**
 * @brief 测试LCD文本显示功能
 */
void LCD_Test_Text(void)
{
    LCD_Clear();
    
    // 测试不同字体大小的字符串显示
    LCD_DisplayString(10, 10, "Font Size 1 (6x8)", 1, 0);
    LCD_DisplayString(10, 25, "Font Size 2 (16x8)", 2, 0);
    LCD_DisplayString(10, 45, "Font Size 3 (16x16)", 3, 0);
    
    // 测试反显功能
    LCD_DisplayString(10, 70, "Normal Text", 2, 0);
    LCD_DisplayString(10, 90, "Inverse Text", 2, 1);
    
    // 测试长字符串
    LCD_DisplayString(10, 110, "This is a long string test for LCD display", 1, 0);
}

/**
 * @brief 测试LCD图形绘制功能
 */
void LCD_Test_Graphics(void)
{
    LCD_Clear();
    
    // 绘制不同大小的矩形
    LCD_DrawRect(10, 10, 50, 30, 1);
    LCD_FillRect(70, 10, 50, 30, 1);
    
    LCD_DrawRect(130, 10, 80, 50, 1);
    LCD_FillRect(20, 50, 100, 60, 1);
    
    // 绘制不同方向的直线
    LCD_DrawLine(10, 120, 100, 120, 1);     // 水平线
    LCD_DrawLine(120, 70, 120, 120, 1);     // 垂直线
    LCD_DrawLine(140, 70, 230, 120, 1);     // 斜线
    LCD_DrawLine(230, 70, 140, 120, 1);     // 斜线
    
    // 绘制嵌套矩形
    LCD_DrawRect(150, 20, 80, 40, 1);
    LCD_DrawRect(160, 30, 60, 20, 1);
    LCD_DrawRect(170, 35, 40, 10, 1);
}