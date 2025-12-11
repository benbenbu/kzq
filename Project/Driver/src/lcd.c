#include "lcd.h"
#include "font_date.h"


//// LCD硬件复位
void LCD_Reset() {
    // 复位时序
    LCD_RST = 0;
    Delay_ms(5);	
    LCD_RST = 1;	
    Delay_ms(200);
}


// LCD写数据函数
void LCD_WriteData(unsigned char write_data) {
    LCD_CS0 = 0;       // 选择LCD
    LCD_CD = 1;        // 设置为数据模式
    LCD_WR = 0;        // 拉低写信号	
    LVC245_OE = 0;      // 使能LVC245		
    LCD_DATA = write_data;   // 输出数据
    Delay_us(1);       // 延迟时间
    LCD_WR = 1;        // 拉高写信号
//    LVC245_OE = 1;      // 禁用LVC245	
    LCD_CS0 = 1;       // 取消选择LCD
}

// LCD写命令函数
void LCD_WriteCmd(unsigned char cmd) {
    LCD_CS0 = 0;       // 选择LCD
    LCD_CD = 0;        // 设置为命令模式
    LCD_WR = 0;        // 拉低写信号    
    LVC245_OE = 0;      // 使能LVC245    
    LCD_DATA = cmd;    // 输出命令
    Delay_us(1);       // 延迟时间
    LCD_WR = 1;        // 拉高写信号
//    LVC245_OE = 1;      // 禁用LVC245
    LCD_CS0 = 1;       // 取消选择LCD
}

// LCD读数据函数
unsigned char LCD_ReadData(void) {
    unsigned char read_data;
    
    LCD_CS0 = 0;       // 选择LCD
    LCD_CD = 1;        // 设置为数据模式
    LCD_WR = 0;        // 拉低读信号    
    LVC245_OE = 0;      // 使能LVC245    
    Delay_us(1);       // 延迟时间
    read_data = LCD_DATA;   // 读取数据
    LCD_WR = 1;        // 拉高读信号
//    LVC245_OE = 1;      // 禁用LVC245    
    LCD_CS0 = 1;       // 取消选择LCD
    
    return read_data;
}


// 扫描整个LCD屏幕并填充数据
void lcdscan(unsigned char pixel_data)
{
	unsigned int  i;
    // 设置整个屏幕的起始地址
    LCD_WriteCmd(0x60);  // 起始行 LSB
    LCD_WriteCmd(0x70);  // 起始行 MSB  
    LCD_WriteCmd(0x00);  // 起始列 LSB
    LCD_WriteCmd(0x10);  // 起始列 MSB
    
    // 启用自动寻址模式（UC1698支持）
    LCD_WriteCmd(0x89);  // 自动寻址模式
    
    // 循环写入所有像素数据 (15,360 字节)
    for ( i = 0; i < 15360; i++) {
        LCD_WriteData(pixel_data);
    }
    
    LCD_WriteCmd(0x88);  // 停止自动寻址
}

