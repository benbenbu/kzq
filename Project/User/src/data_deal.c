

#include "config.h"
#include "data_deal.h"
#include "data_save.h"
#include "math.h"
#include "HT7036.h"


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


uint16_t CRC16_Calc(u8 *pucBuff, u8 unNum)
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
    return uchCRCHi << 8 | uchCRCLo;
}



// ====================== 全局配置宏 ======================
#define REG_24_MAX      0x100000L   // 2^24，24位补码最大值
// 基准系数（根据实际硬件校准，示例值）
#define BASE_W          500.0f      // 有功/无功基准
#define g_53_w          53652.0f    // 有功/无功除数
#define BASE_U          220.0f      // 电压基准
#define g_53_u          1000.0f     // 电压除数
#define BASE_I          10.0f       // 电流基准
#define g_53_i          100.0f      // 电流除数
#define BASE_COS        1.0f        // 功率因数基准
#define g_53_cos        1.0f        // 功率因数除数
// 单位定义
#define UNIT_K          0           // k单位（kVar/KW/KV）
#define UNIT_M          1           // m单位（mVar/mW）
// 溢出阈值（100m=100,000,000）
#define OVERFLOW_M_VAL  100000000.0f





// ====================== 基础函数：24位寄存器转有符号值 ======================
int32_t reg24_to_preg(uint32_t reg_val) {
    if (reg_val > 0x7FFFFFL) {
        return (int32_t)(reg_val - REG_24_MAX);
    } else {
        return (int32_t)reg_val;
    }
}

// ====================== 数据转换函数：meter_data ======================
// 参数说明：
// meter_data：24位寄存器原始值
// type      ：1=无功(Q) 2=有功(P) 3=电压(U) 4=电流(I) 5=功率因数(COS)
// data_deal ：输出放大后的数据（Q/P×100、U×100、I×10、COS×1000）
// unit      ：输出单位（仅Q/P有效：0=K 1=M）
// f         ：输出符号位（仅Q/P有效：0=无负号 1=有负号）
void meter_data(uint32_t meter_data, uint8_t type, uint16_t *data_deal, uint8_t *unit, uint8_t *f) {
    float dat = 0.0f;
    uint32_t abs_dat = 0;  // 绝对值
    int32_t preg = reg24_to_preg(meter_data); // 24位转有符号

    // 初始化输出参数（避免脏数据）
    *data_deal = 0;
    *unit = UNIT_K;
    *f = 0;

    switch(type) {
        // ========== 1=无功(Q) 2=有功(P)：×100，带符号，k/m单位 ==========
        case 1:
        case 2: {
            // 核心运算（浮点截断，无四舍五入）
            dat = (float)preg * BASE_W / g_53_w;
            dat *= (float)g_sys_pt_ct.pt_ratio * ((float)g_sys_pt_ct.ct_ratio / 10) / (g_sys_pt_ct.ct_ratio % 10) / 10;
            
            // 符号位
            *f = (dat < 0.0f) ? 1 : 0;
            // 绝对值
            abs_dat = (uint32_t)(dat < 0.0f ? -dat : dat);

            // 单位判断+溢出处理（≥100m显示9999，M单位）
            if (abs_dat >= OVERFLOW_M_VAL) {
                *data_deal = 9999;  // 99.99（×100后）
                *unit = UNIT_M;
            } else if (abs_dat >= 1000000) { // ≥1m → M单位，×100
                *data_deal = (uint16_t)(abs_dat / 10000); // ÷1e6×100 = ÷10000
                *unit = UNIT_M;
            } else if (abs_dat >= 1000) { // ≥1k → K单位，×100
                *data_deal = (uint16_t)(abs_dat / 10); // ÷1e3×100 = ÷10
                *unit = UNIT_K;
            } else { // <1k → K单位，×100
                *data_deal = (uint16_t)(abs_dat * 100);
                *unit = UNIT_K;
            }
            break;
        }

        // ========== 3=电压(U)：×100，固定KV单位，无符号 ==========
        case 3: {
            dat = (float)preg * BASE_U / g_53_u;
            dat *= (float)g_sys_pt_ct.pt_ratio / 10;
            // 绝对值+截断×100
            abs_dat = (uint32_t)(dat < 0.0f ? -dat : dat);
            *data_deal = (uint16_t)(abs_dat * 100); // ×100（00.00格式）
            *unit = UNIT_K;  // 固定KV
            *f = 0;          // 无负号
            break;
        }

        // ========== 4=电流(I)：×10，固定A单位，无符号 ==========
        case 4: {
            dat = (float)preg * BASE_I / g_53_i;
            dat *= ((float)g_sys_pt_ct.ct_ratio / 10) / (g_sys_pt_ct.ct_ratio % 10) / 100;
            // 绝对值+截断×10
            abs_dat = (uint32_t)(dat < 0.0f ? -dat : dat);
            *data_deal = (uint16_t)(abs_dat * 10); // ×10（000.0格式）
            *unit = 0;       // 无单位（仅A）
            *f = 0;          // 无负号
            break;
        }

        // ========== 5=功率因数(COS)：×1000，0~1范围，无符号 ==========
        case 5: {
            dat = (float)preg * BASE_COS / g_53_cos;
            // 范围限制0~1，截断×1000
            if (dat > 1.0f) dat = 1.0f;
            if (dat < 0.0f) dat = 0.0f;
            *data_deal = (uint16_t)(dat * 1000); // ×1000（0.000格式）
            *unit = 0;       // 无单位
            *f = 0;          // 无负号
            break;
        }

        default:
            break;
    }
}

