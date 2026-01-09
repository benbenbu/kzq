
#include "config.h"
#include "menu.h"
#include "time.h "
#include "adc_key.h"
#include "lcd.h"
#include "data_save.h"
#include "data_deal.h"
#include "font_date.h"
#include "HT7036.h"
#include "fm31256.h"
#include <string.h>
/**********变量定义**********/

MenuState menu_state;       // 当前菜单状态
uint32_t menu_current_time; // 菜单当前时间
uint32_t menu_last_time;    // 菜单当前时间
// 反显光标坐标
uint8_t cur_row = 0; // 当前行位置
uint8_t cur_col = 0; // 当前列位置
char item_id;        // 菜单项选项
char item1;          // 主菜单项
char item2;          // 参数菜单项
char item3;          // 保护菜单项
char state;          // 菜单状态
char edit_col;       // 光标位置

void menu_init()
{
  menu_state.id = METER_DISP;
  menu_state.last_id = METER_DISP;
  menu_state.switch_temp_flag = 1;
  menu_current_time = Get_SystemTime();
  menu_last_time = menu_current_time;
  item_id = 1;
  item1 = 1;
  item2 = 1;
  item3 = 1;
  state = 0;
  edit_col = 0;
}

// 菜单显示函数
void menu_disp()
{

  uint8_t key_val;
  menu_current_time = Get_SystemTime();

  // ========== 第二步：处理界面切换/按键标记（原有逻辑保留） ==========
  if (menu_state.switch_temp_flag == 1)
  {
    // 切换标记生效：设置刷新标记，重置临时标记
    menu_state.refresh_flag = 1;
    menu_state.switch_temp_flag = 0;
    key_val = KEY_NONE; // 切换时无真实按键
    // 切换时更新最后操作时间（避免刚切换就触发超时）
    menu_last_time = menu_current_time;
  }
  else
  {
    // 无切换时，正常读取按键
    key_val = bsp_GetKey();
    // 有真实按键时，标记刷新
    menu_state.refresh_flag = (key_val != KEY_NONE) ? 1 : 0;
  }

  // ========== 第三步：整合3分钟无按键自动返回逻辑 ==========
  if (key_val != KEY_NONE)
  {
    // 有按键：更新最后操作时间
    menu_last_time = menu_current_time;
  }
  else
  {
    // 无按键：判断是否超时返回
    if (menu_state.id != METER_DISP)
    { // 非计量界面
      // 计算无按键时长（uint32_t避免溢出）
      if ((uint32_t)(menu_current_time - menu_last_time) > 180000L)
      {
        // 超时：强制切回计量界面
        menu_state.id = METER_DISP;
        menu_state.switch_temp_flag = 1;    // 标记切换，保证返回后刷新
        menu_last_time = menu_current_time; // 重置超时时间戳
        LCD_Clear();                        // 清屏，避免界面残留
				item_id = 1;
				item1 = 1;
				item2 = 1;
				item3 = 1;
				state = 0;
				
      }
    }
    else
    {
      // 计量界面：重置超时时间（避免计量界面自身触发超时）
      menu_last_time = menu_current_time;
    }
  }

  // ========== 第四步：处理定时/数据更新刷新（计量界面专属，原有逻辑保留） ==========
  if (menu_state.id == METER_DISP)
  {
    // 1s定时刷新
    if (menu_state.timer_1s_flag == 1)
    {
      menu_state.refresh_flag = 1;
      menu_state.timer_1s_flag = 0;
    }
    // 计量数据更新刷新
    if (menu_state.meter_data_flag == 1)
    {
      menu_state.refresh_flag = 1;
      menu_state.meter_data_flag = 0;
    }
  }

  if ((menu_state.refresh_flag == 1))
  {
    switch (menu_state.id)
    {
    case METER_DISP:
      Meter_Disp(key_val); // 计量界面
      break;
    case MAIN_MENU:
      Main_Menu(key_val); // 主菜单
      break;
    case SYSTEM_SETTING:
      System_Setting(key_val); // 系统设置
      break;
    case PARAM_SETTING:
      Para_Menu(key_val); // 参数设置
      break;
    case ADJUST_CAP:
      Cap_Adjust(key_val); // 调试投切
      break;
    case SAMPLING_CALIBRATION:
      Sampling_Adiust(key_val); // 采样校准
      break;
    case EVENT_LOG: // 事件记录
      Event_Log(key_val);
      break;
    case DATA_STATISTICS: // 数据统计
      Data_Stat(key_val);
      break;
    case SYSTEM_PARAM: // 系统参数
      Sys_Para_Set(key_val);
      break;
    case CAP_PARAM: // 电容参数
      Cap_Para_Set(key_val);
      break;
    case CTRL_PARAM: // 控制参数
      Control_Para_Set(key_val);
      break;
    case PROT_PARAM: // 保护参数
      Protect_Menu(key_val);
      break;
    case COMM_PARAM: // 通讯参数
      Sys_Com_Para_Set(key_val);
      break;
		
    case STATISTIC_PARAM: // 统计参数		
		 Sys_Static_Para_Set( key_val);
		break;
    case PROT_PARAM_VOL_H:
      Sys_Vol_Prot_Para_Set(key_val, VOL_PROT_H); // 过压保护
      break;
    case PROT_PARAM_VOL_L:
      Sys_Vol_Prot_Para_Set(key_val, VOL_PROT_L); // 欠压保护
      break;
    case PROT_CAP1:
      Sys_Cap_Prot_Para_Set(key_val, 0); // 一路保护
      break;
    case PROT_CAP2:
      Sys_Cap_Prot_Para_Set(key_val, 1); // 一路保护// 一路保护
      break;
    case PROT_CAP3:
      Sys_Cap_Prot_Para_Set(key_val, 2); // 一路保护// 一路保护
      break;
    case PROT_CAP4:
      Sys_Cap_Prot_Para_Set(key_val, 3); // 一路保护// 一路保护
      break;

    case PASSWORD:
      Pass_Word(key_val); // 保护口令
      break;

    case SAMPLING:		
	   Sampling( key_val);		
		break;
    }
    Low_Disp();
    menu_state.refresh_flag = 0;
    key_val = KEY_NONE; // 清空按键值，防止重复处理
  }
}

/***********显示辅助函数********/

// 辅助：计算 10^n（n <= 9 安全）

static int pow10(int n)
{
  static const int p10[] = {1, 10, 100, 1000, 10000, 100000};
  return (n >= 0 && n < 6) ? p10[n] : 0;
}

// 1. 处理uint8_t（C89标准：变量集中定义在函数开头）
void adjust_digit_u8(uint8_t *value_ptr, uint8_t edit_col, int delta, uint8_t total_digits)
{
#define MAX_DIGITS_U8 3
  // 所有变量集中定义在执行语句前（C89强制要求）
  int pos_from_right;
  long base;
  uint8_t old_val;
  long digit;
  long new_val; // 用long暂存，避免计算溢出

  // 执行逻辑（变量定义后再写执行语句）
  if (edit_col >= total_digits || total_digits == 0 || total_digits > MAX_DIGITS_U8)
  {
    return;
  }

  pos_from_right = total_digits - 1 - edit_col;
  base = pow10(pos_from_right);
  if (base == 0)
  {
    return;
  }

  old_val = *value_ptr;
  digit = (old_val / base) % 10;
  digit = (digit + delta + 10) % 10; // 循环加减，避免负数

  // 先计算新值（用long避免溢出），再强转回uint8_t
  new_val = (long)old_val - ((old_val / base) % 10) * base + digit * base;
  *value_ptr = (uint8_t)new_val;
}
void adjust_digit_u81(uint8_t *value_ptr, uint8_t edit_col, int delta, uint8_t total_digits)
{
  // 所有变量集中定义在执行语句前（C89强制要求）
  int pos_from_right;
  long base;
  uint8_t old_val;
  long digit;
  long new_val; // 用long暂存，避免计算溢出

  // 执行逻辑（变量定义后再写执行语句）
  if (edit_col >= total_digits || total_digits == 0 || total_digits > 3)
  {
    return;
  }

  pos_from_right = total_digits - 1 - edit_col;
  base = pow10(pos_from_right);
  if (base == 0)
  {
    return;
  }

  old_val = *value_ptr;
  digit = (old_val / base) % 10;

  // ========== 核心修改：百位（edit_col=0）特殊限制 ==========
  if (edit_col == 0)
  { // 仅针对百位（edit_col=0）
    digit += delta;
    // 百位规则：0~1循环，超过1置0，低于0置1（兼容减操作）
    if (digit > 1)
    {
      digit = 0;
    }
    else if (digit < 0)
    { // 兼容减delta的场景（如delta=-1）
      digit = 1;
    }
  }
  else
  {                                    // 十位/个位（edit_col=1/2）保留原有循环逻辑
    digit = (digit + delta + 10) % 10; // 循环加减，避免负数
  }

  // 先计算新值（用long避免溢出），再强转回uint8_t
  new_val = (long)old_val - ((old_val / base) % 10) * base + digit * base;
  *value_ptr = (uint8_t)new_val;
}
// 2. 处理uint16_t（同上，变量集中定义）
void adjust_digit_u16(uint16_t *value_ptr, uint8_t edit_col, int delta, uint8_t total_digits)
{
  // 所有变量集中定义在开头
  int pos_from_right;
  long base;
  uint16_t old_val;
  long digit;
  long new_val;

  if (edit_col >= total_digits || total_digits == 0 || total_digits > 5)
  {
    return;
  }

  pos_from_right = total_digits - 1 - edit_col;
  base = pow10(pos_from_right);
  if (base == 0)
  {
    return;
  }

  old_val = *value_ptr;
  digit = (old_val / base) % 10;
  digit = (digit + delta + 10) % 10;

  new_val = (long)old_val - ((old_val / base) % 10) * base + digit * base;
  *value_ptr = (uint16_t)new_val;
}

// 调整一个字节（0~255）的十六进制某一位
// edit_col: 0 = 高位（左），1 = 低位（右）
// delta: +1 或 -1
static void adjust_hex_digit(uint8_t *value, uint8_t edit_col, int delta)
{
  uint8_t digit;
  if (edit_col > 1)
    return; // 只支持2位

  if (edit_col == 0)
  {
    digit = (*value >> 4) & 0x0F; // 高4位
  }
  else
  {
    digit = *value & 0x0F; // 低4位
  }

  // 十六进制循环：0~15（0~F）
  digit = (digit + delta + 16) % 16;

  // 写回
  if (edit_col == 0)
  {
    *value = (*value & 0x0F) | (digit << 4);
  }
  else
  {
    *value = (*value & 0xF0) | digit;
  }
}
// 电容显示  type  0: c1 放下面  1：c1放左边
void cap_disp(uint8_t type)
{

  uint8_t x = 21;
  uint8_t y = 60;
  uint8_t i;
  switch (g_cap_num.cap_num)
  {
  case 1:
    x = 111;
    y = 0;
    break;

  case 2:
    x = 21;
    y = 180;
    break;
  case 3:
    x = 21;
    y = 90;
    break;
  case 4:
    x = 21;
    y = 60;
    break;
  }

  for (i = 0; i < g_cap_num.cap_num; i++)
  {
    if (g_cap[i].onf)
    {
      if (g_cap[i].state)
        LCD_DisplayBpm(x, 48, 36, PICTURE2);
      else
        LCD_DisplayBpm(x, 48, 36, PICTURE1);

      if (i == 0)
      {
        if (type)
          LCD_DisplayString(x - 21, 68, "C1", 16, 0);
        else
          LCD_DisplayString(x, 90, "C1", 16, 0);
      }
      if (i == 1)
      {
        if (type)
          LCD_DisplayString(x - 21, 68, "C2", 16, 0);
        else
          LCD_DisplayString(x, 90, "C2", 16, 0);
      }
      if (i == 2)
      {
        if (type)
          LCD_DisplayString(x - 21, 68, "C3", 16, 0);
        else
          LCD_DisplayString(x, 90, "C3", 16, 0);
      }
      if (i == 3)
      {
        if (type)
          LCD_DisplayString(x - 21, 68, "C4", 16, 0);
        else
          LCD_DisplayString(x, 90, "C4", 16, 0);
      }
    }
    x = x + y;
  }

  LCD_DisplayBpm(111, 0, 48, PICTURE);
  LCD_DisplayLine(30, 48, 210, 1);
}

//void cap_disp(void)
//{

//  uint8_t x = 21;
//  uint8_t y = 60;
//  uint8_t i;

//  switch (g_cap_num.cap_num)
//  {
//  case 1:
//    x = 111;
//    y = 0;
//    break;

//  case 2:
//    x = 21;
//    y = 180;
//    break;
//  case 3:
//    x = 21;
//    y = 90;
//    break;
//  case 4:
//    x = 21;
//    y = 60;
//    break;
//  }

//  for (i = 0; i < g_cap_num.cap_num; i++)
//  {
//    if (g_cap[i].onf)
//    {
//      if (g_cap[i].state)
//        LCD_DisplayBpm(x, 48, 36, PICTURE2);
//      else
//        LCD_DisplayBpm(x, 48, 36, PICTURE1);

//      if (i == 0)
//      {

//        LCD_DisplayString(x - 21, 68, "C1", 16, 0);
//      }
//      if (i == 1)
//      {
//        LCD_DisplayString(x - 21, 68, "C2", 16, 0);
//      }
//      if (i == 2)
//      {
//        LCD_DisplayString(x - 21, 68, "C3", 16, 0);
//      }
//      if (i == 3)
//      {
//        LCD_DisplayString(x - 21, 68, "C4", 16, 0);
//      }
//    }
//    x = x + y;
//  }

//  LCD_DisplayBpm(111, 0, 48, PICTURE);
//  LCD_DisplayLine(30, 48, 210, 1);
//}

// 工具函数：单个数字（0-99）拆分为两位字符（补零），直接显示
// num：0-99的数字；x/y：起始坐标；size：字体大小；inverse：反显
void disp_two_digit(uint8_t num, uint16_t x, uint8_t y, uint8_t size, uint8_t inverse)
{

  // 2. 拆分十位和个位
  uint8_t ten = num / 10; // 十位：如12→1，5→0
  uint8_t one = num % 10; // 个位：如12→2，5→5
  char ten_str[2] = {0};  // 存储十位字符的字符串（如"1"、"0"）
  char one_str[2] = {0};  // 存储个位字符的字符串（如"2"、"5"）
  // 1. 合法性检查：确保num是两位数（0-99），超出则显示99
  if (num > 99)
  {
    num = 99;
  }

  // 3. 定义临时字符串（必须以'\0'结尾，适配LCD_DisplayString的参数要求）

  // 4. 把数字转为字符，并赋值给字符串
  ten_str[0] = '0' + ten; // 数字0→'0'，数字1→'1'...
  one_str[0] = '0' + one;
  LCD_DisplayString(x, y, ten_str, size, inverse);     // 显示十位
  LCD_DisplayString(x + 9, y, one_str, size, inverse); // 显示个位（偏移对应宽度）
}

// 优化后的时间显示函数（无sprintf，无字符串数组）
void time_disp(Date_Struct date, Time_Struct time, uint16_t x, uint8_t y)
{
  // ========== 显示日期：xx/xx/xx ==========
  // 年份（两位）
  disp_two_digit(date.year, x, y, 8, 0);
  // 斜杠 '/'
  LCD_DisplayChar(x + 18, y, "/", 8, 0); // 12=6*2（两个数字宽度）
  // 月份
  disp_two_digit(date.month, x + 27, y, 8, 0); //
  // 斜杠 '/'
  LCD_DisplayChar(x + 45, y, "/", 8, 0); //
  // 日期
  disp_two_digit(date.day, x + 54, y, 8, 0); //

  // ========== 显示时间：xx:xx:xx ==========
  // 小时
  disp_two_digit(time.hour, x, y + 8, 8, 0);
  // 冒号 ':'
  LCD_DisplayChar(x + 18, y + 8, ":", 8, 0);
  // 分钟
  disp_two_digit(time.minute, x + 27, y + 8, 8, 0);
  // 冒号 ':'
  LCD_DisplayChar(x + 45, y + 8, ":", 8, 0); //
  // 秒
  disp_two_digit(time.second, x + 54, y + 8, 8, 0); //
}