// LCD初始化函数
void LCD_Init() {




	    LCD_CS0 = 0;       // 选择LCD
      LCD_Reset();	
	    LCD_WriteCmd(0xe2);			//内部指令复位
		Delay_ms(5);
		LCD_WriteCmd(0xeb);			//(26) 1/12电压比，硬件设计必须的
		LCD_WriteCmd(0x2b);			//(6)  启用内部DC-DC
		LCD_WriteCmd(0x25);			//(5)  温度补偿系数，根据实际使用环境调整
		LCD_WriteCmd(0x81);			//(10) 对比度设置命令，第二字节为对比度值，0~255
		LCD_WriteCmd(0X49);

		/*显示控制*/
		LCD_WriteCmd(0xa4);			//(15)  启用或禁用全显示
		LCD_WriteCmd(0xa6);			//(16)  正向显示(0xa7为反向显示)

		/*LCD控制*/
		LCD_WriteCmd(0xc4);			//(18) COM扫描方向设置为COM160-C0, SEG扫描方向设置为SEG1-S240
		LCD_WriteCmd(0xa1);			//(14)  SEG方向设置
		LCD_WriteCmd(0xd1);			//(20) RGB-RGB-RGB-.....格式
		LCD_WriteCmd(0xd5);			//(21) 4k颜色模式，RRRR-GGGG-BBBB-RRRR-GGGG-BBBB
		LCD_WriteCmd(0x84);			//(11) 关闭部分显示


		/*(19)n-line inversion*/
		LCD_WriteCmd(0xc8);
		LCD_WriteCmd(0x10);			//设置n-line inversion参数











		/*显示区域*/
		LCD_WriteCmd(0xf4);			//(30) 起始列
		LCD_WriteCmd(0x00);			//start from 0
		LCD_WriteCmd(0xf6);			//(32) 结束列
		LCD_WriteCmd(0x4f);			//end:240

		LCD_WriteCmd(0xf5);			//(31) 起始行
		LCD_WriteCmd(0x00);			// 0
		LCD_WriteCmd(0xf7);			//(32) 结束行
		LCD_WriteCmd(0x7F);			// 128

		LCD_WriteCmd(0xf8);			//(34) 扫描设置，显示区域在扫描方向上自动反转1

		LCD_WriteCmd(0x89);			//(12) 启用自动增量，在扫描方向上自动增量1 			
		LCD_WriteCmd(0xad);			//(17) 设置显示模式，无灰度级的ON-OFF模式(正常显示模式)

		/*scroll line 与 RAM 映射关系*/
		LCD_WriteCmd(0x40);			//(8) 设置起始行
		LCD_WriteCmd(0x50);			//     无特殊意义
		LCD_WriteCmd(0xc4);			//(18) COM扫描方向设置为COM160-C0, SEG扫描方向设置为SEG1-S240
		LCD_WriteCmd(0x90);			//(13):  无特殊标记
		LCD_WriteCmd(0x00);

		/*设置显示区域*/
		LCD_WriteCmd(0x84);			//(11) 关闭部分显示
		LCD_WriteCmd(0xf1);			//(27) 设置结束行
		LCD_WriteCmd(0x9f);			// 160
		LCD_WriteCmd(0xf2);			//(28) 设置起始行
		LCD_WriteCmd(0);			//0
		LCD_WriteCmd(0xf3);			// (29) 设置结束列
		LCD_WriteCmd(127);			// 128		
		LCD_WriteCmd(0xad);			//(17) 设置显示模式，无灰度级的ON-OFF模式(正常显示模式)
}



// 设置显示位置
void LCD_SetPosition(unsigned char page, unsigned int column) {
    // 设置页地址
//    LCD_WriteCmd(UC1698_SET_PAGE_ADDR | (page & 0x0F));
//    
//    // 设置列地址
//    LCD_WriteCmd(UC1698_SET_COLUMN_ADDR_L | (column & 0x0F));           // 低4位
//    LCD_WriteCmd(UC1698_SET_COLUMN_ADDR_H | ((column >> 4) & 0x0F));   // 高4位
}



// 清空LCD屏幕
void LCD_Clear(void) {
    lcdscan(0x00);  // 向整个屏幕写入0x00，实现清屏效果
}

