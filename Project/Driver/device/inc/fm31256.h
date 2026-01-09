#ifndef __FM31256_H__
#define __FM31256_H__

#include "Type_def.h" 

// --------------- 用户已有时间结构体（保留原有定义，无需修改）---------------
// 若已在其他文件定义，可注释此行；否则保留
//typedef struct {
//    uint8_t  year;    // 年（00~99，对应2000~2099）
//    uint8_t  month;   // 月（1~12）
//    uint8_t  day;     // 日（1~31）
//    uint8_t  hour;    // 时（0~23）
//    uint8_t  minute;  // 分（0~59）
//    uint8_t  second;  // 秒（0~59）
//} Time_Struct;

// 3. 版本号读取结构体（方便业务逻辑使用）
typedef struct {
    uint8_t main_ver;    // 主版本
    uint8_t sub_ver;     // 次版本
    uint8_t hw_ver;      // 硬件版本
} Version_Struct;


// --------------- FM31256 核心定义（保留原有地址，修正关键寄存器）---------------

//首次上电标志位：选12h寄存器（序列号Byte1）做标志位
#define SN_FLAG_ADDR           0x12    // 标志位地址（12h=Serial Number Byte1）
#define INIT_FLAG_VAL          0xAA    // 初始化完成标志值（非FFh即可）


//  版本号存储地址（13h=主版本，14h=次版本，15h=硬件版本）
#define SN_VER_MAIN_ADDR  0x13
#define SN_VER_SUB_ADDR   0x14
#define SN_HW_VER_ADDR    0x15

#define FRAM_TEST_ADDR    0x5000UL

// I2C从地址（保留你原有定义：A0/A1接地，Memory模式=0xA0/A1，Companion模式=0xD0/D1）
#define FM31256_SLAVE_W    0xA0  // Memory模式-写地址（FRAM存储用）
#define FM31256_SLAVE_R    0xA1  // Memory模式-读地址（FRAM存储用）
#define FM31256_COM_SLAVE_W  0xD0  // Companion模式-写地址（RTC/看门狗用）
#define FM31256_COM_SLAVE_R  0xD1  // Companion模式-读地址（RTC/看门狗用）

// 寄存器地址定义（保留原有地址，补充RTC控制/看门狗关键寄存器）
#define FM31256_RTC_CTRL   0x00  // RTC控制寄存器（Flags/Control：W/R位控制读写）
#define REG_RTC_CAL_CTRL   0x01  // 晶振
#define FM31256_RTC_SEC    0x02  // 秒寄存器（BCD码）
#define FM31256_RTC_MIN    0x03  // 分寄存器（BCD码）
#define FM31256_RTC_HOUR   0x04  // 时寄存器（BCD码，24小时制）
#define FM31256_RTC_WEEK   0x05  // 星期寄存器（BCD码，1~7，写默认值）
#define FM31256_RTC_DAY    0x06  // 日寄存器（BCD码）
#define FM31256_RTC_MONTH  0x07  // 月寄存器（BCD码）
#define FM31256_RTC_YEAR   0x08  // 年寄存器（BCD码）






// 看门狗相关寄存器
#define FM31256_REG_WDT_RESTART  0x09  // 0x09：WDT重启&标志
#define FM31256_REG_WDT_CTRL     0x0A  // 0x0A：WDT控制（使能+超时周期）
#define FM31256_REG_COMPANION_CTL 0x0B // 0x0B：Companion控制（写保护、充电等）

// 看门狗超时时间配置（修正为手册100ms分辨率，映射你原有命名）
#define WDG_1S             0x0A  // 1000ms（10*100ms，WDT4~WDT0=01010）
#define WDG_2S             0x14  // 2000ms（20*100ms，WDT4~WDT0=10100）
#define WDG_4S             0x28  // 4000ms（40*100ms，WDT4~WDT0=101000→修正为手册最大3000ms）
#define WDG_8S             0x1E  // 3000ms（手册最大，WDT4~WDT0=11110）
#define WDG_16S            0x1E  // 超出手册范围，映射到最大3000ms
#define WDG_32S            0x1E  // 超出手册范围，映射到最大3000ms
#define WDG_DISABLE        0x1F  // 禁用看门狗计数器（WDT4~WDT0=11111）

// 控制位掩码（内部使用，用户无需关注）
#define RTC_CTRL_W_MASK    0x02  // W位（D1：1=冻结RTC，0=解锁）
#define RTC_CTRL_R_MASK    0x01  // R位（D0：1=捕获读值，0=正常）
#define WDT_CTRL_WDE_MASK  0x80  // WDE位（D7：1=触发复位，0=仅置位标志）
#define WDT_RESTART_PATTERN 0x0A // 喂狗图案（WR3~WR0=1010b）

// 错误码（保留原有定义）
#define FM31256_OK         0x00  // 操作成功
#define FM31256_ERR_I2C    0x01  // I2C通信错误
#define FM31256_ERR_PARAM  0x02  // 参数错误（地址越界/时间无效）

// BCD码转换宏（保留原有定义）
#define DEC_TO_BCD(x)      (((x)/10)<<4) | ((x)%10)  // 十进制转BCD
#define BCD_TO_DEC(x)      (((x)>>4)*10) + ((x)&0x0F) // BCD转十进制

// --------------- 函数声明---------------
// 看门狗相关
uint8_t FM31256_WDG_Enable(uint8_t timeout);  // 使能看门狗（指定超时时间）
uint8_t FM31256_WDG_Disable(void);            // 禁用看门狗（WDE=0）
uint8_t FM31256_WDG_Feed(void);               // 看门狗喂狗（写1010b重启）

// FRAM存储相关
uint8_t FM31256_FRAM_Read(uint16_t addr, uint8_t *buf, uint16_t len);  // 连续读FRAM
uint8_t FM31256_FRAM_Write(uint16_t addr, uint8_t *buf, uint16_t len); // 连续写FRAM
uint8_t FM31256_Reg_Read(uint8_t addr, uint8_t *byte) ;
uint8_t FM31256_Reg_Write(uint8_t addr,  uint8_t byte);
// RTC时间相关
uint8_t FM31256_RTC_Read_Date(Date_Struct *date) ;
uint8_t FM31256_RTC_Read_Time(Time_Struct *time) ;
uint8_t FM31256_RTC_Write_Time( Time_Struct *time);
uint8_t FM31256_RTC_Write_Date( Date_Struct *date);
//写保护
uint8_t FM31256_Write_Protect(uint8_t onf);//0 关闭 1打开
uint8_t FM31256_System_Init(void);

uint8_t FM31256_Read_Version(Version_Struct *ver);
uint8_t FM31256_Write_Version(Version_Struct *ver);
#endif  // __FM31256_H__