/* 
 * @brief  判断数组中所有启用的电容参数value是否相等
 * @param  cap_array: 电容参数数组首地址
 * @param  n: 数组长度
 * @return uint8_t: 0=不相等/无启用元素，1=所有启用元素的value相等
 * @note   1. 仅判断onf=1的元素，onf=0的忽略；
 *         2. 若只有1个启用元素，返回1（单个元素默认“相等”）；
 *         3. 若无任何启用元素，返回0（无判断意义）。
 */
uint8_t Cap_CheckAllEnabledValueEqual(Cap_Para_Struct *cap_array, uint8_t n)
{
    /* C89要求：变量必须在代码块开头声明 */
    uint16_t ref_value;
    uint8_t enabled_count;
    uint8_t i;

    /* 初始化变量（C89建议显式初始化） */
    ref_value = 0;
    enabled_count = 0;

    /* 边界检查：数组为空/长度为0，直接返回0 */
    if (cap_array == (Cap_Para_Struct *)0 || n == 0) {
        return 0;
    }

    /* 第一步：遍历数组，找到第一个启用的元素，记录其value */
    for (i = 0; i < n; i++) {
        if (cap_array[i].onf == 1) {
            ref_value = cap_array[i].value;
            enabled_count++;
            break; /* 找到第一个启用元素，退出循环 */
        }
    }

    /* 第二步：无任何启用元素，返回0 */
    if (enabled_count == 0) {
        return 0;
    }

    /* 第三步：遍历剩余元素，检查所有启用元素的value是否等于参考值 */
    for (i = 0; i < n; i++) {
        /* 跳过禁用元素 */
        if (cap_array[i].onf != 1) {
            continue;
        }
        /* 找到不相等的value，直接返回0 */
        if (cap_array[i].value != ref_value) {
            return 0;
        }
        enabled_count++;
    }

    /* 所有启用元素的value都相等，返回1 */

    return 1;
}

/*********菜单部分********/

/*********计量界面显示********/
void Meter_Disp(uint8_t key_val)
{
  uint8_t str[20];
  uint16_t dat;
  uint8_t uint, f;
  uint8_t x, y, i;
  meter_data(g_meter_chip[3].rp, 1, &dat, &uint, &f);
  data_disp(dat, uint, f, 1, str);

  LCD_DisplayString(0, 0, str, 8, 0);

  meter_data(g_meter_chip[3].u_a, 2, &dat, &uint, &f);
  data_disp(dat, uint, f, 2, str);

  LCD_DisplayString(0, 9, str, 8, 0);

  meter_data(g_meter_chip[3].i_a, 3, &dat, &uint, &f);
  data_disp(dat, uint, f, 3, str);

  LCD_DisplayString(0, 18, str, 8, 0);

  meter_data(g_meter_chip[3].rq, 4, &dat, &uint, &f);
  data_disp(dat, uint, f, 4, str);

  LCD_DisplayString(0, 27, str, 8, 0);

  meter_data(g_meter_chip[3].rq, 5, &dat, &uint, &f);
  data_disp(dat, uint, f, 5, str);

  LCD_DisplayString(0, 36, str, 8, 0);

  switch (g_cap_num.pro_num)
  {
  case 1:
    x = 93;
    y = 0;
    break;

  case 2:
    x = 0;
    y = 180;
    break;
  case 3:
    x = 0;
    y = 90;
    break;
  case 4:
    x = 0;
    y = 60;
    break;
  }

  for (i = 1; i <= g_cap_num.pro_num; i++)
  {
    cap_data_disp(i, 1, g_cap_data[i - 1].ia, 0, str);
    LCD_DisplayString(x, 86, str, 8, 0);
    cap_data_disp(i, 2, g_cap_data[i - 1].ic, 0, str);
    LCD_DisplayString(x, 94, str, 8, 0);
    cap_data_disp(i, 3, g_cap_data[i - 1].uo, 1, str);
    LCD_DisplayString(x, 102, str, 8, 0);
    x = x + y;
  }
  
  cap_disp(g_cap_num.pro_num);

  switch (key_val)
  {
  case KEY_0_UP: // 上：同列上移，循环

    break;

  case KEY_1_UP: // 下：同列下移，循环

    break;

  case KEY_2_UP: // 左键

    break;

  case KEY_4_UP:
    menu_state.id = MAIN_MENU;
    menu_state.switch_temp_flag = 1;
    LCD_Clear();
    return;
  case KEY_5_UP:
  default:
    break;
  }
}

/*********下端显示********/
void Low_Disp(void)
{
  if (menu_state.id != ADJUST_CAP)
    LCD_DisplayLine(0, 110, 240, 0);
  if ((menu_state.id == METER_DISP) || (menu_state.id == SYSTEM_SETTING) || (menu_state.id == MAIN_MENU) || (menu_state.id == PARAM_SETTING) || (menu_state.id == PROT_PARAM)\ 
		|| (menu_state.id == DATA_STATISTICS) || (menu_state.id == EVENT_LOG) || (menu_state.id == PASSWORD) || (menu_state.id == SAMPLING_CALIBRATION)|| (menu_state.id == STATISTIC_PARAM)\
	|| (menu_state.id == SAMPLING))
  {
    LCD_DisplayString(129, 112, "时间", 16, 0);
    time_disp(system_date, system_time, 171, 112);

    if (g_adjust_cap.mode)
    {
      LCD_DisplayString(6, 112, "手动", 16, 0);
    }
    else
      LCD_DisplayString(6, 112, "自动", 16, 0);
  }
}

/******主菜单**********/

void Main_Menu(uint8_t key_val)
{
  code const char *g_main_menu_titles[7] = {

      "主菜单", "系统设置", "参数\xFD设置", "调试投切", "采样校准", "事件记录", "数\xFD据统计"};

  switch (key_val)
  {
  case KEY_0_UP: // 上：同列上移，循环
    item1 = item1 - 2;
    if (item1 <= 0)
      item1 = item1 + 6;
    break;

  case KEY_1_UP: // 下：同列下移，循环
    item1 = item1 + 2;
    if (item1 > 6)
      item1 = item1 - 6;
    break;

  case KEY_2_UP: // 左键
    item1 = item1 - 1;
    if (item1 <= 0)
      item1 = item1 + 6;
    break;

  case KEY_3_UP: // 右键
    item1 = item1 + 1;
    if (item1 > 6)
      item1 = item1 - 6;
    break;

  case KEY_4_UP:
    switch (item1)
    {
    case 1:
      menu_state.id = PASSWORD;
      menu_state.last_id = SYSTEM_SETTING;
      break; //
    case 2:
      menu_state.id = PASSWORD;
      menu_state.last_id = PARAM_SETTING;
      break; //
    case 3:
      menu_state.last_id = ADJUST_CAP;
      menu_state.id = PASSWORD;
      break; //
    case 4:
      menu_state.id = PASSWORD;
      menu_state.last_id = SAMPLING_CALIBRATION;
      break; //
    case 5:
      menu_state.id = EVENT_LOG;
      break; //
    case 6:
      menu_state.id = DATA_STATISTICS;
      break; //
    }
    LCD_Clear();
    menu_state.switch_temp_flag = 1;
		state=0;
    return;
    break;
  case KEY_5_UP:
    menu_state.id = METER_DISP;
    LCD_Clear();
    menu_state.switch_temp_flag = 1;
	  state=0;
	  item1=1;
    return;
    break;
  default:
    break;
  }

  // ========== 显示菜单 ==========

  LCD_DisplayString(93, 0, g_main_menu_titles[0], 16, 0);
  LCD_DisplayString(24, 24, g_main_menu_titles[1], 16, item1 == 1 ? 1 : 0);
  LCD_DisplayString(144, 24, g_main_menu_titles[2], 16, item1 == 2 ? 1 : 0);
  LCD_DisplayString(24, 48, g_main_menu_titles[3], 16, item1 == 3 ? 1 : 0);
  LCD_DisplayString(144, 48, g_main_menu_titles[4], 16, item1 == 4 ? 1 : 0);
  LCD_DisplayString(24, 72, g_main_menu_titles[5], 16, item1 == 5 ? 1 : 0);
  LCD_DisplayString(144, 72, g_main_menu_titles[6], 16, item1 == 6 ? 1 : 0);
}

/******参数设置菜单**********/

typedef enum
{
  PARA_SYS = 0,     // 0：系统参数 - 标题索引1
  PARA_CAP = 1,     // 1：电容参数 - 标题索引2
  PARA_CTRL = 2,    // 2：控制参数 - 标题索引3
  PARA_PROTECT = 3, // 3：保护参数 - 标题索引4
  PARA_COMM = 4,    // 4：通讯参数 - 标题索引5
  PARA_STAT = 5     // 5：统计参数 - 标题索引6
} ParaMenuID;

void Para_Menu(uint8_t key_val)
{

  // 6. 参数菜单多语言标题数组（核心：对应ID的中英文文本）
  code const char *g_para_menu_titles[7] = {
      // [LANG_ZH] 中文

      "参数\xFD设置", // 索引0：主标题
      "系统参数\xFD", // 索引1：PARA_SYS(0)
      "电容参数\xFD", // 索引2：PARA_CAP(1)
      "控制参数\xFD", // 索引3：PARA_CTRL(2)
      "保护参数\xFD", // 索引4：PARA_PROTECT(3)
      "通讯参数\xFD", // 索引5：PARA_COMM(4)
      "统计参数\xFD"  // 索引6：PARA_STAT(5)

  };

  switch (key_val)
  {
  case KEY_0_UP: // 上：同列上移，循环
    item2 = item2 - 2;
    if (item2 <= 0)
      item2 = item2 + 6;
    break;

  case KEY_1_UP: // 下：同列下移，循环
    item2 = item2 + 2;
    if (item2 > 6)
      item2 = item2 - 6;
    break;

  case KEY_2_UP: // 左键
    item2 = item2 - 1;
    if (item2 <= 0)
      item2 = item2 + 6;
    break;

  case KEY_3_UP: // 右键
    item2 = item2 + 1;
    if (item2 > 6)
      item2 = item2 - 6;
    break;

  case KEY_4_UP:
    switch (item2)
    {
    case 1:
      menu_state.id = SYSTEM_PARAM;
      break; //
    case 2:
      menu_state.id = CAP_PARAM;
      break; //
    case 3:
      menu_state.id = CTRL_PARAM;
      break; //
    case 4:
      menu_state.id = PROT_PARAM;
      break; //
    case 5:
      menu_state.id = COMM_PARAM;
      break; //
//    case 6:
//      menu_state.id = STATISTIC_PARAM;
//      break; //
    }
    menu_state.switch_temp_flag = 1;
    LCD_Clear();
		state=0;
    return;
    break;
  case KEY_5_UP:
    menu_state.id = MAIN_MENU;
    menu_state.switch_temp_flag = 1;
    LCD_Clear();
	  state=0;
	  item2=1;
    return;
    break;
  default:
    break;
  }

  // ========== 显示菜单：从多语言数组读取文本 ==========

  // 主标题（索引0）
  LCD_DisplayString(81, 0, g_para_menu_titles[0], 16, 0);
  // 第一行左：系统参数（索引1）
  LCD_DisplayString(24, 24, g_para_menu_titles[1], 16, item2 == 1 ? 1 : 0);
  // 第一行右：电容参数（索引2）
  LCD_DisplayString(144, 24, g_para_menu_titles[2], 16, item2 == 2 ? 1 : 0);
  // 第二行左：控制参数（索引3）
  LCD_DisplayString(24, 48, g_para_menu_titles[3], 16, item2 == 3 ? 1 : 0);
  // 第二行右：保护参数（索引4）
  LCD_DisplayString(144, 48, g_para_menu_titles[4], 16, item2 == 4 ? 1 : 0);
  // 第三行左：通讯参数（索引5）
  LCD_DisplayString(24, 72, g_para_menu_titles[5], 16, item2 == 5 ? 1 : 0);
  // 第三行右：统计参数（索引6）
  LCD_DisplayString(144, 72, g_para_menu_titles[6], 16, item2 == 6 ? 1 : 0);
}

/*********系统设置菜单部分********/