// 在指定位置绘制一个像素点
// x: X坐标 (0-239)
// y: Y坐标 (0-127)
// color: 颜色 (0: 黑色, 非0: 白色)
void LCD_DrawPixel(unsigned int x, unsigned char y, unsigned char color) {
    unsigned char col_lsb, col_msb;
    unsigned char row_lsb, row_msb;
    unsigned int column;
    unsigned char pixel_pos;
    unsigned char pixel_data[3];
    
    // 检查坐标是否超出范围
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) {
        return;
    }
    
    // 计算行地址（Y坐标反转）
    y = 127 - y;  // 反转Y坐标，使(0,0)位于左上角
    row_lsb = (y & 0x0F);  // 行地址低4位
    row_msb = ((y >> 4) & 0x0F);  // 行地址高4位
    
    // 计算列地址 - UC1698的4k颜色模式是每2个像素占用3个字节
    // 每个像素使用12位颜色（4位R，4位G，4位B）
    column = (x / 2) * 3;  // 列地址是字节索引，每2个像素对应3个字节
    pixel_pos = x % 2;  // 像素在2个像素中的位置（0或1）
    
    // 计算列地址的低4位和高4位
    col_lsb = (column & 0x0F);
    col_msb = ((column >> 4) & 0x0F);
    
    // 设置RAM地址控制
    LCD_WriteCmd(0x89);
    
    // 设置RAM地址 - 注意顺序：列地址低4位 -> 列地址高4位 -> 行地址低4位 -> 行地址高4位
    LCD_WriteCmd(col_lsb);  // 列地址低4位
    LCD_WriteCmd(0x10 | col_msb);  // 列地址高4位
    LCD_WriteCmd(0x60 | row_lsb);  // 行地址低4位
    LCD_WriteCmd(0x70 | row_msb);  // 行地址高4位
    
    // 读取当前的3个字节数据
    LCD_ReadData();  // 虚拟读取
    pixel_data[0] = LCD_ReadData();  // 读取第1个字节
    pixel_data[1] = LCD_ReadData();  // 读取第2个字节
    pixel_data[2] = LCD_ReadData();  // 读取第3个字节
    
    // 根据像素位置和颜色修改数据
    // UC1698的4k颜色模式：RRRR-GGGG-BBBB-RRRR-GGGG-BBBB
    // 字节1：像素0的R[3:0]（高4位），像素0的G[3:0]（低4位）
    // 字节2：像素0的B[3:0]（高4位），像素1的R[3:0]（低4位）
    // 字节3：像素1的G[3:0]（高4位），像素1的B[3:0]（低4位）
    if (color != 0) {
        // 白色：4位R, 4位G, 4位B都为1
        if (pixel_pos == 0) {
            // 第一个像素（高12位）
            pixel_data[0] |= 0xF0;  // R[3:0] 位于第1个字节的高4位
            pixel_data[0] |= 0x0F;  // G[3:0] 位于第1个字节的低4位
            pixel_data[1] |= 0xF0;  // B[3:0] 位于第2个字节的高4位
        } else {
            // 第二个像素（低12位）
            pixel_data[1] |= 0x0F;  // R[3:0] 位于第2个字节的低4位
            pixel_data[2] |= 0xF0;  // G[3:0] 位于第3个字节的高4位
            pixel_data[2] |= 0x0F;  // B[3:0] 位于第3个字节的低4位
        }
    } else {
        // 黑色：4位R, 4位G, 4位B都为0
        if (pixel_pos == 0) {
            // 第一个像素（高12位）
            pixel_data[0] &= 0x0F;  // R[3:0] 位于第1个字节的高4位
            pixel_data[0] &= 0xF0;  // G[3:0] 位于第1个字节的低4位
            pixel_data[1] &= 0x0F;  // B[3:0] 位于第2个字节的高4位
        } else {
            // 第二个像素（低12位）
            pixel_data[1] &= 0xF0;  // R[3:0] 位于第2个字节的低4位
            pixel_data[2] &= 0x0F;  // G[3:0] 位于第3个字节的高4位
            pixel_data[2] &= 0xF0;  // B[3:0] 位于第3个字节的低4位
        }
    }
    
    // 重新设置RAM地址
    LCD_WriteCmd(0x89);
    LCD_WriteCmd(col_lsb);  // 列地址低4位
    LCD_WriteCmd(0x10 | col_msb);  // 列地址高4位
    LCD_WriteCmd(0x60 | row_lsb);  // 行地址低4位
    LCD_WriteCmd(0x70 | row_msb);  // 行地址高4位
    
    // 写入修改后的数据（3个字节）
    LCD_WriteData(pixel_data[0]);
    LCD_WriteData(pixel_data[1]);
    LCD_WriteData(pixel_data[2]);
    
    // 停止自动地址递增
    LCD_WriteCmd(0x88);
}

