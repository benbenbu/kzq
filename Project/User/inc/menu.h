#ifndef __MENU_H__
#define __MENU_H__

#include "config.h"


#define LCD_MID  81  //标题行坐标
#define LCD_LEFT 0  //左半边起始
#define LCD_RIGHT 114//右半边起始
#define LCD_SIZE  9 //单个字符宽度，汉字是2倍的宽度 ：18


// 语言类型定义
typedef enum {
    LANGUAGE_CHINESE,
    LANGUAGE_ENGLISH
} MenuLanguage;

// 菜单定义
typedef enum {
    METER_DISP=0, //计量界面
    MAIN_MENU, //主菜单
    SYSTEM_SETTING, //系统设置
    PARAM_SETTING, //参数设置
    ADJUST_CAP, //调试投切
	  SAMPLING_CALIBRATION, //采样校准显示
    EVENT_LOG, //事件记录
    DATA_STATISTICS, //数据统计
    SYSTEM_PARAM, //系统参数
    CAP_PARAM, //电容参数
    CTRL_PARAM, //控制参数
    PROT_PARAM, //保护参数
    COMM_PARAM, //通讯参数
    STATISTIC_PARAM, //统计参数
	  PROT_PARAM_VOL_H,//过压
	  PROT_PARAM_VOL_L,//欠压
	  PROT_CAP1,//一路	
	  PROT_CAP2,//二路	
	  PROT_CAP3,//三路	
	  PROT_CAP4,//四路		
	
	  PASSWORD,//保护口令
    SAMPLING, //采样校准		
		ERR,//报警
		EVENT,//事件详情		
		

} MENU_ID;

#define MODE_BROWSE     0
#define MODE_EDIT       1



// 菜单状态结构体
typedef struct {
    uint8_t id;             // 当前菜单ID
	uint8_t last_id;          //上一个id
	uint8_t refresh_flag;      // 刷新标记：0=不刷新，1=需要刷新（按键/切换触发）
	uint8_t switch_temp_flag;  // 新增：界面切换临时标记（0=无切换，1=待切换）
	uint8_t timer_1s_flag;//1s时间到
	uint8_t meter_data_flag;//数据更新
	uint8_t flash;//闪烁计时  1s
} MenuState;

// 函数声明

typedef enum {
    VOL_PROT_H = 0,
    VOL_PROT_L = 1
} VolProtType;

void  menu_disp();
void menu_init();

/**********函数定义**********/

void disp_two_digit(uint8_t num, uint16_t x, uint8_t y, uint8_t size, uint8_t inverse);
void Main_Menu(uint8_t key_val);
void Para_Menu(uint8_t key_val);
void Protect_Menu(uint8_t key_val);
void System_Setting(u8 key_val);

void Sys_Para_Set(u8 key_val);
void Sys_Vol_Prot_Para_Set(u8 key_val, VolProtType prot_type);
void Sys_Cap_Prot_Para_Set(u8 key_val, char num);
void Sys_Com_Para_Set(u8 key_val);
void Cap_Para_Set(u8 key_val);

void Cap_Adjust(u8 key_val);
void Meter_Disp(u8 key_val);
void Sampling_Adiust(uint8_t key_val);
void Event_Log(uint8_t key_val);
void Data_Stat(uint8_t key_val);
void Control_Para_Set(u8 key_val);

void Low_Disp(void);
void Pass_Word(u8 key_val);
void Sys_Static_Para_Set(u8 key_val);
void Sampling(uint8_t key_val);
void Err_Deal(uint8_t key_val);
void Event_Detail_Menu(uint8_t key_val);
extern MenuState menu_state; // 当前菜单状态

extern char state;          // 
extern char edit_col;          // 
extern char item_id;        // 菜单项选项
extern char item3;        // 菜单项选项
extern char item1;        // 菜单项选项

#endif /* __MENU_H__ */