void System_Setting(u8 key_val)
{
  // 参数
  u8 y = 0; // 行间隔
  u8 j;

  static Time_Struct time;
  static Date_Struct date;
  static Pass_Para_Struct pass = {0};
  static Alarm_Para_Struct var1 = {0};
  static Contra_Para_Struct var2 = {0};

  code const char *g_systemset_menu_titles[6] = {
      "系统设置",
      "保护口令",
      "日期设置",
      "时间设置",
      "报警设置",
      "对比度设置"

  };

  if (state == 0) // 第一次进入菜单，系统变量赋值到临时变量。初始化坐标，浏览模式，编辑坐标为0
  {

    pass = g_protect_password;
    date = system_date;
    time = system_time;

    var1 = g_alarm_minutes;
    var2 = g_lcd_contrast;
    edit_col = 0;
    state = 1;
    item_id = 1;
  }
  else if (state == 1)
  { // 浏览模式
    switch (key_val)
    {
    case KEY_0_UP:
      item_id--;
      if (item_id < 1)
        item_id = 5;
      break;
    case KEY_1_UP:
      item_id++;
      if (item_id > 5)
        item_id = 1;
      break;
    case KEY_4_UP:
      state = 2; // 进入编辑
      break;

    case KEY_5_UP:
      menu_state.id = MAIN_MENU;

      state = 0;
      menu_state.switch_temp_flag = 1;
      LCD_Clear();
      return;
      break;
    }
    edit_col = 0;
  }
  else if (state == 2) // 参数编辑
  {
    switch (key_val)
    {
    case KEY_0_UP: // 加1
      if (item_id == 1)
      {
        pass.pass_code[edit_col] += 1;
        if (pass.pass_code[edit_col] > 9)
          pass.pass_code[edit_col] = 0;
      }
      if (item_id == 2) // 编辑年月日（修正加1逻辑）
      {
        if (edit_col < 2)
        {
          // 年份（2位：00-99循环）
          adjust_digit_u8(&date.year, edit_col, 1, 2);
        }
        else if (edit_col < 4) // 月份（1-12循环）
        {
          uint8_t digit1 = date.month / 10; // 月十位（0/1）
          uint8_t digit0 = date.month % 10; // 月个位

          switch (edit_col)
          {
          case 2: // 月十位（0→1，1→0循环）
            digit1 = (digit1 + 1) % 2;
            break;
          case 3: // 月个位（受十位约束）
            if (digit1 == 0)
            {
              // 十位=0（1-9月）：个位0-9循环
              digit0 = (digit0 + 1) % 10;
            }
            else // 十位=1（10-12月）：个位0-2循环
            {
              digit0 = (digit0 + 1) % 3;
            }
            break;
          }

          // 修正无效月份（0→1，13→1）
          date.month = digit1 * 10 + digit0;
        }
        else if (edit_col < 6) // 日期（1-31循环）
        {
          uint8_t digit1 = date.day / 10; // 日十位（0/1/2/3）
          uint8_t digit0 = date.day % 10; // 日个位

          switch (edit_col)
          {
          case 4: // 日十位（0→1→2→3→0循环）
            digit1 = (digit1 + 1) % 4;
            break;
          case 5: // 日个位（受十位约束）
            if (digit1 == 3)
            {
              // 十位=3（30-31日）：个位0-1循环
              digit0 = (digit0 + 1) % 2;
            }
            else // 十位=0/1/2（1-9/10-19/20-29日）：个位0-9循环
            {
              digit0 = (digit0 + 1) % 10;
            }
            break;
          }

          // 修正无效日期（0→1，32→1）
          date.day = digit1 * 10 + digit0;
        }
      }
      if (item_id == 3) // 编辑时分秒（time.hour/minute/second，00:00:00格式）
      {
        if (edit_col < 2) // 编辑小时（00-23）
        {
          uint8_t hour_tens = time.hour / 10;  // 时十位（0-2）
          uint8_t hour_units = time.hour % 10; // 时个位（0-9/0-3）

          switch (edit_col)
          {
          case 0: // 时十位（0→1→2→0循环）
            hour_tens = (hour_tens + 1) % 3;
            // 若十位从2→0，个位同步置0（避免23→0x无效值，直接跳00）
            if (hour_tens == 0)
              hour_units = 0;
            break;
          case 1: // 时个位（受十位约束）
            if (hour_tens == 2)
            {
              // 十位=2（20-23），个位0-3循环（加1）
              hour_units = (hour_units + 1) % 4;
            }
            else
            {
              // 十位=0/1（00-19），个位0-9循环（加1）
              hour_units = (hour_units + 1) % 10;
            }
            break;
          }

          // 组合小时，确保00-23（冗余防护）
          time.hour = hour_tens * 10 + hour_units;
          if (time.hour > 23)
            time.hour = 0;
        }
        else if (edit_col < 4) // 编辑分钟（00-59）
        {
          uint8_t min_tens = time.minute / 10;  // 分十位（0-5）
          uint8_t min_units = time.minute % 10; // 分个位（0-9）

          switch (edit_col)
          {
          case 2: // 分十位（0→1→...→5→0循环）
            min_tens = (min_tens + 1) % 6;
            break;
          case 3: // 分个位（0→9循环）
            min_units = (min_units + 1) % 10;
            break;
          }

          // 组合分钟，确保00-59
          time.minute = min_tens * 10 + min_units;
        }
        else if (edit_col < 6) // 编辑秒（00-59，与分钟逻辑一致）
        {
          uint8_t sec_tens = time.second / 10;  // 秒十位（0-5）
          uint8_t sec_units = time.second % 10; // 秒个位（0-9）

          switch (edit_col)
          {
          case 4: // 秒十位（0→1→...→5→0循环）
            sec_tens = (sec_tens + 1) % 6;
            break;
          case 5: // 秒个位（0→9循环）
            sec_units = (sec_units + 1) % 10;
            break;
          }

          // 组合秒，确保00-59
          time.second = sec_tens * 10 + sec_units;
        }
      }
      if (item_id == 4)
      {
        var1.value++;
        if (var1.value > 61)
          var1.value = 0; // 保留原有
      }
      if (item_id == 5)
      {
        var2.value++;
				 LCD_Darker(var2.value);
      }
      break;

    case KEY_1_UP: // 减1
      if (item_id == 1)
      {
        pass.pass_code[edit_col] -= 1;
        if (pass.pass_code[edit_col] < 0)
          pass.pass_code[edit_col] = 9;
      }
      if (item_id == 2) // 编辑年月日（补充减1逻辑）
      {
        if (edit_col < 2)
        {
          // 年份（2位：00-99循环）
          // 年份（2位：00-99循环）
          adjust_digit_u8(&date.year, edit_col, -1, 2);
        }
        else if (edit_col < 4) // 月份（1-12循环）
        {
          uint8_t digit1 = date.month / 10; // 月十位（0/1）
          uint8_t digit0 = date.month % 10; // 月个位

          switch (edit_col)
          {
          case 2: // 月十位（1→0，0→1循环）
            digit1 = (digit1 - 1 + 2) % 2;
            break;
          case 3: // 月个位（受十位约束）
            if (digit1 == 0)
            {
              // 十位=0（1-9月）：个位0→9循环
              digit0 = (digit0 - 1 + 10) % 10;
            }
            else // 十位=1（10-12月）：个位0→2循环
            {
              digit0 = (digit0 - 1 + 3) % 3;
            }
            break;
          }

          // 修正无效月份（0→12）
          date.month = digit1 * 10 + digit0;
        }
        else if (edit_col < 6) // 日期（1-31循环）
        {
          uint8_t digit1 = date.day / 10; // 日十位（0/1/2/3）
          uint8_t digit0 = date.day % 10; // 日个位

          switch (edit_col)
          {
          case 4: // 日十位（3→2→1→0→3循环）
            digit1 = (digit1 - 1 + 4) % 4;
            break;
          case 5: // 日个位（受十位约束）
            if (digit1 == 3)
            {
              // 十位=3（30-31日）：个位0→1循环
              digit0 = (digit0 - 1 + 2) % 2;
            }
            else // 十位=0/1/2：个位0→9循环
            {
              digit0 = (digit0 - 1 + 10) % 10;
            }
            break;
          }

          // 修正无效日期（0→31）
          date.day = digit1 * 10 + digit0;
        }
      }
      if (item_id == 3) // 编辑时分秒（time.hour/minute/second，00:00:00格式）
      {
        if (edit_col < 2) // 编辑小时（00-23）
        {
          uint8_t hour_tens = time.hour / 10;  // 时十位（0-2）
          uint8_t hour_units = time.hour % 10; // 时个位（0-9/0-3）

          switch (edit_col)
          {
          case 0: // 时十位（2→1→0→2循环）
            hour_tens = (hour_tens - 1 + 3) % 3;
            // 若十位从0→2，个位同步置3（避免0x→23，符合最大23点）
            if (hour_tens == 2)
              hour_units = 3;
            break;
          case 1: // 时个位（受十位约束）
            if (hour_tens == 2)
            {
              // 十位=2（20-23），个位3→2→1→0→3循环（减1）
              hour_units = (hour_units - 1 + 4) % 4;
            }
            else
            {
              // 十位=0/1（00-19），个位9→8→...→0→9循环（减1）
              hour_units = (hour_units - 1 + 10) % 10;
            }
            break;
          }

          // 组合小时，确保00-23（冗余防护）
          time.hour = hour_tens * 10 + hour_units;
          if (time.hour > 23)
            time.hour = 23;
        }
        else if (edit_col < 4) // 编辑分钟（00-59）
        {
          uint8_t min_tens = time.minute / 10;  // 分十位（0-5）
          uint8_t min_units = time.minute % 10; // 分个位（0-9）

          switch (edit_col)
          {
          case 2: // 分十位（5→4→...→0→5循环）
            min_tens = (min_tens - 1 + 6) % 6;
            break;
          case 3: // 分个位（9→8→...→0→9循环）
            min_units = (min_units - 1 + 10) % 10;
            break;
          }

          // 组合分钟，确保00-59
          time.minute = min_tens * 10 + min_units;
        }
        else if (edit_col < 6) // 编辑秒（00-59，与分钟逻辑一致）
        {
          uint8_t sec_tens = time.second / 10;  // 秒十位（0-5）
          uint8_t sec_units = time.second % 10; // 秒个位（0-9）

          switch (edit_col)
          {
          case 4: // 秒十位（5→4→...→0→5循环）
            sec_tens = (sec_tens - 1 + 6) % 6;
            break;
          case 5: // 秒个位（9→8→...→0→9循环）
            sec_units = (sec_units - 1 + 10) % 10;
            break;
          }

          // 组合秒，确保00-59
          time.second = sec_tens * 10 + sec_units;
        }
      }
      if (item_id == 4)
      {
        var1.value--;
        if (var1.value < 0)
          var1.value = 61; // 保留原有
      }
      if (item_id == 5)
      {
        var2.value--;
				 LCD_Darker(var2.value);
      }
      break;
    case KEY_2_UP:
      edit_col--;
      if (item_id < 4)
      {
        if (edit_col < 0)
          edit_col = 5;
      }

      break;
    case KEY_3_UP:
      edit_col++;
      if (item_id < 4)
      {
        if (edit_col > 5)
          edit_col = 0;
      }
      break;
    case KEY_4_UP:

      switch (item_id)
      {
      case 1:
        if(Set_Pass_Para(&pass))
				{
				   g_hard_state.iic_err_times++;				
				}
				else
				{
        for (j = 0; j < 6; j++)
          g_protect_password.pass_code[j] = pass.pass_code[j];
        state = 1;
        edit_col = 0;

				}
        break;
      case 2:

        if (FM31256_RTC_Write_Date(&date))
        {
					g_hard_state.iic_err_times++;			
        }
        else
        {
          system_date.year = date.year;
          system_date.month = date.month;
          system_date.day = date.day;
          state = 1;
          edit_col = 0;
        }

        break;
      case 3:
        if (FM31256_RTC_Write_Time(&time))
        {
						g_hard_state.iic_err_times++;		
        }
        else
        {
          system_time.hour = time.hour;
          system_time.minute = time.minute;
          system_time.second = time.second;
          state = 1;
          edit_col = 0;
        }

        break;
      case 4:
        if(Set_Alarm_Para(&var1))
	      {
						g_hard_state.iic_err_times++;		
        }	
				else
				{
        g_alarm_minutes.value = var1.value;
        state = 1;
        edit_col = 0;
				}
        break;
      case 5:
				
        if(Set_Contra_Para(&var2))
			   {
						g_hard_state.iic_err_times++;		
         }
				 else
				 {
          g_lcd_contrast.value = var2.value;
          state = 1;
        LCD_Darker(g_lcd_contrast.value);
        edit_col = 0;
				 }
        break;
      }
      break;
    case KEY_5_UP:
      state = 1;
      edit_col = 0;

      break;
    }
  }

  // ========== 显示部分 ==========

  // 主标题
  LCD_DisplayString(81, 0, g_systemset_menu_titles[0], 16, 0);

  LCD_DisplayString(0, 18, g_systemset_menu_titles[1], 16, ((item_id == 1) && (state == 1)) ? 1 : 0);

  for (j = 0; j < 6; j++)
    LCD_DisplayNum(81 + j * 12, 18, pass.pass_code[j], 16, 1, ((item_id == 1) && (state == 2) && (j == edit_col)) ? 0 : 0xff, 0, 1);

  LCD_DisplayString(0, 36, g_systemset_menu_titles[2], 16, ((item_id == 2) && (state == 1)) ? 1 : 0);
  LCD_DisplayNum(81, 36, date.year, 16, 2, ((item_id == 2) && (state == 2) && (edit_col < 2)) ? edit_col : 0xff, 0, 1);
  LCD_DisplayString(99, 36, "-", 16, 0);
  LCD_DisplayNum(108, 36, date.month, 16, 2, ((item_id == 2) && (state == 2) && ((edit_col < 4) && (edit_col > 1))) ? edit_col - 2 : 0xff, 0, 1);
  LCD_DisplayString(126, 36, "-", 16, 0);
  LCD_DisplayNum(135, 36, date.day, 16, 2, ((item_id == 2) && (state == 2) && ((edit_col < 6) && (edit_col > 3))) ? edit_col - 4 : 0xff, 0, 1); // 日期

  LCD_DisplayString(0, 54, g_systemset_menu_titles[3], 16, ((item_id == 3) && (state == 1)) ? 1 : 0);
  LCD_DisplayNum(81, 54, time.hour, 16, 2, ((item_id == 3) && (state == 2) && (edit_col < 2)) ? edit_col : 0xff, 0, 1);
  LCD_DisplayString(99, 54, ":", 16, 0);
  LCD_DisplayNum(108, 54, time.minute, 16, 2, ((item_id == 3) && (state == 2) && ((edit_col < 4) && (edit_col > 1))) ? edit_col - 2 : 0xff, 0, 1);
  LCD_DisplayString(126, 54, ":", 16, 0);
  LCD_DisplayNum(135, 54, time.second, 16, 2, ((item_id == 3) && (state == 2) && ((edit_col < 6) && (edit_col > 3))) ? edit_col - 4 : 0xff, 0, 1); // 时间

  LCD_DisplayString(0, 72, g_systemset_menu_titles[4], 16, ((item_id == 4) && (state == 1)) ? 1 : 0);
  if (var1.value > 0 && (var1.value < 61))
  {
    LCD_DisplayNum(81, 72, var1.value, 16, 2, 0xff, ((item_id == 4) && (state == 2)) ? 1 : 0, 1);
    LCD_DisplayString(99, 72, "min", 16, 0);
  }
  else if (var1.value == 0)
  {
    LCD_DisplayString(81, 72, "关闭", 16, ((item_id == 4) && (state == 2)) ? 1 : 0);
    LCD_DisplayString(117, 72, " ", 16, 0);
  }
  else if (var1.value == 61)
  {
    LCD_DisplayString(81, 72, "常开", 16, ((item_id == 4) && (state == 2)) ? 1 : 0);
    LCD_DisplayString(117, 72, " ", 16, 0);
  }

  LCD_DisplayString(0, 90, g_systemset_menu_titles[5], 16, ((item_id == 5) && (state == 1)) ? 1 : 0);
  LCD_DisplayNum(99, 90, var2.value, 16, 3, 0xff, ((item_id == 5) && (state == 2)) ? 1 : 0, 1);

  for (j = 0; j < 4; j++)
    LCD_DisplayString(72, y = y + 18, ":", 16, 0);

  LCD_DisplayString(90, y = y + 18, ":", 16, 0);
}

/*********电容调试设置菜单部分********/
void Cap_Adjust(u8 key_val)
{
  static u8 var1; // 临时变量
  u8 j, k,x,y;

  code const char *g_Cap_titles[4] = {

      "调试投电容",
      "调试投切",
      "关闭",
      "打开",

  };

  if (state == 0) // 第一次进入界面，系统变量赋值到临时变量,浏览模式，编辑坐标为0xff
  {
    var1 = g_adjust_cap.mode;
    switch (key_val)
    {

    case KEY_4_UP:
      state = 1;
      g_adjust_cap.mode = var1;
		  edit_col=0;
      break;

    case KEY_5_UP:
      state = 0;
      menu_state.id = MAIN_MENU;
      LCD_Clear();
      menu_state.switch_temp_flag = 1;
      return;
      break;
    }
  }
  else if (state == 1) // 进入调试电容
  {

    switch (key_val)
    {
    case KEY_0_UP:
    case KEY_1_UP:
      if (var1 == 0)
        var1 = 1;
      else
        var1 = 0;
      break;
    case KEY_4_UP: // 确认键进入调试

		  
      if(Set_AutoManual_Para(&g_adjust_cap))
			{
				g_hard_state.iic_err_times++;				
			}
			else
       g_adjust_cap.mode = var1;				
      if (g_adjust_cap.mode)
      {
        LCD_Clear();
        state = 2;
      }
      else
        state = 0;
      for (j = 0; j < g_cap_num.cap_num; j++)
      {
        if (g_cap[j].onf)
        {
          edit_col = j+1; // 箭头位置 1,2,3,4
          break;
        }
      }
      break;

    case KEY_5_UP:
      state = 0;
      break;
    }
  }
  else if (state == 2) // 调试电容界面
  {
    switch (key_val)
    {
    case KEY_3_UP: // 移位箭头，可跳过没有的电容

      edit_col++;
      if (edit_col > g_cap_num.cap_num)
        edit_col = 1;
      for (j = edit_col; j < g_cap_num.cap_num+1; j++)
      {
        if (g_cap[j-1].onf)
        {
          edit_col = j;
          break;
        }
      }
      break;
    case KEY_2_UP:
      edit_col--;
      if (edit_col < 1)
        edit_col = g_cap_num.cap_num;
      for (j = edit_col; j > 0; j--)
      {
        if (g_cap[j-1].onf)
        {
          edit_col = j;
          break;
        }
      }
      break;
    case KEY_4_UP:

      state = 3;
      break;

    case KEY_5_UP:
      state = 1;
      LCD_Clear();
      break;
    }
  }
  else if (state == 3)
  {

    switch (key_val)
    {
    case KEY_0_UP:
    case KEY_1_UP: // 投切装换

      break;
    case KEY_4_UP:
      if (g_cap[edit_col-1].state)
        g_cap[edit_col-1].state = 0;
      else
        g_cap[edit_col-1].state = 1;
      state = 2;
      break;
    case KEY_5_UP:
      // 无操作
      break;
    }
  }

  // 显示部分

  if (state < 2)
  {
    LCD_DisplayString(81, 0, g_Cap_titles[0], 16, 0);

    LCD_DisplayString(0, 20, g_Cap_titles[1], 16, ((state == 0)) ? 1 : 0);
    LCD_DisplayString(72, 20, ":", 16, 0); // 冒号不反显
    LCD_DisplayString(81, 20, (var1 == 0) ? g_Cap_titles[2] : g_Cap_titles[3], 16, ((state == 1)) ? 1 : 0);
    LCD_DisplayString(129, 112, "时间", 16, 0);
    time_disp(system_date, system_time, 171, 112);

    if (g_adjust_cap.mode)
    {
      LCD_DisplayString(6, 112, "手动", 16, 0);
    }
    else
      LCD_DisplayString(6, 112, "自动", 16, 0);
    LCD_DisplayLine(0, 108, 240, 0);
  }
  else
  {

    LCD_DisplayString(20, 20, "调试投切", 16, 0);
    cap_disp(1);
		switch (g_cap_num.cap_num)
		{
		case 1:
			x = 111;
			y = 0;
			break;

		case 2:
			x = 21;
			y = 180;
			break;
		case 3:
			x = 21;
			y = 90;
			break;
		case 4:
			x = 21;
			y = 60;
			break;
		}
    for (k = 1; k < g_cap_num.cap_num+1; k++)
    {
      LCD_DisplayString(x, 88, (k == edit_col) ? "箭\xFD" : "  ", 16, 0);
			x=x+y;
    }
		switch (g_cap_num.cap_num)
		{
		case 1:
			x = 111;
			y = 0;
			break;

		case 2:
			x = 21;
			y = 180;
			break;
		case 3:
			x = 21;
			y = 90;
			break;
		case 4:
			x = 21;
			y = 60;
			break;
		}
    for (k = 1; k < g_cap_num.cap_num+1; k++)
    {

      if (g_cap[k-1].onf)
      {

        LCD_DisplayString(x, 108,
                          g_cap[k-1].state ? "切" : "投",
                          16,
                          ((state == 3) && (edit_col == k)) ? 1 : 0);
      }

      x = x + y;
    }
  }
}