// 在两点之间绘制一条直线
// x1, y1: 起始坐标
// x2, y2: 结束坐标
// color: 颜色 (0: 黑色, 非0: 白色)
void LCD_DrawLine(unsigned int x1, unsigned char y1, unsigned int x2, unsigned char y2, unsigned char color) {
    int dx, dy, sx, sy, err, e2;
    
    // 计算x和y方向的差值
    dx = (x1 < x2) ? (x2 - x1) : (x1 - x2);
    dy = (y1 < y2) ? (y2 - y1) : (y1 - y2);
    
    // 确定步进方向
    sx = (x1 < x2) ? 1 : -1;
    sy = (y1 < y2) ? 1 : -1;
    
    // 误差值
    err = dx - dy;
    
    // 绘制每个像素点
    while (1) {
        // 绘制当前像素点
        LCD_DrawPixel(x1, y1, color);
        
        // 检查是否到达终点
        if (x1 == x2 && y1 == y2) {
            break;
        }
        
        // 更新误差值
        e2 = 2 * err;
        
        // 更新x坐标
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        
        // 更新y坐标
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

// 绘制一个矩形边框
// x, y: 左上角坐标
// width: 矩形宽度
// height: 矩形高度
// color: 颜色 (0: 黑色, 非0: 白色)
void LCD_DrawRect(unsigned int x, unsigned char y, unsigned int width, unsigned char height, unsigned char color) {
    unsigned int x2, i;
    unsigned char y2, j;
    
    // 计算右下角坐标
    x2 = x + width - 1;
    y2 = y + height - 1;
    
    // 检查坐标是否超出范围
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT || x2 >= LCD_WIDTH || y2 >= LCD_HEIGHT) {
        return;
    }
    
    // 绘制上边
    LCD_DrawLine(x, y, x2, y, color);
    
    // 绘制右边
    LCD_DrawLine(x2, y, x2, y2, color);
    
    // 绘制下边
    LCD_DrawLine(x2, y2, x, y2, color);
    
    // 绘制左边
    LCD_DrawLine(x, y2, x, y, color);
}

// 绘制一个填充矩形
// x, y: 左上角坐标
// width: 矩形宽度
// height: 矩形高度
// color: 颜色 (0: 黑色, 非0: 白色)
void LCD_FillRect(unsigned int x, unsigned char y, unsigned int width, unsigned char height, unsigned char color) {
    unsigned int x2, i;
    unsigned char y2, j;
    
    // 计算右下角坐标
    x2 = x + width - 1;
    y2 = y + height - 1;
    
    // 检查坐标是否超出范围
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT || x2 >= LCD_WIDTH || y2 >= LCD_HEIGHT) {
        return;
    }
    
    // 绘制每个像素点
    for (j = y; j <= y2; j++) {
        for (i = x; i <= x2; i++) {
            LCD_DrawPixel(i, j, color);
        }
    }
}


