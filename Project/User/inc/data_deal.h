#ifndef _DATA_DEAL_H_
#define _DATA_DEAL_H_

#include "Type_def.h"

/* 新增：保护计时结构体（记录各保护当前计时值） */
typedef struct {
    uint16_t volt_h_time;          /* 过压保护当前计时（ms） */
    uint16_t volt_l_time;          /* 欠压保护当前计时（ms） */
    uint16_t cap_over_time[4][2];  /* 电容组过流当前计时（ms）：[组号][0=IA,1=IC] */
    uint16_t cap_quick_time[4][2]; /* 电容组速断当前计时（ms）：[组号][0=IA,1=IC] */
    uint16_t cap_zero_time[4];     /* 电容组零序当前计时（ms）（仅UO，不分通道） */
} Protect_Time_Struct;

typedef enum {
    CAP_STRATEGY_DIFF = 1,    // 差容（最优匹配）
    CAP_STRATEGY_COMBINE = 2, // 组合（选两个）
    CAP_STRATEGY_FILTER = 3,  // 滤波（顺序投切）
    CAP_STRATEGY_EQUAL = 4    // 等容（均衡使用）
} Cap_Strategy_Type;

// 投切指令状态（保留核心状态，无重试）
typedef enum Cap_Cmd_Type {
    CMD_NONE = 0,          // 无指令
    CMD_CHARGE = 1,        // 投电容指令（延时中）
    CMD_DISCHARGE = 2,     // 切电容指令（延时中）
    CMD_WAIT_FEEDBACK = 3  // 等待反馈状态（单次检测）
} Cap_Cmd_Type;

// 修改后的电容投切状态结构体
typedef struct Cap_Switch_State {
    Cap_Cmd_Type pending_cmd;       // 待执行/当前状态指令（CMD_NONE/CHARGE/DISCHARGE/WAIT_FEEDBACK）
    uint8_t pending_cap_idx[2];     // 待投/切的电容索引（兼容组合/非组合，保留原有长度）
    uint8_t pending_cap_count;      // 待操作的电容数量（1=非组合，2=组合）
    uint16_t delay_remaining;       // 投切延时剩余时间（s）
    uint32_t last_switch_time;      // 上一次投切完成时间（s）
    uint16_t daily_switch_count;    // 今日投切累计次数
    uint8_t switch_result;          // 投切反馈结果（0=未检测，1=成功，2=失败）
    
    // 反馈检测核心变量（无重试）
    uint8_t feedback_timer;         // 反馈检测计时（s，累计到3s后触发检测）
    uint8_t cmd_sent_flag;          // 命令发送完成标志（0=未发完，1=已发完）
    // ========== 新增：记录最后下发的投/切命令 ==========
    Cap_Cmd_Type last_cmd;          // 记录触发反馈前的原始命令（CMD_CHARGE/CMD_DISCHARGE）
} Cap_Switch_State;

typedef struct {
    uint8_t cmd_state;  // 命令状态：0=无命令，1=已下发投/切命令，2=2秒到已关断
    uint8_t timer;      // 计时（1秒递增，2=2秒）
    uint8_t cap_ch;     // 目标电容通道：1=CAP1，2=CAP2，3=CAP3，4=CAP4
    uint8_t cmd_type;   // 命令类型：1=投（ON），2=切（OFF）
} Cap_Latch_Struct;
extern volatile Protect_Time_Struct g_pro_time;        // 保护计时变量
extern Cap_Switch_State g_cap_switch_state;

extern  Cap_Latch_Struct g_cap_latch[4] ;
void Beep_Control(uint8_t enable);


uint8_t CRC8_Calc(const uint8_t *sdata, uint16_t len);
uint16_t Modbus_CRC16(u8 *pucBuff, u8 unNum);

void data_disp(long dat, uint8_t type, char* buf);
void meter_data(uint32_t meter_data, uint8_t type, int32_t *actual_value);
void cap_data_disp(char num,char type,uint32_t calc_val, char unit, char *out_str);
uint8_t cap_switch_delay_handler(void);
void  Data_Get(void);
#endif