/*********采样校准菜单********/
void Sampling_Adiust(uint8_t key_val)
{
  if (key_val == KEY_5_UP)
  {
    menu_state.id = MAIN_MENU;
    LCD_Clear();
    menu_state.switch_temp_flag = 1;
    return;
  }

  LCD_DisplayString(84, 0, "采样校准", 16, 0);

  LCD_DisplayString(0, 20, "版本:", 16, 0);
  LCD_DisplayString(48, 20, "默认", 16, 0);
}

/*********事件记录菜单********/

// 配置参数
#define EVENT_LOG_CNT 100    // 总事件数
#define TOTAL_PAGE_CNT 20    // 总页数
#define EVENT_PER_PAGE 5     // 每页显示5条
#define EVENT_STR_BUF_LEN 16 // 事件字符串缓冲区长度

// 显示状态变量
static uint8_t s_current_page = 1;  // 当前页（1~20）
static uint8_t s_selected_line = 1; // 当前选中行（1~5）
static uint8_t s_refresh_flag = 1;  // 显示刷新标志
static uint8_t s_latest_event_idx;  // 最新事件的FRAM索引（首次进入菜单读取）
static uint8_t s_page_first_seq;    // 当前页第一条记录的序号（如第1页=1，第2页=6）
static uint8_t s_first_enter = 1;
// 缓存当前页的5条事件记录（避免重复读FRAM）
static Event_Log_Struct s_page_event_cache[EVENT_PER_PAGE];
// 缓存当前页的5条格式化字符串（避免重复拼接）
static char s_page_str_cache[EVENT_PER_PAGE][EVENT_STR_BUF_LEN];

// 临时缓冲区（复用）
static char s_temp_buf[4];
static uint8_t s_cache_valid = 0; // 缓存有效标志（1=有效，0=需重新读取）

/*
*********************************************************************************************************
*   函 数 名: Event_Log_Get_Latest_Idx
*   功能说明: 首次进入菜单时，获取最新事件的FRAM索引（仅读1次）
*   返 回 值: 最新事件的FRAM索引（0~99）
*********************************************************************************************************
*/
static uint8_t Event_Log_Get_Latest_Idx(void)
{
  uint8_t current_idx = Get_Event_Index();
  // 最新事件索引：当前索引为0→最新是99，否则为当前索引-1
  return (current_idx == 0) ? (EVENT_LOG_CNT - 1) : (current_idx - 1);
}

/*
*********************************************************************************************************
*   函 数 名: Event_Log_Calc_Seq_To_Idx
*   功能说明: 根据事件序号（001~100）计算对应的FRAM索引（核心映射）
*   形    参: seq_num - 事件序号（1=最新，100=最旧）
*   返 回 值: FRAM索引（0~99）
*********************************************************************************************************
*/
static uint8_t Event_Log_Calc_Seq_To_Idx(uint8_t seq_num)
{
  if (seq_num < 1 || seq_num > EVENT_LOG_CNT)
    return 0;
  // 序号1→最新事件索引，序号递增→索引递减（循环）
  return (s_latest_event_idx - (seq_num - 1) + EVENT_LOG_CNT) % EVENT_LOG_CNT;
}

// ==================== 工具函数（保留）====================
static void Uint8_To_3Str(uint8_t num, char *buf)
{
  if (buf == NULL)
    return;
  buf[0] = (num / 100) + '0';
  buf[1] = (num % 100 / 10) + '0';
  buf[2] = (num % 10) + '0';
  buf[3] = '\0';
}

static void Uint8_To_2Str(uint8_t num, char *buf)
{
  if (buf == NULL)
    return;
  buf[0] = (num / 10) + '0';
  buf[1] = (num % 10) + '0';
  buf[2] = '\0';
}

static const char *Event_Type_To_String(Event_Type_E type)
{
  switch (type)
  {
  case EVENT_TYPE_NONE:
    return "            ";
  case EVENT_TYPE_OVER_CURRENT1:
    return "一路过\xFD流";
  case EVENT_TYPE_OVER_CURRENT2:
    return "二路过\xFD流";
  case EVENT_TYPE_OVER_CURRENT3:
    return "三路过\xFD流";
  case EVENT_TYPE_OVER_CURRENT4:
    return "四路过\xFD流";
  case EVENT_TYPE_OVER_CURRENT11:
    return "一路速断";
  case EVENT_TYPE_OVER_CURRENT22:
    return "二路速断";
  case EVENT_TYPE_OVER_CURRENT33:
    return "三路速断";
  case EVENT_TYPE_OVER_CURRENT44:
    return "四路速断";
  case EVENT_TYPE_OVER_VOL_ZERO1:
    return "一路零序";
  case EVENT_TYPE_OVER_VOL_ZERO2:
    return "二路零序";
  case EVENT_TYPE_OVER_VOL_ZERO3:
    return "三路零序";
  case EVENT_TYPE_OVER_VOL_ZERO4:
    return "四路零序";
  case EVENT_TYPE_OVER_VOL:
    return "系统过\xFD压";
  case EVENT_TYPE_UNDER_VOL:
    return "系统欠压";
  case EVENT_TYPE_IN_STOP1:
    return "一路拒投";
  case EVENT_TYPE_IN_STOP2:
    return "二路拒投";
  case EVENT_TYPE_IN_STOP3:
    return "三路拒投";
  case EVENT_TYPE_IN_STOP4:
    return "四路拒投";
  case EVENT_TYPE_QUIT_STOP1:
    return "一路拒切";
  case EVENT_TYPE_QUIT_STOP2:
    return "二路拒切";
  case EVENT_TYPE_QUIT_STOP3:
    return "三路拒切";
  case EVENT_TYPE_QUIT_STOP4:
    return "四路拒切";
  case EVENT_TYPE_ERR1:
    return "一路外部故障";
  case EVENT_TYPE_ERR2:
    return "二路外部故障";
  case EVENT_TYPE_ERR3:
    return "三路外部故障";
  case EVENT_TYPE_ERR4:
    return "四路外部故障";
  case EVENT_TYPE_POWER_OFF:
    return "前段总闸断电";
  case EVENT_TYPE_IN1:
    return "一路投入";
  case EVENT_TYPE_IN2:
    return "二路投入";
  case EVENT_TYPE_IN3:
    return "三路投入";
  case EVENT_TYPE_IN4:
    return "四路投入";
  case EVENT_TYPE_QUIT1:
    return "一路切除\xFD";
  case EVENT_TYPE_QUIT2:
    return "二路切除\xFD";
  case EVENT_TYPE_QUIT3:
    return "三路切除\xFD";
  case EVENT_TYPE_QUIT4:
    return "四路切除\xFD";
  default:
    return "      ";
  }
}

// ==================== 格式化函数（保留）====================
static void Event_Log_Format_String(Event_Log_Struct *log, uint8_t seq_num, char *buf)
{

  uint8_t buf_idx = 0;
  const char *type_str = Event_Type_To_String(log->event_type);
  if (log == NULL || buf == NULL)
    return;
  // 拼接3位序号
  Uint8_To_3Str(seq_num, s_temp_buf);
  buf[buf_idx++] = s_temp_buf[0];
  buf[buf_idx++] = s_temp_buf[1];
  buf[buf_idx++] = s_temp_buf[2];
  buf[buf_idx++] = ' ';

  // 拼接事件类型
  while (*type_str != '\0' && buf_idx < EVENT_STR_BUF_LEN - 1)
  {
    buf[buf_idx++] = *type_str++;
  }
  buf[buf_idx] = '\0';
}
/*
*********************************************************************************************************
*   函 数 名: Event_Log_Load_Page_Cache
*   功能说明: 加载当前页的5条记录到缓存（仅跨页/首次进入时调用）
*   返 回 值: 无
*********************************************************************************************************
*/
static void Event_Log_Load_Page_Cache(void)
{
  uint8_t fram_idx, seq_num, i;

  // 计算当前页第一条记录的序号（如第1页=1，第2页=6，第3页=11...）
  s_page_first_seq = (s_current_page - 1) * EVENT_PER_PAGE + 1;

  // 读取当前页的5条记录到缓存
  for (i = 0; i < EVENT_PER_PAGE; i++)
  {
    seq_num = s_page_first_seq + i;
    if (seq_num > EVENT_LOG_CNT)
    { // 边界保护（避免越界）
      memset(&s_page_event_cache[i], 0, sizeof(Event_Log_Struct));
      memset(s_page_str_cache[i], 0, EVENT_STR_BUF_LEN);
      continue;
    }

    // 计算FRAM索引并读取记录
    fram_idx = Event_Log_Calc_Seq_To_Idx(seq_num);
    FM31256_FRAM_Read(EVENT_LOG_ADDR(fram_idx), (uint8_t *)&s_page_event_cache[i], sizeof(Event_Log_Struct));

    // 格式化字符串并缓存
    Event_Log_Format_String(&s_page_event_cache[i], seq_num, s_page_str_cache[i]);
  }

  s_cache_valid = 1;  // 缓存有效
  s_refresh_flag = 1; // 标记需要刷新显示
}
// ==================== 分页信息显示（保留）====================
static void Event_Log_Display_Page_Info(void)
{
  char page_buf[8];
  uint8_t idx = 0;

  Uint8_To_2Str(s_current_page, s_temp_buf);
  page_buf[idx++] = s_temp_buf[0];
  page_buf[idx++] = s_temp_buf[1];
  page_buf[idx++] = '/';

  Uint8_To_2Str(TOTAL_PAGE_CNT, s_temp_buf);
  page_buf[idx++] = s_temp_buf[0];
  page_buf[idx++] = s_temp_buf[1];
  page_buf[idx] = '\0';

  LCD_DisplayString(195, 0, page_buf, 16, 0);
}

/*
*********************************************************************************************************
*   函 数 名: Event_Log_Refresh_Line
*   功能说明: 仅刷新指定行的显示（反显/正常），不重绘整页（核心优化）
*   形    参: line - 行号（1~5）；highlight - 1=反显，0=正常
*********************************************************************************************************
*/
static void Event_Log_Refresh_Line(uint8_t line, uint8_t highlight)
{

  uint8_t line_idx = line - 1;
  uint8_t line_y = 20 + line_idx * 16; // 行Y坐标
  if (line < 1 || line > EVENT_PER_PAGE)
    return;
  // 显示事件字符串
  LCD_DisplayString(0, line_y, s_page_str_cache[line_idx], 16, highlight);

  // 显示时间（仅事件有效时）
  if (s_page_event_cache[line_idx].event_type != EVENT_TYPE_NONE)
  {
    disp_two_digit(s_page_event_cache[line_idx].year, 180, line_y, 8, 0);
    LCD_DisplayChar(198, line_y, "/", 8, 0);
    disp_two_digit(s_page_event_cache[line_idx].month, 207, line_y, 8, 0);
    LCD_DisplayChar(225, line_y, "/", 8, 0);
    disp_two_digit(s_page_event_cache[line_idx].day, 234, line_y, 8, 0);

    disp_two_digit(s_page_event_cache[line_idx].hour, 180, line_y + 8, 8, 0);
    LCD_DisplayChar(198, line_y + 8, ":", 8, 0);
    disp_two_digit(s_page_event_cache[line_idx].minute, 207, line_y + 8, 8, 0);
    LCD_DisplayChar(225, line_y + 8, ":", 8, 0);
    disp_two_digit(s_page_event_cache[line_idx].second, 234, line_y + 8, 8, 0);
  }
  else
  {
    LCD_DisplayString(180, line_y, "        ", 8, 0);
    LCD_DisplayString(180, line_y + 8, "        ", 8, 0);
  }
}

/*
*********************************************************************************************************
*   函 数 名: Event_Log_Display_Page
*   功能说明: 整页刷新（仅首次/跨页时调用）
*********************************************************************************************************
*/
static void Event_Log_Display_Page(void)
{
  uint8_t i;
  if (!s_cache_valid)
    return; // 缓存无效时不显示

  // 1. 清屏+显示标题+分页信息
  LCD_DisplayString(81, 0, "事件记录", 16, 0);
  Event_Log_Display_Page_Info();

  // 2. 显示当前页所有行（仅首次/跨页时）
  for (i = 0; i < EVENT_PER_PAGE; i++)
  {
    uint8_t highlight = (i + 1 == s_selected_line) ? 1 : 0;
    Event_Log_Refresh_Line(i + 1, highlight);
  }

  s_refresh_flag = 0;
}
/*
*********************************************************************************************************
*   函 数 名: Event_Log_Init
*   功能说明: 首次进入事件菜单初始化（仅调用1次）
*********************************************************************************************************
*/
static void Event_Log_Init(void)
{
  // 1. 重置状态
  s_current_page = 1;
  s_selected_line = 1;
  s_cache_valid = 0;

  // 2. 获取最新事件索引（仅首次进入时读1次）
  s_latest_event_idx = Event_Log_Get_Latest_Idx();

  // 3. 加载第一页（最新5条）缓存
  Event_Log_Load_Page_Cache();

  // 4. 首次整页显示
  Event_Log_Display_Page();
}
/*
*********************************************************************************************************
*   函 数 名: Event_Log_Handle_Key
*   功能说明: 按键处理（核心重构：仅跨页时加载缓存）
*********************************************************************************************************
*/
static void Event_Log_Handle_Key(uint8_t key_val)
{
  uint8_t need_refresh_line = 0;               // 是否需要刷新行反显
  uint8_t need_reload_cache = 0;               // 是否需要重新加载缓存（跨页）
  uint8_t old_selected_line = s_selected_line; // 记录旧选中行

  switch (key_val)
  {
  case KEY_0_UP: // 上键：001上键→100（最后一页最后一行）
    if (s_selected_line > 1)
    {
      // 非第一行：仅切换行，不跨页
      s_selected_line--;
      need_refresh_line = 1;
    }
    else
    {
      // 第一行：判断是否是第一页
      if (s_current_page > 1)
      {
        s_current_page--;
        s_selected_line = EVENT_PER_PAGE;
        need_reload_cache = 1; // 跨页→重新加载缓存
      }
      else
      {
        // 第一页第一行（序号001）：跳转到最后一页最后一行（序号100）
        s_current_page = TOTAL_PAGE_CNT;  // 最后一页（20页）
        s_selected_line = EVENT_PER_PAGE; // 最后一行（5行）
        need_reload_cache = 1;            // 跨页→重新加载缓存
      }
    }
    break;

  case KEY_1_UP: // 下键：100下键→001（第一页第一行）
    if (s_selected_line < EVENT_PER_PAGE)
    {
      // 非最后一行：仅切换行，不跨页
      s_selected_line++;
      need_refresh_line = 1;
    }
    else
    {
      // 最后一行：判断是否是最后一页
      if (s_current_page < TOTAL_PAGE_CNT)
      {
        s_current_page++;
        s_selected_line = 1;
        need_reload_cache = 1; // 跨页→重新加载缓存
      }
      else
      {
        // 最后一页最后一行（序号100）：跳转到第一页第一行（序号001）
        s_current_page = 1;    // 第一页
        s_selected_line = 1;   // 第一行
        need_reload_cache = 1; // 跨页→重新加载缓存
      }
    }
    break;

  case KEY_2_UP: // 左键（上一页）：第一页左键→最后一页
    if (s_current_page > 1)
    {
      s_current_page--;
      need_reload_cache = 1; // 跨页→重新加载缓存
    }
    else
    {
      // 第一页：跳转到最后一页
      s_current_page = TOTAL_PAGE_CNT;
      need_reload_cache = 1;
    }
    break;

  case KEY_3_UP: // 右键（下一页）：最后一页右键→第一页
    if (s_current_page < TOTAL_PAGE_CNT)
    {
      s_current_page++;
      need_reload_cache = 1; // 跨页→重新加载缓存
    }
    else
    {
      // 最后一页：跳转到第一页
      s_current_page = 1;
      need_reload_cache = 1;
    }
    break;

  case KEY_5_UP: // 返回主菜单
    menu_state.id = MAIN_MENU;
    s_cache_valid = 0; // 重置缓存标志
    s_first_enter = 1; // 下次进入重新初始化
    LCD_Clear();
    menu_state.switch_temp_flag = 1;
    return; // 直接返回，不处理后续

  default:
    return;
  }

  // 1. 跨页：重新加载缓存+整页刷新
  if (need_reload_cache)
  {
    Event_Log_Load_Page_Cache();
    Event_Log_Display_Page();
  }
  // 2. 仅行切换：刷新旧行（正常显示）+ 新行（反显）
  else if (need_refresh_line)
  {
    Event_Log_Refresh_Line(old_selected_line, 0); // 旧行取消反显
    Event_Log_Refresh_Line(s_selected_line, 1);   // 新行反显
  }
}