// 扩展的words函数，支持多种字体和反显
// 功能说明：
// x: 起始x坐标
// y: 起始y坐标
// type: 字体类型 (1:6x8, 2:16x8, 3:16x16汉字)
// ch: 要显示的字符
// inverse: 是否反显 (0:正常, 1:反显)
void words(unsigned char x, unsigned char y, unsigned char type, unsigned char inverse, unsigned char ch) {
    unsigned char *p;
    unsigned char i, k, j, m, n, l, x0;
    unsigned char dat0, dat1, dat2, dat3, dat4, dat5, dat6;
    unsigned char dat6_col7, dat7_col8, dat8_col9;  // 用于16x9字体显示时的变量
    unsigned char inv_mask = inverse ? 0xFF : 0x00;  // 反显掩码
	
    x = 0x00 + x;  // 每个地址对应3个像素点
    x0 = 0x00 | (x & 0x0F);
    x = 0x10 | ((x & 0xF0) >> 4);
    
    // 根据字体类型和字符选择对应的字模数据
    if (type == 1) {  // 6x8字体
        p = (unsigned char*)F8X6[ch - 32];
    } else if (type == 2) {  // 16x8字体
        p = (unsigned char*)F16X9[ch - 32];
    } else if (type == 3) {  // 16x16汉字
        // 16x16汉字，从Hzk数组中获取字模数据
        // 由于Hzk是FONT_DATA类型的数组，需要取dat字段
        p = (unsigned char*)Hzk[ch - 32].dat;
    }
    
    // 6x8字体 (F8X6)
    if (type == 1) {
       // 6x8字体：1页，每页6字节
        for (i = 0; i < 1; i++) {
            n = i * 6;  // 每页6字节
            
            for (j = 0; j < 8; j++) {  // 每页8行
                m = i * 8 + j;
                LCD_WriteCmd(0x89);
                LCD_WriteCmd(x0);
                LCD_WriteCmd(x);
                LCD_WriteCmd(0x60 | ((y + m) & 0x0F));
                LCD_WriteCmd(0x70 | (((y + m) & 0xF0) >> 4));
                
                // 6x8: 每页1次循环 (6字节)
                for (k = 0; k < 1; k++) {
                    l = k * 6 + n;  // 每次处理6字节
                    
                    dat6 = 0x01 << j;
                    
                    // 第1-2字节处理 (6x8只需要6字节，转换为3个字节)
                    dat0 = (*(p + l)) & dat6;
                    dat0 = dat0 >> j;
                    dat0 <<= 7;
                    
                    dat1 = (*(p + l + 1)) & dat6;
                    dat1 = dat1 >> j;
                    dat1 <<= 3;
                    
                    dat2 = (*(p + l + 2)) & dat6;
                    dat2 = dat2 >> j;
                    dat2 <<= 7;
                    
                    dat3 = (*(p + l + 3)) & dat6;
                    dat3 = dat3 >> j;
                    dat3 <<= 3;
                    
                    dat4 = (*(p + l + 4)) & dat6;
                    dat4 = dat4 >> j;
                    dat4 <<= 7;
                    
                    dat5 = (*(p + l + 5)) & dat6;
                    dat5 = dat5 >> j;
                    dat5 <<= 3;
                    
                    // 写入数据到相应位置
                    LCD_WriteData((dat0 | dat1) ^ inv_mask);
                    LCD_WriteData((dat2 | dat3) ^ inv_mask);
                    LCD_WriteData((dat4 | dat5) ^ inv_mask);
                }
                LCD_WriteCmd(0x88);
            }
        }
    }
    // 16x9字体 (F16X9)
    else if (type == 2) {
        // 参考UC1698C语言.c中的数据结构实现16x9字体显示
        // 16x9字体：每个字符9列16行，共18字节数据
        // 16行分为2页，每页8行，每页9字节数据
        for (i = 0; i < 2; i++) {  // 16行分为2页，每页8行
            n = i * 9;  // 每页9字节数据
            
            for (j = 0; j < 8; j++) {  // 每页8行
                m = i * 8 + j;  // 当前页偏移
                
                // 设置RAM地址
                LCD_WriteCmd(0x89);
                LCD_WriteCmd(x0);
                LCD_WriteCmd(x);
                LCD_WriteCmd(0x60 | ((y + m) & 0x0F));  // 行地址 LSB
                LCD_WriteCmd(0x70 | (((y + m) & 0xF0) >> 4));  // 行地址 MSB
                
                dat6 = 0x01 << j;  // 取数据对应位
                
                // 处理第1-3列，对应3字节数据
                dat0 = (*(p + n)) & dat6;
                dat0 = dat0 >> j;
                dat0 <<= 7;
                
                dat1 = (*(p + n + 1)) & dat6;
                dat1 = dat1 >> j;
                dat1 <<= 3;
                
                dat2 = (*(p + n + 2)) & dat6;
                dat2 = dat2 >> j;
                dat2 <<= 7;
                
                // 处理第4-6列，对应3字节数据
                dat3 = (*(p + n + 3)) & dat6;
                dat3 = dat3 >> j;
                dat3 <<= 3;
                
                dat4 = (*(p + n + 4)) & dat6;
                dat4 = dat4 >> j;
                dat4 <<= 7;
                
                dat5 = (*(p + n + 5)) & dat6;
                dat5 = dat5 >> j;
                dat5 <<= 3;
                
                // 处理第7-9列，对应3字节数据
                dat6_col7 = (*(p + n + 6)) & dat6;
                dat6_col7 = dat6_col7 >> j;
                
                dat7_col8 = (*(p + n + 7)) & dat6;
                dat7_col8 = dat7_col8 >> j;
                
                dat8_col9 = (*(p + n + 8)) & dat6;
                dat8_col9 = dat8_col9 >> j;
                
                // 将9列数据转换为UC1698需要的格式
                // 按照UC1698的存储格式，每2列合并为1个字节
                LCD_WriteData((dat0 | dat1) ^ inv_mask);  // 第1-2列
                LCD_WriteData((dat2 | dat3) ^ inv_mask);  // 第3-4列，修改了第4列的问题
                LCD_WriteData((dat4 | dat5) ^ inv_mask);  // 第5-6列
                // 第7-8列合并为一个字节，修改了第7列的问题
                LCD_WriteData(((dat6_col7 << 7) | (dat7_col8 << 3)) ^ inv_mask);
                // 第9列为单独的一个字节
                LCD_WriteData((dat8_col9 << 7) ^ inv_mask);
                
                LCD_WriteCmd(0x88);  // 结束当前行的写入
            }
        }
    }
    // 16x16汉字 (Hzk)
    else if (type == 3) {
        // 16x16汉字：2页 (16/8=2), 每页16字节
        for (i = 0; i < 2; i++) {
            n = i * 16;  // 每页16字节
            
            for (j = 0; j < 8; j++) {  // 每页8行
                m = i * 8 + j;
                LCD_WriteCmd(0x89);
                LCD_WriteCmd(x0);
                LCD_WriteCmd(x);
                LCD_WriteCmd(0x60 | ((y + m) & 0x0F));
                LCD_WriteCmd(0x70 | (((y + m) & 0xF0) >> 4));
                
                // 16x16: 每页4次循环 (16字节/4=4)
                for (k = 0; k < 4; k++) {
                    l = k * 4 + n;  // 每次处理4字节
                    
                    dat6 = 0x01 << j;
                    
                    // 第1-2字节处理
                    dat0 = (*(p + l)) & dat6;
                    dat0 = dat0 >> j;
                    dat0 <<= 7;
                    
                    dat1 = (*(p + l + 1)) & dat6;
                    dat1 = dat1 >> j;
                    dat1 <<= 3;
                    
                    // 第3-4字节处理
                    dat2 = (*(p + l + 2)) & dat6;
                    dat2 = dat2 >> j;
                    dat2 <<= 7;
                    
                    dat3 = (*(p + l + 3)) & dat6;
                    dat3 = dat3 >> j;
                    dat3 <<= 3;
                    
                    // 写入数据到相应位置
                    LCD_WriteData((dat0 | dat1) ^ inv_mask);
                    LCD_WriteData((dat2 | dat3) ^ inv_mask);
                }
                LCD_WriteCmd(0x88);
            }
        }
    }
}

