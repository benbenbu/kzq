#ifndef _TIMER_H
#define _TIMER_H

#include "config.h"

// 定时器状态枚举
typedef enum {
    TIMER_DISABLED = 0,
    TIMER_ENABLED
} timer_state_t;

// 定时器配置结构体
typedef struct {
    timer_state_t state;      // 定时器状态
    uint16_t period_ms;       // 定时器周期（毫秒）
    uint16_t counter;         // 当前计数值
    uint16_t target_count;    // 目标计数值
    uint8_t callback_enabled; // 回调函数使能标志
    void (*callback)(void);   // 回调函数指针
} timer_config_t;

// 函数声明

/**
 * @brief 定时器初始化函数
 * @param 无
 * @return 无
 */
void Timer_Init(void);

/**
 * @brief 定时器0配置函数
 * @param period_ms 定时器周期（毫秒）
 * @param callback 回调函数指针，可为NULL
 * @return 无
 */
void Timer0_Config(uint16_t period_ms, void (*callback)(void));

/**
 * @brief 定时器1配置函数
 * @param period_ms 定时器周期（毫秒）
 * @param callback 回调函数指针，可为NULL
 * @return 无
 */
void Timer1_Config(uint16_t period_ms, void (*callback)(void));

/**
 * @brief 定时器2配置函数
 * @param period_ms 定时器周期（毫秒）
 * @param callback 回调函数指针，可为NULL
 * @return 无
 */
void Timer2_Config(uint16_t period_ms, void (*callback)(void));

/**
 * @brief 定时器3配置函数
 * @param period_ms 定时器周期（毫秒）
 * @param callback 回调函数指针，可为NULL
 * @return 无
 */
void Timer3_Config(uint16_t period_ms, void (*callback)(void));

/**
 * @brief 定时器4配置函数
 * @param period_ms 定时器周期（毫秒）
 * @param callback 回调函数指针，可为NULL
 * @return 无
 */
void Timer4_Config(uint16_t period_ms, void (*callback)(void));

/**
 * @brief 停止指定定时器
 * @param timer_num 定时器编号 (0-4)
 * @return 无
 */
void Timer_Stop(uint8_t timer_num);

/**
 * @brief 启动指定定时器
 * @param timer_num 定时器编号 (0-4)
 * @return 无
 */
void Timer_Start(uint8_t timer_num);

/**
 * @brief 获取定时器状态
 * @param timer_num 定时器编号 (0-4)
 * @return 定时器状态
 */
timer_state_t Timer_GetState(uint8_t timer_num);



#endif // _C8051F120_TIMER_H