/*
*********************************************************************************************************
*   函 数 名: Event_Log
*   功能说明: 事件菜单主入口（最终版）
*********************************************************************************************************
*/
void Event_Log(uint8_t key_val)
{
  // 首次进入菜单：初始化（仅执行1次）

  if (s_first_enter)
  {
    Event_Log_Init();
    s_first_enter = 0;
    return;
  }

  // 非首次：处理按键
  if (key_val != KEY_NONE)
  { // KEY_NONE需定义为0，根据你的按键驱动调整
    Event_Log_Handle_Key(key_val);
  }
}

/*********数据统计菜单********/
void Data_Stat(uint8_t key_val)
{

  code const char *g_data_stat_titles[6] = {

      "数\xFD据统计",
      "日投次数\xFD:",
      "月投次数\xFD:",
      "日切次数\xFD:",
      "月切次数\xFD:",
      "次",
  };

  LCD_DisplayString(84, 0, g_data_stat_titles[0], 16, 0);
  LCD_DisplayString(0, 20, g_data_stat_titles[1], 16, 0);
  LCD_DisplayString(0, 38, g_data_stat_titles[2], 16, 0);
  LCD_DisplayString(0, 56, g_data_stat_titles[3], 16, 0);
  LCD_DisplayString(0, 74, g_data_stat_titles[4], 16, 0);

  LCD_DisplayNum(81, 20, g_stat.day_times_on, 16, 5, 0xff, 0, 1);
  LCD_DisplayNum(81, 38, g_stat.day_times_on, 16, 5, 0xff, 0, 1);
  LCD_DisplayNum(81, 56, g_stat.day_times_on, 16, 5, 0xff, 0, 1);
  LCD_DisplayNum(81, 74, g_stat.day_times_on, 16, 5, 0xff, 0, 1);

  LCD_DisplayString(132, 20, g_data_stat_titles[5], 16, 0);
  LCD_DisplayString(132, 38, g_data_stat_titles[5], 16, 0);
  LCD_DisplayString(132, 56, g_data_stat_titles[5], 16, 0);
  LCD_DisplayString(132, 74, g_data_stat_titles[5], 16, 0);

  if (key_val == KEY_5_UP)
  {

    menu_state.id = MAIN_MENU;
    LCD_Clear();
    menu_state.switch_temp_flag = 1;
    return;
  }
}

/*********系统参数设置菜单部分********/

void Sys_Para_Set(u8 key_val)
{
  static Ratio_Para_Struct var, sys_pt_ct;
  static Cap_Ratio_Para_Struct var1, var2, var3, var4, cap_ratio[4];

  code const char *g_systemset_para_titles[9] = {
      "系统参数\xFD", // 4字符=7字节 + \0=1 → 8字节
      "系统PT",       // 4字符=5字节 + \0=1 → 6字节
      "系统CT",       // 4字符=5字节 + \0=1 → 6字节
      "一路CT",       // 4字符=5字节 + \0=1 → 6字节
      "二路CT",       // 4字符=5字节 + \0=1 → 6字节
      "三\xFD路CT",   // 4字符=5字节 + \0=1 → 6字节
      "四路CT",       // 4字符=5字节 + \0=1 → 6字节
      "保存",         // 2字=4字节 + \0=1 → 5字节
      "退出",         // 2字=4字节 + \0=1 → 5字节

  };
  if (state == 0) // 第一次进入菜单，系统变量赋值到临时变量。初始化坐标，浏览模式，编辑坐标为0xff
  {
    sys_pt_ct = g_sys_pt_ct;
    cap_ratio[0] = g_cap_ratio[0];
    cap_ratio[1] = g_cap_ratio[1];
    cap_ratio[2] = g_cap_ratio[2];
    cap_ratio[3] = g_cap_ratio[3];
    var = g_sys_pt_ct;
    var1 = g_cap_ratio[0];
    var2 = g_cap_ratio[1];
    var3 = g_cap_ratio[2];
    var4 = g_cap_ratio[3];
    edit_col = 0;
    state = 1;
    item_id = 1;
  }
  else if (state == 1)
  {
    // 浏览模式
    switch (key_val)
    {
     case KEY_0_UP:
		  if(g_cap_num.pro_num==0)
			{
          if(item_id==1)
             item_id=7;
          else if(item_id==7)
             item_id=1;		
          if(item_id==2)
             item_id=8;
          else if(item_id==8)
             item_id=2;						
			}			 		 
		  if(g_cap_num.pro_num==1)
			{
          if(item_id==1)
             item_id=7;
          else if(item_id==7)
             item_id=3;	
          else if(item_id==3)
             item_id=1;	
          if(item_id==2)
             item_id=8;
          else if(item_id==8)
             item_id=2;					
			}
		  if(g_cap_num.pro_num==2)
			{
          if(item_id==1)
             item_id=7;
          else if(item_id==7)
             item_id=3;	
          else if(item_id==3)
             item_id=1;	
          if(item_id==2)
             item_id=8;
          else if(item_id==8)
             item_id=4;	
          else if(item_id==4)
             item_id=2;						
			}		
		  if(g_cap_num.pro_num==3)
			{
          if(item_id==1)
             item_id=7;
          else if(item_id==7)
             item_id=5;	
          else if(item_id==5)
             item_id=3;	
          else if(item_id==3)
             item_id=1;						
					
          if(item_id==2)
             item_id=8;
          else if(item_id==8)
             item_id=4;	
          else if(item_id==4)
             item_id=2;				
			}		
		  if(g_cap_num.pro_num==4)
			{
          if(item_id==1)
             item_id=7;
          else if(item_id==7)
             item_id=5;	
          else if(item_id==5)
             item_id=3;	
          else if(item_id==3)
             item_id=1;						
					
          if(item_id==2)
             item_id=8;
          else if(item_id==8)
             item_id=6;	
          else if(item_id==6)
             item_id=4;	
          else if(item_id==4)
             item_id=2;						
			}		
      break;
    case KEY_1_UP:			
		  if(g_cap_num.pro_num==0)
			{
          if(item_id==1)
             item_id=7;
          else if(item_id==7)
             item_id=1;		
          if(item_id==2)
             item_id=8;
          else if(item_id==8)
             item_id=2;						
			}		
		  if(g_cap_num.pro_num==1)
			{
          if(item_id==1)
             item_id=3;
          else if(item_id==3)
             item_id=7;	
          else if(item_id==7)
             item_id=1;	

          if(item_id==2)
             item_id=8;
          else if(item_id==8)
             item_id=2;					
			}
		  if(g_cap_num.pro_num==2)
			{
          if(item_id==1)
             item_id=3;
          else if(item_id==3)
             item_id=7;	
          else if(item_id==7)
             item_id=1;	

          if(item_id==2)
             item_id=4;
          else if(item_id==4)
             item_id=8;
          else if(item_id==8)
             item_id=2;					
			}		
		  if(g_cap_num.pro_num==3)
			{
          if(item_id==1)
             item_id=3;
          else if(item_id==3)
             item_id=5;	
          else if(item_id==5)
             item_id=7;	
          else if(item_id==7)
             item_id=1;	
          if(item_id==2)
             item_id=4;
          else if(item_id==4)
             item_id=8;
          else if(item_id==8)
             item_id=2;						
			}		
		  if(g_cap_num.pro_num==4)
			{
          if(item_id==1)
             item_id=3;
          else if(item_id==3)
             item_id=5;	
          else if(item_id==5)
             item_id=7;	
          else if(item_id==7)
             item_id=1;	
          if(item_id==2)
             item_id=4;
          else if(item_id==4)
             item_id=6;
          else if(item_id==6)
             item_id=8;
          else if(item_id==8)
             item_id=2;				
			}				
      break;

    case KEY_2_UP:
				if(item_id==7)
					item_id=8;
				else if(item_id==8)
					item_id=7;
        else 
				{		

				if (item_id<=(g_cap_num.pro_num + 2))
				item_id--;
        if(item_id<1)	
         item_id=(g_cap_num.pro_num + 2);	
			}

      break;
    case KEY_3_UP:
				if(item_id==7)
					item_id=8;
				else if(item_id==8)
					item_id=7;
        else 
				{				
				if (item_id>=1)
				item_id++;
         if(item_id>(g_cap_num.pro_num + 2))	
         item_id=1;	
			}

      break;
    case KEY_4_UP:
      if (item_id <= (g_cap_num.pro_num + 2))
      {
        state = 2; // 进入编辑
        edit_col = 0;
      }
      else if (item_id == 7)
      {
        if(Set_Ratio_Para(&sys_pt_ct)||Set_Cap_Ratio_Para_Ch(0,&var1)||Set_Cap_Ratio_Para_Ch(0,&var2)||Set_Cap_Ratio_Para_Ch(0,&var3)||Set_Cap_Ratio_Para_Ch(0,&var4))
				{
				    g_hard_state.iic_err_times++;
				}
				else
				{
						g_sys_pt_ct = sys_pt_ct;
            g_cap_ratio[0]=cap_ratio[0];
            g_cap_ratio[1]=cap_ratio[1];					
            g_cap_ratio[2]=cap_ratio[2];
            g_cap_ratio[3]=cap_ratio[3];					
						state = 0; // 保存
						menu_state.id = PARAM_SETTING;
						LCD_Clear();
						menu_state.switch_temp_flag = 1;
					    return;
				}
      }
      else if (item_id == 8)
      {
        state = 0; // 退出（不保存）
        menu_state.id = PARAM_SETTING;
        LCD_Clear();
        menu_state.switch_temp_flag = 1;
        return;
      }
      break;

    case KEY_5_UP:
      state = 0; // 退出（不保存）
      menu_state.id = PARAM_SETTING;
      LCD_Clear();
      menu_state.switch_temp_flag = 1;
      return;
      break;
    }

    var = sys_pt_ct;

    var1 = cap_ratio[0];
    var2 = cap_ratio[1];
    var3 = cap_ratio[2];
    var4 = cap_ratio[3];
  }
  else if (state == 2) // 参数编辑
  {

    switch (key_val)
    {
    case KEY_0_UP: // 加
      if (item_id == 1)
      {

        adjust_digit_u16(&var.pt_ratio, edit_col, 1, 3);
      }
      if (item_id == 2)
      {
        if (edit_col < 4)
        {
          adjust_digit_u16(&var.ct_ratio, edit_col, 1, 4);
        }
        else
        {
          if (var.ct_ratio1 == 1)
            var.ct_ratio1 = 5;
          else
            var.ct_ratio1 = 1;
        }
      }
      if (item_id == 3)
      {
        if (edit_col < 4)
        {
          adjust_digit_u16(&var1.ct_ratio, edit_col, 1, 4);
        }
        else
        {
          if (var1.ct_ratio1 == 1)
            var1.ct_ratio1 = 5;
          else
            var1.ct_ratio1 = 1;
        }
      }
      if (item_id == 4)
      {
        if (edit_col < 4)
        {
          adjust_digit_u16(&var2.ct_ratio, edit_col, 1, 4);
        }
        else
        {
          if (var2.ct_ratio1 == 1)
            var2.ct_ratio1 = 5;
          else
            var2.ct_ratio1 = 1;
        }
      }
      if (item_id == 5)
      {
        if (edit_col < 4)
        {
          adjust_digit_u16(&var3.ct_ratio, edit_col, 1, 4);
        }
        else
        {
          if (var3.ct_ratio1 == 1)
            var3.ct_ratio1 = 5;
          else
            var3.ct_ratio1 = 1;
        }
      }
      if (item_id == 6)
      {
        if (edit_col < 4)
        {
          adjust_digit_u16(&var4.ct_ratio, edit_col, 1, 4);
        }
        else
        {
          if (var4.ct_ratio1 == 1)
            var4.ct_ratio1 = 5;
          else
            var4.ct_ratio1 = 1;
        }
      }

      break;
    case KEY_1_UP: // 减
      if (item_id == 1)
      {
        adjust_digit_u16(&var.pt_ratio, edit_col, -1, 3);
      }
      if (item_id == 2)
      {
        if (edit_col < 4)
        {
          adjust_digit_u16(&var.ct_ratio, edit_col, -1, 4);
        }
        else
        {
          if (var.ct_ratio1 == 1)
            var.ct_ratio1 = 5;
          else
            var.ct_ratio1 = 1;
        }
      }
      if (item_id == 3)
      {
        if (edit_col < 4)
        {
          adjust_digit_u16(&var1.ct_ratio, edit_col, -1, 4);
        }
        else
        {
          if (var1.ct_ratio1 == 1)
            var1.ct_ratio1 = 5;
          else
            var1.ct_ratio1 = 1;
        }
      }
      if (item_id == 4)
      {
        if (edit_col < 4)
        {
          adjust_digit_u16(&var2.ct_ratio, edit_col, -1, 4);
        }
        else
        {
          if (var2.ct_ratio1 == 1)
            var2.ct_ratio1 = 5;
          else
            var2.ct_ratio1 = 1;
        }
      }
      if (item_id == 5)
      {
        if (edit_col < 4)
        {
          adjust_digit_u16(&var3.ct_ratio, edit_col, -1, 4);
        }
        else
        {
          if (var3.ct_ratio1 == 1)
            var3.ct_ratio1 = 5;
          else
            var3.ct_ratio1 = 1;
        }
      }
      if (item_id == 6)
      {
        if (edit_col < 4)
        {
          adjust_digit_u16(&var4.ct_ratio, edit_col, -1, 4);
        }
        else
        {
          if (var4.ct_ratio1 == 1)
            var4.ct_ratio1 = 5;
          else
            var4.ct_ratio1 = 1;
        }
      }

      break;
    case KEY_2_UP:
      edit_col--;
      if (item_id == 1)
      {
        if (edit_col < 0)
          edit_col = 2;
      }
      if ((item_id >= 2) && (item_id < 7))
      {
        if (edit_col < 0)
        {
          edit_col = 4;
        }
      }

      break;
    case KEY_3_UP:
      edit_col++;
      if (item_id == 1)
      {
        if (edit_col > 2)
          edit_col = 0;
      }
      if ((item_id >= 2) && (item_id < 7))
      {
        if (edit_col > 4)
        {
          edit_col = 0;
        }
      }
      break;
    case KEY_4_UP:

      if (item_id == 1)
      {
        sys_pt_ct.pt_ratio = var.pt_ratio;
      }
      if (item_id == 2)
      {
        sys_pt_ct.ct_ratio = var.ct_ratio;
        sys_pt_ct.ct_ratio1 = var.ct_ratio1;
      }
      if (item_id == 3)
      {
        cap_ratio[0].ct_ratio = var1.ct_ratio;
        cap_ratio[0].ct_ratio1 = var1.ct_ratio1;
      }
      if (item_id == 4)
      {
        cap_ratio[1].ct_ratio = var2.ct_ratio;
        cap_ratio[1].ct_ratio1 = var2.ct_ratio1;
      }
      if (item_id == 5)
      {
        cap_ratio[2].ct_ratio = var3.ct_ratio;
        cap_ratio[2].ct_ratio1 = var3.ct_ratio1;
      }
      if (item_id == 6)
      {
        cap_ratio[3].ct_ratio = var4.ct_ratio;
        cap_ratio[3].ct_ratio1 = var4.ct_ratio1;
      }

      if (item_id <= 6)
      {
        state = 1;
      }
      edit_col = 0xff;
      break;
    case KEY_5_UP:

      if (item_id == 1)
      {
        var.pt_ratio=sys_pt_ct.pt_ratio ;
      }
      if (item_id == 2)
      {
         var.ct_ratio=sys_pt_ct.ct_ratio ;
         var.ct_ratio1=sys_pt_ct.ct_ratio1 ;
      }
      if (item_id == 3)
      {
        var1.ct_ratio=cap_ratio[0].ct_ratio  ;
        var1.ct_ratio1=cap_ratio[0].ct_ratio1 ;
      }
      if (item_id == 4)
      {
        var2.ct_ratio=cap_ratio[1].ct_ratio;
        var2.ct_ratio1=cap_ratio[1].ct_ratio1  ;
      }
      if (item_id == 5)
      {
        var3.ct_ratio=cap_ratio[2].ct_ratio  ;
        var3.ct_ratio1=cap_ratio[2].ct_ratio1 ;
      }
      if (item_id == 6)
      {
        var4.ct_ratio=cap_ratio[3].ct_ratio ;
        var4.ct_ratio1=cap_ratio[3].ct_ratio1 ;
      }			
		
		
      if (item_id <= 6)
      {
        state = 1;
      }
      edit_col = 0xff;
      break;
    }
  }

  // 显示部分

  LCD_DisplayString(LCD_MID, 0, g_systemset_para_titles[0], 16, 0);

  LCD_DisplayChinese(0, 20, g_systemset_para_titles[1], 16, ((item_id == 1) && (state == 1)) ? 1 : 0);
  LCD_DisplayString(48, 20, "#", 16, 0); // 冒号不反显
  LCD_DisplayNum(54, 20, var.pt_ratio, 16, 3, ((item_id == 1) && (state == 2)) ? edit_col : 0xff, 0, 1);

  LCD_DisplayChinese(123, 20, g_systemset_para_titles[2], 16, ((item_id == 2) && (state == 1)) ? 1 : 0);
  LCD_DisplayString(171, 20, ":", 16, 0); // 冒号不反显

  LCD_DisplayNum(177, 20, var.ct_ratio, 16, 4, ((item_id == 2) && (edit_col < 4) && (state == 2)) ? edit_col : 0xff, 0, 1);

  LCD_DisplayString(213, 20, "/", 16, 0); // 冒号不反显

  LCD_DisplayNum(222, 20, var.ct_ratio1, 16, 1, ((item_id == 2) && (edit_col == 4) && (state == 2)) ? 0 : 0xff, 0, 1);
  LCD_DisplayString(231, 20, "A", 16, 0); // 冒号不反显

  if (g_cap_num.pro_num > 0)
  {
    LCD_DisplayChinese(0, 40, g_systemset_para_titles[3], 16, ((item_id == 3) && (state == 1)) ? 1 : 0);
    LCD_DisplayString(48, 40, "#", 16, 0); // 冒号不反显
    LCD_DisplayNum(54, 40, var1.ct_ratio, 16, 4, ((item_id == 3) && (edit_col < 4) && (state == 2)) ? edit_col : 0xff, 0, 1);

    LCD_DisplayString(90, 40, "/", 16, 0); // 冒号不反显

    LCD_DisplayNum(99, 40, var1.ct_ratio1, 16, 1, ((item_id == 3) && (edit_col == 4) && (state == 2)) ? 0 : 0xff, 0, 1);
    LCD_DisplayString(108, 40, "A", 16, 0); // 冒号不反显
  }
  if (g_cap_num.pro_num > 1)
  {
    LCD_DisplayChinese(123, 40, g_systemset_para_titles[4], 16, ((item_id == 4) && (state == 1)) ? 1 : 0);
    LCD_DisplayString(171, 40, ":", 16, 0); // 冒号不反显
    LCD_DisplayNum(177, 40, var2.ct_ratio, 16, 4, ((item_id == 4) && (edit_col < 4) && (state == 2)) ? edit_col : 0xff, 0, 1);

    LCD_DisplayString(213, 40, "/", 16, 0); // 冒号不反显

    LCD_DisplayNum(222, 40, var2.ct_ratio1, 16, 1, ((item_id == 4) && (edit_col == 4) && (state == 2)) ? 0 : 0xff, 0, 1);
    LCD_DisplayString(231, 40, "A", 16, 0); // 冒号不反显
  }
  if (g_cap_num.pro_num > 2)
  {
    LCD_DisplayChinese(0, 60, g_systemset_para_titles[5], 16, ((item_id == 5) && (state == 1)) ? 1 : 0);
    LCD_DisplayString(48, 60, ":", 16, 0); // 冒号不反显
    LCD_DisplayNum(54, 60, var3.ct_ratio, 16, 4, ((item_id == 5) && (edit_col < 4) && (state == 2)) ? edit_col : 0xff, 0, 1);

    LCD_DisplayString(90, 60, "/", 16, 0); // 冒号不反显

    LCD_DisplayNum(99, 60, var3.ct_ratio1, 16, 1, ((item_id == 5) && (edit_col == 4) && (state == 2)) ? 0 : 0xff, 0, 1);
    LCD_DisplayString(108, 60, "A", 16, 0); // 冒号不反显
  }
  if (g_cap_num.pro_num > 3)
  {
    LCD_DisplayChinese(123, 60, g_systemset_para_titles[6], 16, ((item_id == 6) && (state == 1)) ? 1 : 0);
    LCD_DisplayString(171, 60, ":", 16, 0); // 冒号不反显
    LCD_DisplayNum(177, 60, var4.ct_ratio, 16, 4, ((item_id == 6) && (edit_col < 4) && (state == 2)) ? edit_col : 0xff, 0, 1);

    LCD_DisplayString(213, 60, "/", 16, 0); // 冒号不反显

    LCD_DisplayNum(222, 60, var4.ct_ratio1, 16, 1, ((item_id == 6) && (edit_col == 4) && (state == 2)) ? 0 : 0xff, 0, 1);
    LCD_DisplayString(231, 60, "A", 16, 0); // 冒号不反显
  }

  LCD_DisplayString(0, 112, g_systemset_para_titles[7], 16, ((item_id == 7) && (state == 1)) ? 1 : 0);
  LCD_DisplayString(204, 112, g_systemset_para_titles[8], 16, ((item_id == 8) && (state == 1)) ? 1 : 0);
}

