/*
*********************************************************************************************************
*
*   模块名称 : 按键驱动模块 (ADC版)
*   文件名称 : key.c
*   版    本 : V1.1
*   说    明 : 扫描ADC输入上的多个按键，具有软件滤波机制和FIFO。
*             支持：短按、弹起、长按、连发
*
*********************************************************************************************************
*/

#include "adc_key.h"
#include "math.h"

// 外部获取ADC值（需由ADC中断更新）




//static KEY_T s_tBtn[KEY_COUNT];
//static KEY_FIFO_T s_tKey;

 KEY_T s_tBtn[KEY_COUNT];
 KEY_FIFO_T s_tKey;

static void bsp_InitKeyVar(void);
static void bsp_DetectKey(uint8_t i);
int GetAdcDetectedKey(void);  // 内部函数：根据ADC值返回当前按键索引

extern unsigned int s_LastAdcValue ;  // 新增：记录上一次有效ADC值

/*
*********************************************************************************************************
*   函 数 名: GetAdcDetectedKey
*   功能说明: 根据当前ADC值判断哪一个按键被按下
*   形    参: 无
*   返 回 值: 0~5 表示对应按键被按下；-1 表示无键
*********************************************************************************************************
*/
int GetAdcDetectedKey(void) {
    unsigned int val = g_adc_value;

    if ((val >= THRESHOLD_KEY0 - THRESHOLD_WINDOW) && (val <= THRESHOLD_KEY0 + THRESHOLD_WINDOW)) 
		{
			return 0;
		}
    else if ((val >= THRESHOLD_KEY1 - THRESHOLD_WINDOW) && (val <= THRESHOLD_KEY1 + THRESHOLD_WINDOW)) 
		{

			return 1;
		}
    else if ((val >= THRESHOLD_KEY2 - THRESHOLD_WINDOW) && (val <= THRESHOLD_KEY2 + THRESHOLD_WINDOW)) 
		{

			return 2;
		}
    else if ((val >= THRESHOLD_KEY3 - THRESHOLD_WINDOW) && (val <= THRESHOLD_KEY3 + THRESHOLD_WINDOW)) 
		{

			return 3;
		}
    else if ((val >= THRESHOLD_KEY4 - THRESHOLD_WINDOW) && (val <= THRESHOLD_KEY4 + THRESHOLD_WINDOW)) 
		{

			return 4;
		}
    else if ((val >= THRESHOLD_KEY5 - THRESHOLD_WINDOW) && (val <= THRESHOLD_KEY5 + THRESHOLD_WINDOW)) 
		{
			return 5;
		}

    return -1;  // 无有效按键
}
static uint8_t s_StableKeyDetectCount = 0;
static int s_LastDetectedKey = -1;
#define ADC_STABLE_COUNT    3     // 稳定按键计数
int GetStableAdcDetectedKey(void) {
	    int current_key = GetAdcDetectedKey();
    if (!g_adc_data_valid) {
        s_StableKeyDetectCount = 0;
        s_LastDetectedKey = -1;
        return -1;
    }
    if (current_key == s_LastDetectedKey && current_key != -1) {
        s_StableKeyDetectCount++;
        if (s_StableKeyDetectCount >= ADC_STABLE_COUNT) return current_key;
    } else {
        s_StableKeyDetectCount = 0;
        s_LastDetectedKey = current_key;
    }
    return -1;
}

//int GetAdcDetectedKey(void) {
//    unsigned int val = g_adc_value;

//    // 优化1：过滤ADC值突变（噪声导致的误判，阈值差>80+20=100则判定为噪声）
//    if (abs((int)val - (int)s_LastAdcValue) > (THRESHOLD_WINDOW + 20)) {
//        return -1;
//    }

//    // 保留你的原始判断逻辑，适配80窗口（无需缩小窗口，因你的阈值间距足够大）
//    if ((val >= THRESHOLD_KEY0 - THRESHOLD_WINDOW) && (val <= THRESHOLD_KEY0 + THRESHOLD_WINDOW)) {
//        return 0;
//    } else if ((val >= THRESHOLD_KEY1 - THRESHOLD_WINDOW) && (val <= THRESHOLD_KEY1 + THRESHOLD_WINDOW)) {
//        return 1;
//    } else if ((val >= THRESHOLD_KEY2 - THRESHOLD_WINDOW) && (val <= THRESHOLD_KEY2 + THRESHOLD_WINDOW)) {
//        return 2;
//    } else if ((val >= THRESHOLD_KEY3 - THRESHOLD_WINDOW) && (val <= THRESHOLD_KEY3 + THRESHOLD_WINDOW)) {
//        return 3;
//    } else if ((val >= THRESHOLD_KEY4 - THRESHOLD_WINDOW) && (val <= THRESHOLD_KEY4 + THRESHOLD_WINDOW)) {
//        return 4;
//    } else if ((val >= THRESHOLD_KEY5 - THRESHOLD_WINDOW) && (val <= THRESHOLD_KEY5 + THRESHOLD_WINDOW)) {
//        return 5;
//    }