// ====================== 格式化显示函数：data_disp ======================
// 参数说明：
// dat   ：放大后的数据（Q/P×100、U×100、I×10、COS×1000）
// unit  ：单位（仅Q/P有效：0=K 1=M）
// f     ：符号位（仅Q/P有效：0=无 1=有）
// type  ：1=Q 2=P 3=U 4=I 5=COS
// buf   ：输出缓冲区（至少16字节）
void data_disp(uint16_t dat, uint8_t unit, uint8_t f, uint8_t type, char* buf) {
    int idx = 0;
    int i = 0;

	for ( i = 0; i < 16; i++) buf[i] = '\0';

    switch(type) {
        // ========== 1=无功(Q)：Q=±00.00 KVar/MVar ==========
        case 1: {
            buf[idx++] = 'Q';
            buf[idx++] = '=';
            // 符号位（- / 空格）
            buf[idx++] = f ? '-' : ' ';
            // 00.00格式（补零）
            buf[idx++] = (dat / 10000) % 10 + '0'; // 十位（如9999→9）
            buf[idx++] = (dat / 1000) % 10 + '0'; // 个位（如9999→9）
            buf[idx++] = '.';
            buf[idx++] = (dat / 10) % 10 + '0';   // 十分位（如9999→9）
            buf[idx++] = dat % 10 + '0';          // 百分位（如9999→9）
            // 单位
            buf[idx++] = ' ';
            buf[idx++] = unit ? 'M' : 'K';
            buf[idx++] = 'V';
            buf[idx++] = 'a';
            buf[idx++] = 'r';
            break;
        }

        // ========== 2=有功(P)：P=±00.00 KW/MW ==========
        case 2: {
            buf[idx++] = 'P';
            buf[idx++] = '=';
            // 符号位
            buf[idx++] = f ? '-' : ' ';
            // 00.00格式（补零）
            buf[idx++] = (dat / 10000) % 10 + '0';
            buf[idx++] = (dat / 1000) % 10 + '0';
            buf[idx++] = '.';
            buf[idx++] = (dat / 10) % 10 + '0';
            buf[idx++] = dat % 10 + '0';
            // 单位
            buf[idx++] = ' ';
            buf[idx++] = unit ? 'M' : 'K';
            buf[idx++] = 'W';
            break;
        }

        // ========== 3=电压(U)：U=00.00 KV ==========
        case 3: {
            buf[idx++] = 'U';
            buf[idx++] = '=';
            buf[idx++] = ' '; // 无符号位
            // 00.00格式（补零）
            buf[idx++] = (dat / 10000) % 10 + '0';
            buf[idx++] = (dat / 1000) % 10 + '0';
            buf[idx++] = '.';
            buf[idx++] = (dat / 10) % 10 + '0';
            buf[idx++] = dat % 10 + '0';
            // 单位
            buf[idx++] = ' ';
            buf[idx++] = 'K';
            buf[idx++] = 'V';
            break;
        }

        // ========== 4=电流(I)：I=000.0 A ==========
        case 4: {
            buf[idx++] = 'I';
            buf[idx++] = '=';
            buf[idx++] = ' '; // 无符号位
            // 000.0格式（补零）
            buf[idx++] = (dat / 1000) % 10 + '0'; // 百位（如123→0）
            buf[idx++] = (dat / 100) % 10 + '0'; // 十位（如123→1）
            buf[idx++] = (dat / 10) % 10 + '0';  // 个位（如123→2）
            buf[idx++] = '.';
            buf[idx++] = dat % 10 + '0';         // 十分位（如123→3）
            // 单位
            buf[idx++] = ' ';
            buf[idx++] = 'A';
            break;
        }

        // ========== 5=功率因数(COS)：COS=0.000 ==========
        case 5: {
            buf[idx++] = 'C';
            buf[idx++] = 'O';
            buf[idx++] = 'S';
            buf[idx++] = '#';					
            buf[idx++] = '=';
            // 0.000格式（补零）
            buf[idx++] = (dat / 1000) % 10 + '0'; // 整数位（0/1）
            buf[idx++] = '.';
            buf[idx++] = (dat / 100) % 10 + '0';  // 百分位
            buf[idx++] = (dat / 10) % 10 + '0';   // 千分位
            buf[idx++] = dat % 10 + '0';          // 万分位
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
        // 电压：传入值 = 实际值 × 10 → 实际值 = 传入值 / 10
        temp = val; // 传入值本身就是实际值×10
        if (temp < 1000) {   // temp<1000等价于实际值<100
            integer_part = (uint8_t)(temp / 10);  // 实际值的整数部分
            decimal_part = (uint8_t)(temp % 10);  // 实际值的小数部分
            int_val = 0; // 标记走00.0格式
        } else {
            int_val = (uint16_t)(val / 10);       // 还原为实际整数值
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
void  Data_Get(void)
{

  if(g_meter_chip[0].data_ready)
	{
	
   g_cap_data[0].ia=g_meter_chip[0].i_a*1000>>13*(uint32_t)g_cap_ratio[0].ct_ratio*10/g_cap_ratio[0].ct_ratio1;
   g_cap_data[0].ic=g_meter_chip[0].i_b*1000>>13*(uint32_t)g_cap_ratio[0].ct_ratio*10/g_cap_ratio[0].ct_ratio1;
   g_cap_data[0].uo=g_meter_chip[0].u_a*10>>13;		
   g_cap_data[1].ia=g_meter_chip[0].u_b*1000>>13*(uint32_t)g_cap_ratio[1].ct_ratio*10/g_cap_ratio[1].ct_ratio1;
   g_cap_data[1].ic=g_meter_chip[0].i_c*1000>>13*(uint32_t)g_cap_ratio[1].ct_ratio*10/g_cap_ratio[1].ct_ratio1;		
   g_cap_data[1].uo=g_meter_chip[0].u_c*10>>13;			
		
	  g_meter_chip[0].data_ready=0;
	
	
	}
  if(g_meter_chip[1].data_ready)
	{
	 g_cap_data[2].ia=g_meter_chip[1].i_a*1000>>13*(uint32_t)g_cap_ratio[0].ct_ratio*10/g_cap_ratio[0].ct_ratio1;
   g_cap_data[2].ic=g_meter_chip[1].i_b*1000>>13*(uint32_t)g_cap_ratio[0].ct_ratio*10/g_cap_ratio[0].ct_ratio1;
   g_cap_data[2].uo=g_meter_chip[1].u_a>>13;		
   g_cap_data[3].ia=g_meter_chip[1].u_b*1000>>13*(uint32_t)g_cap_ratio[1].ct_ratio*10/g_cap_ratio[1].ct_ratio1;
   g_cap_data[3].ic=g_meter_chip[1].i_c*1000>>13*(uint32_t)g_cap_ratio[1].ct_ratio*10/g_cap_ratio[1].ct_ratio1;		
   g_cap_data[3].uo=g_meter_chip[1].u_c*10>>13;		
	  g_meter_chip[1].data_ready=0;
	}
  if(g_meter_chip[2].data_ready)
	{
	
	  g_meter_chip[2].data_ready=0;	
	}
}