/*********电容参数设置菜单部分********/
void Cap_Para_Set(u8 key_val)
{            // 标题起始列
  u8 y=20 ; // 行间隔
  u8 j=1;
	u8 flag=0;
  static Cap_Para_Struct cap[4], var[4];
  code const char *g_cap_para_titles[9] = {

      "电容参数\xFD",
      "C1",
      "C2",
      "C3",
      "C4",
      "保存",
      "退出",
      "投入",
      "退出",
  };

  if (state == 0) // 第一次进入菜单，系统变量赋值到临时变量。初始化坐标，浏览模式，编辑坐标为0xff
  {

    cap[0] = g_cap[0];
    cap[1] = g_cap[1];
    cap[2] = g_cap[2];
    cap[3] = g_cap[3];
    var[0] = g_cap[0];
    var[1] = g_cap[1];
    var[2] = g_cap[2];
    var[3] = g_cap[3];
    edit_col = 0;
    state = 1;
    item_id = 1;
  }
  else if (state == 1)
  {
    // 浏览模式
    switch (key_val)
    {
    case KEY_0_UP:
			if(item_id<6)
			{
      item_id--;
      if (item_id < 1)
        item_id = 5;
			else if(item_id >g_cap_num.cap_num)
				item_id=g_cap_num.cap_num;
		}	
      break;
    case KEY_1_UP:
      if(item_id<6)
			{
      item_id++;
      if ((item_id > (g_cap_num.cap_num))&&(item_id<6))
        item_id = 5;
			else if(item_id==6)
				item_id=1;
		}
      break;
    case KEY_2_UP:
    case KEY_3_UP:
      if (item_id == 5)
				item_id=6;
      else if (item_id == 6)
				item_id=5;
      break;

    case KEY_4_UP:

      if (item_id <= 4)
      {
        state = 2;
      }
      else if (item_id == 5)
      {
        
				if((Cap_CheckAllEnabledValueEqual(&cap,g_cap_num.cap_num)==0)&&(g_control_para.type==4))
				   flag=1;
				if(flag==0)
				{
				for(j=0;j<g_cap_num.cap_num;j++)
				{
				  if(Set_Cap_Para_Ch(j, &cap[j]))
					{
							g_hard_state.iic_err_times++;
						  break;
					}
							
				}
				 if(j==g_cap_num.cap_num)
				{
	        g_cap[0] = cap[0];
          g_cap[1] = cap[1];
					g_cap[2] = cap[2];
					g_cap[3] = cap[3];
					LCD_Clear();
					menu_state.switch_temp_flag = 1;				
					state = 0; // 保存
					menu_state.id = PARAM_SETTING;	
        return;				
				}
			}
      }
      else if (item_id == 6)
      {
        state = 0; // 退出（不保存）
        menu_state.id = PARAM_SETTING;
        LCD_Clear();
        menu_state.switch_temp_flag = 1;
        return;
      }
      break;

    case KEY_5_UP:
      state = 0; // 退出（不保存）
      menu_state.id = PARAM_SETTING;
      LCD_Clear();
      menu_state.switch_temp_flag = 1;
      return;
      break;
    }
    edit_col = 0;

    var[0] = cap[0];
    var[1] = cap[1];
    var[2] = cap[2];
    var[3] = cap[3];
  }
  else if (state == 2) // 参数编辑
  {
    switch (key_val)
			
		{

    case KEY_0_UP: // 加

    case KEY_1_UP: // 减
      if (item_id <= g_cap_num.cap_num)
      {
        if (edit_col < 4)
          adjust_digit_u16(&var[item_id - 1].value, edit_col, key_val == KEY_0_UP ? 1 : -1, 4);
        else
        {
          if (var[item_id - 1].onf == 0)
            var[item_id - 1].onf = 1;
          else
            var[item_id - 1].onf = 0;
        }
      }
    break;
  case KEY_2_UP:
    edit_col--;
    if (edit_col < 0)
    {
      edit_col = 4;
    }

    break;
  case KEY_3_UP:
    edit_col++;

    if (edit_col > 4)
    {
      edit_col = 0;
    }

    break;
  case KEY_4_UP:

    switch (item_id)
    {
    case 1:
      cap[0].value = var[0].value;
      cap[0].onf = var[0].onf;
      break;
    case 2:
      cap[1].value = var[1].value;
      cap[1].onf = var[1].onf;
      break;
    case 3:
      cap[2].value = var[2].value;
      cap[2].onf = var[2].onf;
      break;
    case 4:
      cap[3].value = var[3].value;
      cap[3].onf = var[3].onf;
      break;
    }
    state = 1;
    edit_col = 0;
    break;
  case KEY_5_UP:
    switch (item_id)
    {
    case 1:
      var[0] = cap[0];
      break;
    case 2:
      var[1]= cap[1];
      break;
    case 3:
      var[2]=cap[2];
      break;
    case 4:
      var[3]=cap[3];
      break;
    }
    state = 1;
    edit_col = 0;
    break;
  }
}

// 显示部分
LCD_DisplayString(81, 0, g_cap_para_titles[0], 16, 0);
for (j = 1; j < g_cap_num.cap_num+1; j++)
{
  LCD_DisplayString(0, y, g_cap_para_titles[j], 16, ((item_id == j) && (state == 1)) ? 1 : 0);
  LCD_DisplayString(18, y, ":", 16, 0); // 冒号不反显
  LCD_DisplayNum(27, y, var[j-1].value, 16, 4, ((item_id == j) && (state == 2) && (edit_col < 4)) ? edit_col : 0xff, 0, 1);
  LCD_DisplayString(66, y, "KVar", 16, 0); // 冒号不反显
  LCD_DisplayString(117, y, (var[j-1].onf == 1) ? g_cap_para_titles[7] : g_cap_para_titles[8], 16, ((item_id == j) && (state == 2) && (edit_col == 4)) ? 1 : 0);
	y=y+20;
}

LCD_DisplayString(0, 112, g_cap_para_titles[5], 16, ((item_id == 5) && (state == 1)) ? 1 : 0);
LCD_DisplayString(204, 112, g_cap_para_titles[6], 16, ((item_id == 6) && (state == 1)) ? 1 : 0);
}



/*********控制参数设置菜单部分********/