//    return -1;  // 无有效按键
//}
//static uint8_t s_StableKeyDetectCount = 0;
//static int s_LastDetectedKey = -1;
//#define ADC_STABLE_COUNT    3     // 3次稳定（3*10ms=30ms防抖）

//int GetStableAdcDetectedKey(void) {
//    int current_key = GetAdcDetectedKey();

//    // 优化1：无有效ADC数据 或 当前无键 → 重置所有计数
//    if (!g_adc_data_valid || current_key == -1) {
//        s_StableKeyDetectCount = 0;
//        s_LastDetectedKey = -1;
//        return -1;
//    }

//    // 优化2：稳定计数逻辑（仅连续3次同一按键才返回）
//    if (current_key == s_LastDetectedKey) {
//        s_StableKeyDetectCount++;
//        if (s_StableKeyDetectCount >= ADC_STABLE_COUNT) {
//            return current_key; // 稳定3次，返回有效按键
//        }
//    } else {
//        // 按键变化，重置计数
//        s_StableKeyDetectCount = 1;  // 优化：从1开始，而非0
//        s_LastDetectedKey = current_key;
//    }

//    return -1;  // 未达到稳定条件
//}
/*
*********************************************************************************************************
*   函 数 名: IsKeyDownX
*   功能说明: 判断第i个键是否当前被按下
*   形    参: 无
*   返 回 值: 1=按下，0=未按下
*********************************************************************************************************
*/
static uint8_t IsKeyDown0(void) {return  (GetStableAdcDetectedKey() == 0) ? 1 : 0; }
static uint8_t IsKeyDown1(void) { return (GetStableAdcDetectedKey() == 1) ? 1 : 0; }
static uint8_t IsKeyDown2(void) { return (GetStableAdcDetectedKey() == 2) ? 1 : 0; }
static uint8_t IsKeyDown3(void) { return (GetStableAdcDetectedKey() == 3) ? 1 : 0; }
static uint8_t IsKeyDown4(void) { return (GetStableAdcDetectedKey() == 4) ? 1 : 0; }
static uint8_t IsKeyDown5(void) { return (GetStableAdcDetectedKey() == 5) ? 1 : 0; }

/*
*********************************************************************************************************
*   函 数 名: Bsp_InitKey
*   功能说明: 初始化按键模块
*********************************************************************************************************
*/
void Bsp_InitKey(void) {
    bsp_InitKeyVar();
    // 不需要初始化GPIO，ADC由其他模块管理
}

/*
*********************************************************************************************************
*   函 数 名: bsp_PutKey
*   功能说明: 将键值压入FIFO
*/
void bsp_PutKey(uint8_t _KeyCode) {
    s_tKey.Buf[s_tKey.Write] = _KeyCode;
    if (++s_tKey.Write >= KEY_FIFO_SIZE) {
        s_tKey.Write = 0;
    }
}

/*
*********************************************************************************************************
*   函 数 名: bsp_GetKey
*   功能说明: 从FIFO读取键值
*/
uint8_t bsp_GetKey(void) {
    uint8_t ret;
    if (s_tKey.Read == s_tKey.Write) return KEY_NONE;
    ret = s_tKey.Buf[s_tKey.Read];
    if (++s_tKey.Read >= KEY_FIFO_SIZE) s_tKey.Read = 0;
    return ret;
}

/*
*********************************************************************************************************
*   函 数 名: bsp_GetKeyState
*   功能说明: 获取某键当前状态
*/
uint8_t bsp_GetKeyState(KEY_ID_E _ucKeyID) {
    if (_ucKeyID < KEY_COUNT) {
        return s_tBtn[_ucKeyID].State;
    }
    return 0;
}