// 在指定位置显示一个字符
// x: 起始x坐标
// y: 起始y坐标
// ch: 要显示的字符
// disp_size: 显示大小(1=6x8点阵, 2=16x8点阵, 3=16x16点阵)
// inverse: 反显控制(0=正常显示, 1=反显)
void LCD_DisplayChar(unsigned int x, unsigned char y, char ch, unsigned char disp_size, unsigned char inverse) {
    unsigned char uc_ch = (unsigned char)ch;
    
    // 直接调用words函数显示单个字符
    words(x, y, disp_size, inverse, uc_ch);
}

// 在指定位置显示一个字符串
// x: 起始x坐标
// y: 起始y坐标
// str: 要显示的字符串
// disp_size: 显示大小(1=6x8点阵, 2=16x8点阵, 3=16x16点阵)
// inverse: 反显控制(0=正常显示, 1=反显)
void LCD_DisplayString(unsigned int x, unsigned char y, char *str, unsigned char disp_size, unsigned char inverse) {
    unsigned int char_width = 0;
    unsigned int current_x = x;
    unsigned char i = 0;
    
    // ??????????????????????
    switch (disp_size) {
        case 1:  // 6??8????
            char_width = 6;
            break;
        case 2:  // 16??8????
            char_width = 9;  // 16??9????????????9????
            break;
        case 3:  // 16??16????
            char_width = 16;
            break;
        default:
            return;
    }
    
    // ??????????????
    while (str[i] != '\0') {
        // ?????????
        LCD_DisplayChar(current_x, y, str[i], disp_size, inverse);
        
        // ??????????????λ??
        current_x += char_width + 1;  // ???????1????
        
        // ??????????????
        if (current_x >= LCD_WIDTH) {
            break;
        }
        
        i++;
    }
}





