void Control_Para_Set(u8 key_val)
{
  static Control_Para_Struct control_para, var;
	uint8_t flag=0;
  code const char *g_control_para_titles[17] = {

      "控制参数\xFD",

      "电压上限",
      "投切方案",
      "电压下限",
      "投切系数\xFD",
      "COS}上限",
      "投入延时",
      "COS}下限",
      "切除\xFD延时",
      "限投次数\xFD",
      "连动间隔",

      "差容",
      "组合",
      "滤波",
      "等容",
      "保存",
      "退出",
  };
  if (state == 0) // 第一次进入菜单，系统变量赋值到临时变量。初始化坐标，浏览模式，编辑坐标为0xff
  {
    control_para = g_control_para;
    var = g_control_para;
    edit_col = 0;
    state = 1;
    item_id = 1;
  }
  else if (state == 1)
  {
    // 浏览模式
    switch (key_val)
    {
    case KEY_0_UP: // KEY0=上键：同列向上循环（原逻辑不变）
      item_id = item_id - 2;
      if (item_id <= 0)
        item_id = item_id + 12;
      break;

    case KEY_1_UP: // KEY1=下键：同列向下循环（原逻辑不变）
      item_id = item_id + 2;
      if (item_id > 12)
        item_id = item_id - 12;
      break;

    case KEY_2_UP: // KEY2=左键：Z型循环的「上一个」（1→7、12→1、2→12...）
      item_id = item_id - 1;
      if (item_id <= 0)
        item_id = item_id + 12;
      break;

    case KEY_3_UP: // KEY3=右键：Z型循环的「下一个」（1→12、12→2、...7→1）
    {
      item_id = item_id + 1;
      if (item_id > 12)
        item_id = item_id - 12;
      break;
    case KEY_4_UP:
      if (item_id == 11)
      {
        if (control_para.vol_up <= control_para.vol_down)
					flag=1;

				if ((control_para.cos_up_f!=0)&&(control_para.cos_up <= control_para.cos_down))
					flag=1;
					
				
				if((Cap_CheckAllEnabledValueEqual(&g_cap,g_cap_num.cap_num)==0)&&(control_para.type==4))
					flag=1;			
				if(flag==0)
        {
          if(Set_Control_Para(&control_para))
					{
					  g_hard_state.iic_err_times++;
					}
					else
					{
          g_control_para = control_para;
          state = 0;
          menu_state.id = PARAM_SETTING;
          LCD_Clear();
          menu_state.switch_temp_flag = 1;
          return;
					}
        }
      }
      else if (item_id == 12)
      {
        state = 0;
        menu_state.id = PARAM_SETTING;
        LCD_Clear();
        menu_state.switch_temp_flag = 1;
        return;
      }
      else
      {

        state = 2; // 进入编辑
        edit_col = 0;
      }
    }
    break;

    case KEY_5_UP:
      state = 0;
      menu_state.id = PARAM_SETTING;
      LCD_Clear();
      menu_state.switch_temp_flag = 1;
      return;
      break;
    }
    var = control_para;
  }
  else if (state == 2) // 参数编辑
  {
    switch (key_val)
    {
    case KEY_0_UP: // 加

    case KEY_1_UP: // 减
      if (item_id == 1)
      {
        adjust_digit_u16(&var.vol_up, edit_col, key_val == KEY_0_UP ? 1 : -1, 3); //
      }
      if (item_id == 2)
      {
        if (key_val == KEY_0_UP)
        {
          var.type++;
          if (var.type > 4)
            var.type = 1;
          ; //
        }
        if (key_val == KEY_1_UP)
        {
          var.type--;
          if (var.type < 1)
            var.type = 4; //
        }
      }
      if (item_id == 3)
      {
        adjust_digit_u16(&var.vol_down, edit_col, key_val == KEY_0_UP ? 1 : -1, 3); //
      }
      if (item_id == 4)
      {
        adjust_digit_u81(&var.factor, edit_col, key_val == KEY_0_UP ? 1 : -1, 3); //
      }
      if (item_id == 5)
      {
        if (edit_col == 0)
        {
          if (var.cos_up_f)
            var.cos_up_f = 0;
          else
            var.cos_up_f = 1;
        }
        else
        {
          adjust_digit_u81(&var.cos_up, edit_col-1, key_val == KEY_0_UP ? 1 : -1, 3); //
        }
      }
      if (item_id == 6)
      {
        adjust_digit_u16(&var.delay_time_on, edit_col, key_val == KEY_0_UP ? 1 : -1, 3); //
      }
      if (item_id == 7)
      {
        adjust_digit_u8(&var.cos_down, edit_col, key_val == KEY_0_UP ? 1 : -1, 2); //
      }
      if (item_id == 8)
      {
        adjust_digit_u16(&var.delay_time_off, edit_col, key_val == KEY_0_UP ? 1 : -1, 3); //
      }
      if (item_id == 9)
      {
        adjust_digit_u16(&var.times, edit_col, key_val == KEY_0_UP ? 1 : -1, 3); //
      }
      if (item_id == 10)
      {
        adjust_digit_u8(&var.time_interval, edit_col, key_val == KEY_0_UP ? 1 : -1, 2); //
      }
      break;
    case KEY_2_UP:
      edit_col--;
      if ((item_id == 1) || (item_id == 3) || (item_id == 9) || (item_id == 4) || (item_id == 6) || (item_id == 8))
      {
        if (edit_col < 0)
          edit_col = 2;
      }
      if (item_id == 5)
      {
        if (edit_col < 0)
          edit_col = 3;
      }
      if ((item_id == 7) || (item_id == 10))
      {
        if (edit_col < 0)
          edit_col = 1;
      }
      break;
    case KEY_3_UP:
      edit_col++;
      if ((item_id == 1) || (item_id == 3) || (item_id == 9) || (item_id == 4) || (item_id == 6) || (item_id == 8))
      {
        if (edit_col > 2)
          edit_col = 0;
      }
      if (item_id == 5)
      {
        if (edit_col > 3)
          edit_col = 0;
      }
      if ((item_id == 7) || (item_id == 10))
      {
        if (edit_col > 1)
          edit_col = 0;
      }
      break;
    case KEY_4_UP:

      if (item_id == 5)
      {
        if ((var.cos_up != 0)&&(var.cos_up<=100))
        {

          control_para.cos_up_f = var.cos_up_f;
          control_para.cos_up = var.cos_up;
          state = 1;
          edit_col = 0;
        }
      }
      else if (item_id == 4)
      {
        if ((var.factor >= 50) && (var.factor <= 150))
        {
          state = 1;
          control_para.factor = var.factor;
          edit_col = 0;
        }
      }
      else if (item_id == 7)
      {
        if ((var.cos_down > 0))
        {
          state = 1;
          control_para.cos_down = var.cos_down;
          edit_col = 0;
        }
      }			
      else
      {
        switch (item_id)
        {
        case 1:
          control_para.vol_up = var.vol_up;
          break;
        case 2:

          control_para.type = var.type;
          break;
        case 3:
          control_para.vol_down = var.vol_down;
          break;
        case 6:
          control_para.delay_time_on = var.delay_time_on;
          break;
        case 8:
          control_para.delay_time_off = var.delay_time_off;
          break;
        case 9:
          control_para.times = var.times;
          break;
        case 10:
          control_para.time_interval = var.time_interval;
          break;
        }

        state = 1;
        edit_col = 0;
      }
      break;
    case KEY_5_UP:
        switch (item_id)
        {
        case 1:
          var.vol_up = control_para.vol_up;
          break;
        case 2:
          var.type = control_para.type;
          break;
        case 3:
          var.vol_down = control_para.vol_down;
          break;
        case 4:
          var.factor = control_para.factor;
          break;				
        case 5:
          var.cos_up_f = control_para.cos_up_f;
          var.cos_up = control_para.cos_up;
          break;							
        case 6:
          var.delay_time_on = control_para.delay_time_on;
          break;
        case 7:
          var.cos_down = control_para.cos_down;
          break;
        case 8:
          var.delay_time_off = control_para.delay_time_off;
          break;
        case 9:
          var.times = control_para.times;
          break;
        case 10:
          var.time_interval = control_para.time_interval;
          break;
        }
      state = 1;
      edit_col = 0;
      break;
    }
  }

  // 显示部分

  LCD_DisplayString(81, 0, g_control_para_titles[0], 16, 0);

  LCD_DisplayString(0, 18, g_control_para_titles[1], 16, ((item_id == 1) && (state == 1)) ? 1 : 0);
  LCD_DisplayString(69, 18, ":", 16, 0); // 冒号不反显
  LCD_DisplayFixedPoint(75, 18, var.vol_up / 10, var.vol_up % 10, 2, 1, 16,
                        ((item_id == 1) && (state == 2)) ? edit_col : 0xFF, 0);
  LCD_DisplayString(105, 18, "KV", 16, 0);

  LCD_DisplayString(129, 18, g_control_para_titles[2], 16, ((item_id == 2) && (state == 1)) ? 1 : 0);
  LCD_DisplayChar(198, 18, ":", 16, 0);                                                                           // 冒号不反显
  LCD_DisplayString(204, 18, g_control_para_titles[10 + var.type], 16, ((item_id == 2) && (state == 2)) ? 1 : 0); // 冒号不反显

  LCD_DisplayString(0, 36, g_control_para_titles[3], 16, ((item_id == 3) && (state == 1)) ? 1 : 0);
  LCD_DisplayString(69, 36, ":", 16, 0); // 冒号不反显
  LCD_DisplayFixedPoint(75, 36, var.vol_down / 10, var.vol_down % 10, 2, 1, 16,
                        ((item_id == 3) && (state == 2)) ? edit_col : 0xFF, 0);
  LCD_DisplayString(105, 36, "KV", 16, 0);

  LCD_DisplayString(129, 36, g_control_para_titles[4], 16, ((item_id == 4) && (state == 1)) ? 1 : 0);
  LCD_DisplayChar(198, 36, ":", 16, 0); // 冒号不反显
  LCD_DisplayFixedPoint(204, 36, var.factor / 100, var.factor % 100, 1, 2, 16,
                        ((item_id == 4) && (state == 2)) ? edit_col : 0xFF, 0);

  LCD_DisplayString(0, 54, g_control_para_titles[5], 16, ((item_id == 5) && (state == 1)) ? 1 : 0);
  LCD_DisplayString(69, 54, ":", 16, 0);                                                                                // 冒号不反显
  LCD_DisplayString(75, 54, var.cos_up_f ? "-" : "+", 16, ((item_id == 5) && (state == 2) && (edit_col == 0)) ? 1 : 0); //+-
  LCD_DisplayFixedPoint(84, 54, var.cos_up / 100, var.cos_up % 100, 1, 2, 16,
                        ((item_id == 5) && (state == 2) && (edit_col > 0)) ? (edit_col - 1) : 0xFF, 0);

  LCD_DisplayString(129, 54, g_control_para_titles[6], 16, ((item_id == 6) && (state == 1)) ? 1 : 0);
  LCD_DisplayChar(198, 54, ":", 16, 0); // 冒号不反显
  LCD_DisplayNum(204, 54, var.delay_time_on, 16, 3, ((item_id == 6) && (state == 2)) ? edit_col : 0xff, 0, 1);
  LCD_DisplayChar(231, 54, "s", 16, 0); // 冒号不反显

  LCD_DisplayString(0, 72, g_control_para_titles[7], 16, ((item_id == 7) && (state == 1)) ? 1 : 0);
  LCD_DisplayString(69, 72, ":", 16, 0);  // 冒号不反显
  LCD_DisplayString(75, 72, "0.", 16, 0); // 冒号不反显
  LCD_DisplayNum(87, 72, var.cos_down, 16, 2, ((item_id == 7) && (state == 2)) ? edit_col : 0xff, 0, 1);

  LCD_DisplayString(129, 72, g_control_para_titles[8], 16, ((item_id == 8) && (state == 1)) ? 1 : 0);
  LCD_DisplayString(198, 72, ":", 16, 0); // 冒号不反显
  LCD_DisplayNum(204, 72, var.delay_time_off, 16, 3, ((item_id == 8) && (state == 2)) ? edit_col : 0xff, 0, 1);
  LCD_DisplayChar(231, 72, "s", 16, 0); // 冒号不反显

  LCD_DisplayString(0, 90, g_control_para_titles[9], 16, ((item_id == 9) && (state == 1)) ? 1 : 0);
  LCD_DisplayString(69, 90, ":", 16, 0); // 冒号不反显
  LCD_DisplayNum(75, 90, var.times, 16, 3, ((item_id == 9) && (state == 2)) ? edit_col : 0xff, 0, 1);
  LCD_DisplayString(102, 90, "/日", 16, 0); // 冒号不反显

  LCD_DisplayString(129, 90, g_control_para_titles[10], 16, ((item_id == 10) && (state == 1)) ? 1 : 0);
  LCD_DisplayString(198, 90, ":", 16, 0); // 冒号不反显
  LCD_DisplayNum(204, 90, var.time_interval, 16, 2, ((item_id == 10) && (state == 2)) ? edit_col : 0xff, 0, 1);
  LCD_DisplayString(222, 90, "s", 16, 0); // 冒号不反显

  LCD_DisplayString(0, 110, g_control_para_titles[15], 16, ((item_id == 11) && (state == 1)) ? 1 : 0);
  LCD_DisplayString(204, 110, g_control_para_titles[16], 16, ((item_id == 12) && (state == 1)) ? 1 : 0);
}



// ==================== 通用电压保护参数设置函数（C89+宏定义） ====================
void Sys_Vol_Prot_Para_Set(u8 key_val, VolProtType prot_type)
{
  static Protect_Para_Struct pro, var;
  code const char *g_voll_prot_titles[7] = {

      "系统欠压保护", "欠压投退", "欠压电压", "欠压时间", "保存", "退出", "投入"

  };
  code const char *g_volh_prot_titles[4] = {

      "系统过\xFD压保护 ", "过\xFD压投退", "过\xFD压电压", "过\xFD压时间 "
  };

  // 初始化
  if (state == 0)
  {
    if (prot_type == VOL_PROT_H)
    {
      pro = g_vol_h;
      var = g_vol_h;
    }
    else
    {
      pro = g_vol_l;
      var = g_vol_l;
    }
    edit_col = 0;
    item_id = 1;
    state = 1;
  }

  else if (state == 1)
  {
    // 浏览模式
    switch (key_val)
    {
    case KEY_0_UP:
      if (item_id < 5)
      {
        item_id--;
        if (item_id < 1)
          item_id = 4;
      }
      break;
    case KEY_1_UP:
      if (item_id < 5)
      {
        item_id++;
        if (item_id > 4)
          item_id = 1;
      }

      break;
    case KEY_2_UP:
    case KEY_3_UP:
      if (item_id == 5)
        item_id = 4;
      else if (item_id == 4)
        item_id = 5;
      break;
    case KEY_4_UP:
      if (item_id <= 3)
      {
        state = 2; // 编辑前3项
        edit_col = 0;
      }
      else if (item_id == 4)
      {
				    if (prot_type == VOL_PROT_H)
    {
			         if(Set_OV_Protect_Para(&pro))
								 g_hard_state.iic_err_times++;
							 else
							 {
							   g_vol_h=pro;
								menu_state.id = PROT_PARAM; // 保存
								state = 0;
								LCD_Clear();
								menu_state.switch_temp_flag = 1;								 
							         return;
							 }
								 
		}
		else
		{
		
				         if(Set_UV_Protect_Para(&pro))
								 g_hard_state.iic_err_times++;
							 else
							 {
							   g_vol_l=pro;
								menu_state.id = PROT_PARAM; // 保存
								state = 0;
								LCD_Clear();
								menu_state.switch_temp_flag = 1;								 
							         return;
							 }	
		
		
		}

      }
      else if (item_id == 5)
      {
        menu_state.id = PROT_PARAM; // 保存
        state = 0;
        LCD_Clear();
        menu_state.switch_temp_flag = 1;
        return;
      }
      break;
    case KEY_5_UP:
      menu_state.id = PROT_PARAM; // 保存
      state = 0;
      LCD_Clear();
      menu_state.switch_temp_flag = 1;
      return;
      break;
    }
    var = pro;
  }
  else if (state == 2)
  { // 编辑模式

    switch (key_val)
    {
    case KEY_0_UP: // 加
    case KEY_1_UP: // 减（对开关量，加减效果相同）
      if (item_id == 1)
      {
        var.onf = !var.onf; // 切换 on/off
      }
      else if (item_id == 2)
      {

        adjust_digit_u16(&var.value, edit_col, (key_val == KEY_0_UP) ? 1 : -1, 4);
      }
      else if (item_id == 3)
      {
        adjust_digit_u16(&var.time, edit_col, (key_val == KEY_0_UP) ? 1 : -1, 4);
      }
      break;
    case KEY_2_UP: // 左
      if (item_id >= 2 && item_id <= 3)
      {
        edit_col--;
        if (edit_col < 0)
          edit_col = 3;
      }
      break;
    case KEY_3_UP: // 右
      if (item_id >= 2 && item_id <= 3)
      {
        edit_col++;
        if (edit_col > 3)
          edit_col = 0;
      }
      break;
    case KEY_4_UP: // 退出编辑
      state = 1;
      edit_col = 0;
      switch (item_id)
      {
      case 1:
          pro.onf = var.onf;
        break;
      case 2:
          pro.value = var.value;
        break;
      case 3:
          pro.time = var.time;
			break;
		}
      break;
    case KEY_5_UP:
      switch (item_id)
      {
      case 1:
          var.onf = pro.onf;
        break;
      case 2:
          var.value = pro.value;
        break;
      case 3:
          var.time = pro.time;
        break;
      }
      state = 1;
      edit_col = 0;
      break;
    }
  }

  // 显示（仅中文示例）

  LCD_DisplayString(66, 0, prot_type == VOL_PROT_L ? g_voll_prot_titles[0] : g_volh_prot_titles[0], 16, 0);

  // 投退
  LCD_DisplayString(0, 20, prot_type == VOL_PROT_L ? g_voll_prot_titles[1] : g_volh_prot_titles[1], 16, ((item_id == 1) && (state == 1)));
  LCD_DisplayChar(72, 20, ":", 16, 0);
  LCD_DisplayString(72 + 9, 20,
                    var.onf ? g_voll_prot_titles[6] : g_voll_prot_titles[5], // "投入" 或 "退出"
                    16, ((item_id == 1) && (state == 2)));

  // 电压值
  LCD_DisplayString(0, 40, prot_type == VOL_PROT_L ? g_voll_prot_titles[2] : g_volh_prot_titles[2], 16, ((item_id == 2) && (state == 1)));
  LCD_DisplayString(72, 40, ":", 16, 0);
  LCD_DisplayFixedPoint(72 + 9, 40, var.value / 100, var.value % 100, 2, 2, 16,
                        ((item_id == 2) && (state == 2)) ? edit_col : 0xFF, 0);
  LCD_DisplayString(72 + 51, 40, "KV", 16, 0);

  // 时间
  LCD_DisplayString(0, 60, prot_type == VOL_PROT_L ? g_voll_prot_titles[3] : g_volh_prot_titles[3], 16, ((item_id == 3) && (state == 1)));
  LCD_DisplayString(72, 60, ":", 16, 0);
  LCD_DisplayFixedPoint(72 + 9, 60, var.time / 100, var.time % 100, 2, 2, 16,
                        ((item_id == 3) && (state == 2)) ? edit_col : 0xFF, 0);
  LCD_DisplayChar(72 + 51, 60, "s", 16, 0);

  // 按钮
  LCD_DisplayString(0, 112, g_voll_prot_titles[4], 16, ((item_id == 4) && (state == 1)));
  LCD_DisplayString(204, 112, g_voll_prot_titles[5], 16, ((item_id == 5) && (state == 1)));
}