/*
*********************************************************************************************************
*   函 数 名: bsp_SetKeyParam
*   功能说明: 设置长按时间和连发速度
*/
void bsp_SetKeyParam(uint8_t _ucKeyID, uint16_t _LongTime, uint8_t _RepeatSpeed) {
    if (_ucKeyID < KEY_COUNT) {
        s_tBtn[_ucKeyID].LongTime = _LongTime;
        s_tBtn[_ucKeyID].RepeatSpeed = _RepeatSpeed;
        s_tBtn[_ucKeyID].RepeatCount = 0;
    }
}

/*
*********************************************************************************************************
*   函 数 名: bsp_ClearKey
*   功能说明: 清空FIFO
*/
void bsp_ClearKey(void) {
    s_tKey.Read = s_tKey.Write;
}

/*
*********************************************************************************************************
*   函 数 名: bsp_InitKeyVar
*   功能说明: 初始化所有按键变量
*/
static void bsp_InitKeyVar(void) {
    uint8_t i;

    s_tKey.Read = 0;
    s_tKey.Write = 0;

    for (i = 0; i < KEY_COUNT; i++) {
        s_tBtn[i].LongTime = KEY_LONG_TIME;
        s_tBtn[i].Count = KEY_FILTER_TIME / 2;
        s_tBtn[i].State = 0;
        s_tBtn[i].RepeatSpeed = 0;
        s_tBtn[i].RepeatCount = 0;
    }

    /* 自定义参数示例 */
    s_tBtn[0].RepeatSpeed = 50;  // 每100ms连发
    s_tBtn[1].RepeatSpeed = 50;

    /* 绑定检测函数 */
    s_tBtn[0].IsKeyDownFunc = IsKeyDown0;
    s_tBtn[1].IsKeyDownFunc = IsKeyDown1;
    s_tBtn[2].IsKeyDownFunc = IsKeyDown2;
    s_tBtn[3].IsKeyDownFunc = IsKeyDown3;
    s_tBtn[4].IsKeyDownFunc = IsKeyDown4;
    s_tBtn[5].IsKeyDownFunc = IsKeyDown5;
}

/*
*********************************************************************************************************
*   函 数 名: bsp_DetectKey
*   功能说明: 检测单个按键状态（带去抖、长按、连发）
*   修改说明: 使用标准递增/递减去抖，避免突变
*/
static void bsp_DetectKey(uint8_t i) {
    KEY_T *pBtn = &s_tBtn[i];
    uint8_t press = pBtn->IsKeyDownFunc();

    if (press) {
        if (pBtn->Count < KEY_FILTER_TIME) {
            pBtn->Count++;  // 逐步增加，直到达到阈值
        }

        if (pBtn->Count == KEY_FILTER_TIME) {
            // 首次确认按下
            if (pBtn->State == 0) {
                pBtn->State = 1;
                pBtn->LongCount = 0;
                pBtn->RepeatCount = 0;
                bsp_PutKey((uint8_t)(3 * i + 1));  // KEY_X_DOWN
            }

            // 长按处理
            if (pBtn->LongTime > 0) {
                if (++pBtn->LongCount >= pBtn->LongTime) {
                    pBtn->LongCount = pBtn->LongTime;  // 防溢出
                    // 触发长按事件（只发一次）
                    if ((pBtn->RepeatSpeed == 0) || 
                        (pBtn->RepeatCount == 0)) {  // 避免重复发送 LONG
                        bsp_PutKey((uint8_t)(3 * i + 3));  // KEY_X_LONG
                    }
                }

                // 连发处理
                if (pBtn->RepeatSpeed > 0) {
                    if (++pBtn->RepeatCount >= pBtn->RepeatSpeed) {
                        pBtn->RepeatCount = 0;
                        bsp_PutKey((uint8_t)(3 * i + 2));  // 连发 DOWN
                    }
                }
            }
        }
    } else {
        if (pBtn->Count > 0) {
            pBtn->Count--;  // 逐步减少
        }

        if (pBtn->Count == 0) {
            if (pBtn->State == 1) {
                pBtn->State = 0;
                bsp_PutKey((uint8_t)(3 * i + 2));  // KEY_X_UP
            }
            pBtn->LongCount = 0;
            pBtn->RepeatCount = 0;
        }
    }
}

/*
*********************************************************************************************************
*   函 数 名: bsp_KeyScan
*   功能说明: 扫描所有按键（主循环每10ms调用一次）
*/
void bsp_KeyScan(void) {
	  uint8_t i;
    for ( i = 0; i < KEY_COUNT; i++) {
        bsp_DetectKey(i);
    }
}