/*
*********************************************************************************************************
*
*   模块名称 : 按键驱动模块 (ADC版)
*   文件名称 : key.h
*   版    本 : V1.1 (ADC适配版)
*   说    明 : 支持通过单ADC通道读取多个按键状态，使用分压电阻网络
*
*********************************************************************************************************
*/

#ifndef ADC_KEY_H
#define ADC_KEY_H

#include "config.h"

// 按键阈值设定（根据实际校准调整）
#define THRESHOLD_KEY0    3754    /* ~2.20V */
#define THRESHOLD_KEY1    2816    /* ~1.65V */
#define THRESHOLD_KEY2    2167    /* ~1.27V */
#define THRESHOLD_KEY3    1758    /* ~1.03V */
#define THRESHOLD_KEY4    1143    /* ~0.67V */
#define THRESHOLD_KEY5     512    /* ~0.30V */

// 容差窗口（± LSB）
// 检查最小间距：
// K0-K1: 682, K1-K2: 468, K2-K3: 300, K3-K4: 445, K4-K5: 462 → 最小为 300
// 所以 WINDOW 最大可设为 ±120~140

#define THRESHOLD_WINDOW   80    /* 推荐值 */

#define ADC_SAMPLE_BUF_SIZE  8       // 8个采样点（8*10ms=80ms滤波窗口）
#define ADC_STABLE_COUNT     3       // 3次稳定（3*10ms=30ms防抖）
#define NO_KEY_THRESHOLD     100    // 无键值（10位ADC最大值，适配你的硬件）
#define KEY_RELEASE_CONFIRM  2       // 松开二次确认（2*10ms=20ms）


#define KEY_COUNT    6                     /* 按键个数，现为6个ADC按键 */

/* 按键ID */
typedef enum {
    KID_K0 = 0,
    KID_K1,
    KID_K2,
    KID_K3,
    KID_K4,
    KID_K5,
} KEY_ID_E;

/*
    滤波时间：2个单位（每个单位10ms），即20ms去抖
    长按时间：50单位 = 500ms 触发长按
*/
#define KEY_FILTER_TIME   3
#define KEY_LONG_TIME     50      /* 单位10ms，持续500ms认为长按 */

/*
    每个按键的状态结构体
*/
typedef struct {
    uint8_t (*IsKeyDownFunc)(void);   /* 检测该键是否正在被按下 */
    uint8_t  Count;         /* 滤波计数器 */
    uint16_t LongCount;     /* 长按计数器 */
    uint16_t LongTime;      /* 长按判定阈值（单位：10ms） */
    uint8_t  State;         /* 当前状态：1=按下，0=释放 */
    uint8_t  RepeatSpeed;   /* 连发速度（单位：10ms） */
    uint8_t  RepeatCount;   /* 连发计数 */
} KEY_T;

/*
    键值枚举（必须连续）
*/
typedef enum {
    KEY_NONE = 0,

    KEY_0_DOWN, KEY_0_UP, KEY_0_LONG,
    KEY_1_DOWN, KEY_1_UP, KEY_1_LONG,
    KEY_2_DOWN, KEY_2_UP, KEY_2_LONG,
    KEY_3_DOWN, KEY_3_UP, KEY_3_LONG,
    KEY_4_DOWN, KEY_4_UP, KEY_4_LONG,
    KEY_5_DOWN, KEY_5_UP, KEY_5_LONG,

    // 可扩展组合键...
} KEY_ENUM;

/* 按键FIFO */
#define KEY_FIFO_SIZE   10
typedef struct {
    uint8_t Buf[KEY_FIFO_SIZE];
    uint8_t Read;
    uint8_t Write;
} KEY_FIFO_T;

/* 外部接口函数声明 */
void Bsp_InitKey(void);
void bsp_KeyScan(void);
void bsp_PutKey(uint8_t _KeyCode);
uint8_t bsp_GetKey(void);
uint8_t bsp_GetKeyState(KEY_ID_E _ucKeyID);
void bsp_SetKeyParam(uint8_t _ucKeyID, uint16_t _LongTime, uint8_t _RepeatSpeed);
void bsp_ClearKey(void);

#endif