void Sys_Cap_Prot_Para_Set(u8 key_val, char num)
{
  static Cap_Protect_Para_Struct pro, var;

  code const char *g_cap_titles[7] = {

      "一路电容保护", "二路电容保护", "三\xFD路电容保护", "四路电容保护", "投入", "退出", "保存"

  };
  code const char *g_cap_prot_titles[9] = {

      "过\xFD流投退", "过\xFD流电流", "过\xFD流时间", "速断投退", "速断电流", "速断时间", "零序投退", "零序电压", "零序时间"

  };

  // 初始化
  if (state == 0)
  {

    pro = g_cap_protect[num];
    var=g_cap_protect[num];
    edit_col = 0;
    item_id = 1;
    state = 1;
  }

  else if (state == 1)
  {
    // 浏览模式
    switch (key_val)
    {
    case KEY_0_UP: // 上
      if (item_id % 3 == 0)
        item_id = item_id - 1;
      else if (item_id % 3 == 1)
        item_id = item_id - 3;
      else
        item_id = item_id - 2;

      if (item_id < 0)
        item_id = item_id + 12;
      if (item_id == 0)
        item_id = 11;
      break;
    case KEY_1_UP: // 下
      if (item_id % 3 == 0)
        item_id = item_id + 2;
      else if (item_id % 3 == 1)
        item_id = item_id + 3;
      else
        item_id = item_id + 1;
      if (item_id > 11)
        item_id = item_id - 12;
      if (item_id == 0)
        item_id = 2;
      break;
    case KEY_2_UP:
    case KEY_3_UP: // 左右键
      if (item_id < 4)
      {
        if (item_id == 1)
          item_id = 2;
        else
          item_id = 1;
      }
      else if (item_id < 7)
      {
        if (item_id == 4)
          item_id = 5;
        else
          item_id = 4;
      }
      else if (item_id < 10)
      {
        if (item_id == 7)
          item_id = 8;
        else
          item_id = 7;
      }
      else if (item_id < 13)
      {
        if (item_id == 10)
          item_id = 11;
        else
          item_id = 10;
      }
      break;
    case KEY_4_UP:
      if (item_id <= 9)
      {
        state = 2; // 
        edit_col = 0;
      }
      else if (item_id == 10)
      {
        if(Set_Cap_Protect_Para_Ch(num,&pro))
					g_hard_state.iic_err_times++;
				else
				{
        menu_state.id = PROT_PARAM; // 保存
        state = 0;
        LCD_Clear();
        menu_state.switch_temp_flag = 1;
        return;
				}
      }
      else if (item_id == 11)
      {
        menu_state.id = PROT_PARAM; // 保存
        state = 0;
        LCD_Clear();
        menu_state.switch_temp_flag = 1;
        return;
      }
      break;
    case KEY_5_UP:
      menu_state.id = PROT_PARAM; // 保存
      state = 0;
      LCD_Clear();
      menu_state.switch_temp_flag = 1;
      return;
      break;
    }
    var = pro;
  }
  else if (state == 2)
  { // 编辑模式

    switch (key_val)
    {
    case KEY_0_UP: // 加
    case KEY_1_UP: // 减（对开关量，加减效果相同）

      switch (item_id)
      {

      case 1:
        var.over_onf = !var.over_onf; // 切换 on/off
        break;
      case 2:
        adjust_digit_u16(&var.over_value, edit_col, (key_val == KEY_0_UP) ? 1 : -1, 4);
        break;
      case 3:
        adjust_digit_u16(&var.over_time, edit_col, (key_val == KEY_0_UP) ? 1 : -1, 4);
        break;
      case 4:
        var.quick_onf = !var.quick_onf; // 切换 on/off
        break;
      case 5:
        adjust_digit_u16(&var.quick_value, edit_col, (key_val == KEY_0_UP) ? 1 : -1, 4);
        break;
      case 6:
        adjust_digit_u16(&var.quick_time, edit_col, (key_val == KEY_0_UP) ? 1 : -1, 4);
        break;
      case 7:
        var.zero_onf = !var.zero_onf; // 切换 on/off
        break;
      case 8:
        adjust_digit_u16(&var.zero_value, edit_col, (key_val == KEY_0_UP) ? 1 : -1, 4);
        break;
      case 9:
        adjust_digit_u16(&var.zero_time, edit_col, (key_val == KEY_0_UP) ? 1 : -1, 4);
        break;
      }
      break;
    case KEY_2_UP: // 左
      if ((item_id != 1) && (item_id != 4) && (item_id != 7))
      {
        edit_col--;
        if (edit_col < 0)
          edit_col = 3;
      }
      break;
    case KEY_3_UP: // 右
      if ((item_id != 1) && (item_id != 4) && (item_id != 7))
      {
        edit_col++;
        if (edit_col > 3)
          edit_col = 0;
      }
      break;
    case KEY_4_UP: // 退出编辑
      state = 1;
      edit_col = 0;
      switch (item_id)
      {

      case 1:
        pro.over_onf = var.over_onf;
        break;
      case 2:
        pro.over_value = var.over_value;
        break;
      case 3:
        pro.over_time = var.over_time;
        break;
      case 4:
        pro.quick_onf = var.quick_onf;
        break;
      case 5:
        pro.quick_value = var.quick_value;
        break;
      case 6:
        pro.quick_time = var.quick_time;
        break;
      case 7:
        pro.zero_onf = var.zero_onf;
        break;
      case 8:
        pro.zero_value = var.zero_value;
        break;
      case 9:
        pro.zero_time = var.zero_time;
        break;
      }
      break;
    case KEY_5_UP:
  switch (item_id)
      {

      case 1:
        var.over_onf = pro.over_onf;
        break;
      case 2:
        var.over_value = pro.over_value;
        break;
      case 3:
        var.over_time = pro.over_time;
        break;
      case 4:
        var.quick_onf = pro.quick_onf;
        break;
      case 5:
        var.quick_value = pro.quick_value;
        break;
      case 6:
        var.quick_time = pro.quick_time;
        break;
      case 7:
        var.zero_onf = pro.zero_onf;
        break;
      case 8:
        var.zero_value = pro.zero_value;
        break;
      case 9:
        var.zero_time = pro.zero_time;
        break;
      }
      state = 1;
      edit_col = 0;
      break;
    }
  }

  // 显示（仅中文示例）

  LCD_DisplayString(66, 0, g_cap_titles[num], 16, 0);

  // 投退
  LCD_DisplayString(0, 16, g_cap_prot_titles[0], 16, ((item_id == 1) && (state == 1)));
  LCD_DisplayChar(72, 16, "#", 16, 0);
  LCD_DisplayString(75, 16,
                    var.over_onf ? g_cap_titles[4] : g_cap_titles[5], // "投入" 或 "退出"
                    16, ((item_id == 1) && (state == 2)));

  // 电流值
  LCD_DisplayString(120, 16, g_cap_prot_titles[1], 16, ((item_id == 2) && (state == 1)));
  LCD_DisplayString(189, 16, "#", 16, 0);
  LCD_DisplayFixedPoint(192, 16, var.over_value/100,var.over_value%100,2,2, 16,  ((item_id == 2) && (state == 2)) ? edit_col : 0xFF, 0);
  LCD_DisplayString(231, 16, "A", 16, 0);

  // 时间
  LCD_DisplayString(120, 31, g_cap_prot_titles[2], 16, ((item_id == 3) && (state == 1)));
  LCD_DisplayString(189, 31, "#", 16, 0);
  LCD_DisplayFixedPoint(192, 31, var.over_time / 100, var.over_time % 100, 2, 2, 16,
                        ((item_id == 3) && (state == 2)) ? edit_col : 0xFF, 0);
  LCD_DisplayChar(231, 31, "s", 16, 0);

  LCD_DisplayString(0, 48, g_cap_prot_titles[3], 16, ((item_id == 4) && (state == 1)));
  LCD_DisplayChar(72, 48, "#", 16, 0);
  LCD_DisplayString(75, 48,
                    var.quick_onf ? g_cap_titles[4] : g_cap_titles[5], // "投入" 或 "退出"
                    16, ((item_id == 4) && (state == 2)));

  // 电流值
  LCD_DisplayString(120, 47, g_cap_prot_titles[4], 16, ((item_id == 5) && (state == 1)));
  LCD_DisplayString(189, 47, "#", 16, 0);
  LCD_DisplayFixedPoint(192, 48, var.quick_value/100,var.quick_value%100,2,2 ,16,  ((item_id == 5) && (state == 2)) ? edit_col : 0xFF, 0);
  LCD_DisplayString(231, 47, "A", 16, 0);

  // 时间
  LCD_DisplayString(120, 62, g_cap_prot_titles[5], 16, ((item_id == 6) && (state == 1)));
  LCD_DisplayString(189, 62, "#", 16, 0);
  LCD_DisplayFixedPoint(192, 62, var.quick_time / 100, var.quick_time % 100, 2, 2, 16,
                        ((item_id == 6) && (state == 2)) ? edit_col : 0xFF, 0);
  LCD_DisplayChar(231, 62, "s", 16, 0);

  LCD_DisplayString(0, 80, g_cap_prot_titles[6], 16, ((item_id == 7) && (state == 1)));
  LCD_DisplayChar(72, 80, "#", 16, 0);
  LCD_DisplayString(75, 80,
                    var.zero_onf ? g_cap_titles[4] : g_cap_titles[5], // "投入" 或 "退出"
                    16, ((item_id == 7) && (state == 2)));

  // 电压值
  LCD_DisplayString(120, 78, g_cap_prot_titles[7], 16, ((item_id == 8) && (state == 1)));
  LCD_DisplayString(189, 78, "#", 16, 0);
  LCD_DisplayFixedPoint(192, 78, var.zero_value/100,var.zero_value%100,2,2 ,16, ((item_id == 8) && (state == 2)) ? edit_col : 0xFF, 0);
  LCD_DisplayString(231, 78, "V", 16, 0);

  // 时间
  LCD_DisplayString(120, 93, g_cap_prot_titles[8], 16, ((item_id == 9) && (state == 1)));
  LCD_DisplayString(189, 93, "#", 16, 0);
  LCD_DisplayFixedPoint(192, 93, var.zero_time / 100, var.zero_time % 100, 2, 2, 16,
                        ((item_id == 9) && (state == 2)) ? edit_col : 0xFF, 0);
  LCD_DisplayChar(231, 93, "s", 16, 0);

  // 按钮
  LCD_DisplayString(0, 112, g_cap_titles[6], 16, ((item_id == 10) && (state == 1)));
  LCD_DisplayString(204, 112, g_cap_titles[5], 16, ((item_id == 11) && (state == 1)));
}

/*********通讯参数设置菜单部分********/
void Sys_Com_Para_Set(u8 key_val)
{

  static Com_Para_Struct com, var;
	static uint8_t key[4]={0};
	static num=0;
  code const char *g_com_para_titles[6] = {

      "通讯参数\xFD",
      "通讯地址",
      "波特率值",
      "通讯规约",
      "保存",
      "退出",
  };

  if (state == 0) // 第一次进入菜单，系统变量赋值到临时变量。初始化坐标，浏览模式，编辑坐标为0xff
  {
    com = g_com;
    var = g_com;
    edit_col = 0;
    state = 1;
    item_id = 1;
		key[0]=0;
		key[1]=0;
		key[2]=0;
		key[3]=0;	
    num=0;		
  }
  else if (state == 1)
  {
    // 浏览模式
    switch (key_val)
    {
    case KEY_0_UP:
      if (item_id < 5)
      {
        item_id--;
        if (item_id < 1)
          item_id = 4;
      }
      break;
    case KEY_1_UP:
      if (item_id < 5)
      {
        item_id++;
        if (item_id > 4)
          item_id = 1;
      }

      break;
    case KEY_2_UP:
    case KEY_3_UP:
      if (item_id == 5)
        item_id = 4;
      else if (item_id == 4)
        item_id = 5;
			
			if((com.address==0xff)&&(item_id==3))
			{
			     key[num++]=key_val;
				  if(num==4)
					{
					  if((key[0]==KEY_2_UP)&&(key[1]==KEY_2_UP)&&(key[2]==KEY_3_UP)&&(key[3]==KEY_3_UP))
						{
	      menu_state.id = PASSWORD;
        menu_state.last_id = STATISTIC_PARAM;
        LCD_Clear();
        state = 0; // 保存
        menu_state.switch_temp_flag = 1;	
        num=0;
        return ;							
							
						}
					  num=0;
					}
			}
			
			
      break;

    case KEY_4_UP:
      if (item_id <= 2)
      {
        state = 2; // 进入编辑
        edit_col = 0;
      }
      else if (item_id == 4)
      {
        if(Set_Com_Para(&com))
					g_hard_state.iic_err_times++;
				else
				{
        g_com = com;
        menu_state.id = PARAM_SETTING;
        LCD_Clear();
        state = 0; // 保存
        menu_state.switch_temp_flag = 1;
				
        return;
				}
      }
      else if (item_id == 5)
      {
        menu_state.id = PARAM_SETTING;
        LCD_Clear();
        state = 0; // 退出（不保存）
        menu_state.switch_temp_flag = 1;
        return;
      }
      break;

    case KEY_5_UP:
      menu_state.id = PARAM_SETTING;
      LCD_Clear();
      state = 0; // 退出（不保存）
      menu_state.switch_temp_flag = 1;
      return;
      break;
    }
    var = com;
  }
  else if (state == 2) // 参数编辑
  {
    switch (key_val)
    {
    case KEY_0_UP: // 加
      if (item_id == 1)
      {
        adjust_hex_digit(&var.address, edit_col, 1);
      }
      if (item_id == 2)
      {
        var.bps = var.bps << 1;
        if (var.bps > 9600)
          var.bps = 1200;
      }
      break;
    case KEY_1_UP: // 减
      if (item_id == 1)
      {
        adjust_hex_digit(&var.address, edit_col, -1);
      }
      if (item_id == 2)
      {
        var.bps = var.bps >> 1;
        if (var.bps < 1200)
          var.bps = 9600;
      }
      break;
    case KEY_2_UP:
      edit_col--;
      if ((item_id == 1))
      {
        if (edit_col <0)
        {
          edit_col = 1;
        }
      }
      break;
    case KEY_3_UP:
      edit_col++;
      if ((item_id == 1))
      {
        if (edit_col > 1)
        {
          edit_col = 0;
        }
      }
      break;
    case KEY_4_UP:

      com = var;

      state = 1;
      edit_col = 0xff;
      break;
    case KEY_5_UP:
      var = com;
      state = 1;
      edit_col = 0xff;
      break;
    }
  }

  // 显示部分

  LCD_DisplayString(81, 0, g_com_para_titles[0], 16, 0);

  LCD_DisplayString(0, 20, g_com_para_titles[1], 16, ((item_id == 1) && (state == 1)) ? 1 : 0);
  LCD_DisplayChar(72, 20, ":", 16, 0); // 冒号不反显

  LCD_DisplayHex(81, 20, var.address, 16, 2, ((item_id == 1) && (state == 2)) ? edit_col : 0xff, 0);

  LCD_DisplayChar(99, 20, "H", 16, 0); // 冒号不反显

  LCD_DisplayString(0, 40, g_com_para_titles[2], 16, ((item_id == 2) && (state == 1)) ? 1 : 0);
  LCD_DisplayChar(72, 40, ":", 16, 0); // 冒号不反显
  LCD_DisplayNum(81, 40, var.bps, 16, 4, 0xff, ((item_id == 2) && (state == 2)) ? 1 : 0, 1);
  //    LCD_DisplayString(x1 + 45, 40, "bps", 16, 0);

  LCD_DisplayString(0, 60, g_com_para_titles[3], 16, ((item_id == 3) && (state == 1)) ? 1 : 0);
  LCD_DisplayChar(72, 60, ":", 16, 0); // 冒号不反显

  LCD_DisplayString(81, 60, "Modbus", 16, 0);

  LCD_DisplayString(0, 112, g_com_para_titles[4], 16, ((item_id == 4) && (state == 1)) ? 1 : 0);
  LCD_DisplayString(204, 112, g_com_para_titles[5], 16, ((item_id == 5) && (state == 1)) ? 1 : 0);
}
