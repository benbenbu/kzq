




#include "config.h"
#include "system.h"
#include "fm31256.h"
#include "menu.h"
#include "adc_key.h"
#include "bsp.h"
#include "menu.h"
#include "lcd.h"
#include "data_save.h"
#include "HT7036.h"
#include "time.h"
#include "delay.h"
#include "data_deal.h"
#include "uart.h"
//参数定义，系统用参数



//参数定义，用户参数

 Date_Struct  system_date;
 Time_Struct  system_time;
 
volatile   Cap_Data  g_cap_data[4];
volatile   Sys_Data  g_sys_data;
 SysHard_State  g_hard_state;
 
extern void Beep_Timer_Handler(void);

void system_int()
{
  char i=0;	
  bsp_int();
	 Delay_ms(100);
  i=Sys_Param_Init();	
  switch(i)
	{
	  case 0: FM31256_RTC_Read_Date(&system_date); FM31256_RTC_Read_Time(&system_time);		break;

		case 1:  BEEP=0; break;//iic错
		case 2:  BEEP=0; break;//无数据		
	}
	FM31256_WDG_Enable(WDG_2S);
	UART0_Init(g_com.bps);
	LCD_Init();
	Bsp_InitKey();		
  Ht7036_init()	;
  menu_init();	
	g_sys_data.q=0;
	g_sys_data.p=0;	
	g_sys_data.i=0;
	g_sys_data.u=0;
	g_sys_data.c=0;	
	g_cap_data[0].ia=0;
	g_cap_data[0].ic=0;
	g_cap_data[0].uo=0;	
	
	g_cap_data[1].ia=0;
	g_cap_data[1].ic=0;
	g_cap_data[1].uo=0;	

	g_cap_data[2].ia=0;
	g_cap_data[2].ic=0;
	g_cap_data[2].uo=0;	

	g_cap_data[3].ia=0;
	g_cap_data[3].ic=0;
	g_cap_data[3].uo=0;	
	
	g_all_io_state=0;
	g_relay_state=0;
	err_state=0;
	com_led_flag=0;
	com_led_timer=0;
	for(i=0;i<6;i++)
	{
	g_sys_flag[i]=0;
	}
	EA=1;
}

/**
 * @brief 每日/每月清零投切次数（解决0点断电漏清零，适配两位数年份的Stat_date结构体）
 * @note 1. 需在主循环/1s定时器中调用，依赖FM31256 RTC时钟；
 *       2. 每次调用都判断跨日/跨月，断电后开机仍能补清零，不再局限0点0~1分；
 *       3. 年份为两位数（00-99），无需转换，直接对比；
 */
void cap_reset_daily_count(void)
{
    // C89：所有局部变量前置声明
    uint8_t curr_hour;
    uint8_t curr_min;
    uint8_t curr_day;
    uint8_t curr_month;
    uint8_t curr_year;   // 两位数年份（00-99），直接使用
    uint8_t is_cross_day = 0;    // 标记：是否跨日
    uint8_t is_cross_month = 0;  // 标记：是否跨月
    uint8_t is_update_stat = 0;  // 标记：是否需要更新统计值到存储
    uint8_t is_update_date = 0;  // 标记：是否需要更新清零日期到存储

    // 1. 提取当前时间参数（适配新Stat_date结构体，年份直接用两位数）
    curr_hour = system_time.hour;   // 当前小时（0~23）
    curr_min = system_time.minute;  // 当前分钟（0~59）
    curr_day = system_date.day;     // 当前日期（1~31）
    curr_month = system_date.month; // 当前月份（1~12）
    curr_year = system_date.year;   // 两位数年份（如26=2026，00=2000）

    // ==================== 核心优化：每次调用都判断跨日/跨月，不局限0点窗口 ====================
    // 2.1 跨日判断（核心：当前日期≠清零日期 → 跨日，无论是否在0点窗口）
    if (curr_day != g_last_reset_date.day || 
        curr_month != g_last_reset_date.month ||
        curr_year != g_last_reset_date.year) {
        is_cross_day = 1;
    }

    // 2.2 跨月判断（核心：当前月份≠清零月份 → 跨月，无论是否在0点窗口）
    if (curr_month != g_last_reset_date.month ||
        curr_year != g_last_reset_date.year) {
        is_cross_month = 1;
    }

    // ==================== 清零执行逻辑：分“窗口期优先”和“断电补清零” ====================
    if (is_cross_day || is_cross_month) {
        // 场景1：0点0~3分内（容错窗口扩大到3分钟）→ 正常清零（优先执行）
        // 场景2：非0点窗口但跨日/跨月（如0点1分断电，开机后9点调用）→ 补清零
        if ((curr_hour == 0 && curr_min <= 3) ||  // 优先窗口：0点0~3分（容错）
            (curr_hour > 0 && is_cross_day) ||    // 补清零：跨日且过了0点（任意时间）
            (curr_hour >= 0 && is_cross_month)) { // 补清零：跨月（任意时间）

            // ① 日清零：跨日则清零（解决0点断电漏清零）
            if (is_cross_day) {
                g_stat.day_times_on = 0;    
                g_stat.day_times_off = 0;   
                is_update_stat = 1;
            }

            // ② 月清零：跨月则清零（无需判断1日，跨月即清零）
            if (is_cross_month) {
                g_stat.month_times_on = 0;  
                g_stat.month_times_off = 0;
                is_update_stat = 1;
            }

            // ③ 更新清零日期（完整覆盖新Stat_date结构体，含时分秒）
            g_last_reset_date.year = curr_year;     // 两位数年份，直接赋值
            g_last_reset_date.month = curr_month;
            g_last_reset_date.day = curr_day;
            g_last_reset_date.hour = curr_hour;     // 记录清零时的小时
            g_last_reset_date.minute = curr_min;   // 记录清零时的分钟
            g_last_reset_date.second = system_time.second; // 记录清零时的秒
            is_update_date = 1;
        }
    }

    // ==================== 批量写入存储（避免重复擦写，保护存储芯片） ====================
    if (is_update_stat) {
        Set_Stat_Para(&g_stat);  // 统计值写入存储
    }
    if (is_update_date) {
        Set_Stat_Date(&g_last_reset_date); // 清零日期（含时分秒）写入存储
    }
}
// 每隔1s读取一次时间
void Read_Time_Per_1s(void) {
		static uint32_t last_read_time = 0;    // 【关键】记录上一次读取时间的时间戳，初始化为0
    uint32_t current_time = Get_SystemTime();  // 安全读取当前时间戳
    uint32_t time_diff = current_time - last_read_time;  // 无符号数自动处理溢出
    // 当时间差≥100ms时，执行读取操作
    if (time_diff >= 100) {

			FM31256_RTC_Read_Date(&system_date);
			FM31256_RTC_Read_Time(&system_time);			
      last_read_time = current_time;
			menu_state.timer_1s_flag = 1;
			menu_state.flash=!menu_state.flash;
			cap_reset_daily_count();
      cap_switch_delay_handler();			
      Beep_Timer_Handler();			
    }
}


unsigned int g_adc_raw_values[ADC_SAMPLE_BUF_SIZE] = {0};
unsigned int g_adc_value = 0;
bit g_adc_data_valid  = 0;
static uint8_t s_adc_buf_index = 0;
uint8_t g_key_press_flag = 0;  // 1=有按键按下，0=无键
unsigned int s_LastAdcValue = 0;  // 新增：记录上一次有效ADC值
/*
*********************************************************************************************************
*   函 数 名: UpdateAdcFilter
*   功能说明: 优化的中值滤波（5点中值，抗干扰更强）
*********************************************************************************************************
*/
static void UpdateAdcFilter(void) {
    unsigned int temp;
    // 核心修改：缓冲区改为static，避免栈溢出（51栈空间小）
    static unsigned int buf[ADC_SAMPLE_BUF_SIZE];
    uint8_t i,j;

    // 1. 复制数据（临界区：仅复制时关中断，排序不关）
    EA = 0;
    for ( i = 0; i < ADC_SAMPLE_BUF_SIZE; i++) {
        buf[i] = g_adc_raw_values[i];
    }
    EA = 1; // 立即开中断，避免丢失ADC采样

    // 2. 冒泡排序（开中断执行，不影响采样）
    for ( i = 0; i < ADC_SAMPLE_BUF_SIZE - 1; i++) {
        for ( j = 0; j < ADC_SAMPLE_BUF_SIZE - i - 1; j++) {
            if (buf[j] > buf[j + 1]) {
                temp = buf[j];
                buf[j] = buf[j + 1];
                buf[j + 1] = temp;
            }
        }
    }

    // 3. 更新有效值（仅更新时关中断）
    EA = 0;
    g_adc_value = buf[ADC_SAMPLE_BUF_SIZE / 2];
    g_adc_data_valid = 1;
    EA = 1;
}

void ADC0_ISR(void) interrupt 15 {
    unsigned int val;
     uint8_t is_valid_key_val = 0;

		  char SFRPAGE_SAVE = SFRPAGE; // Save Current SFR page

	  SFRPAGE = ADC0_PAGE;
    if (AD0INT) {
        AD0INT = 0;
        val = (unsigned int)ADC0H << 8 | ADC0L;
			
		SFRPAGE	=SFRPAGE_SAVE;

        // 新增：先判断当前值是否在任意按键阈值窗口内（快速筛选有效值）

        if ((val >= THRESHOLD_KEY0 - THRESHOLD_WINDOW && val <= THRESHOLD_KEY0 + THRESHOLD_WINDOW) ||
            (val >= THRESHOLD_KEY1 - THRESHOLD_WINDOW && val <= THRESHOLD_KEY1 + THRESHOLD_WINDOW) ||
            (val >= THRESHOLD_KEY2 - THRESHOLD_WINDOW && val <= THRESHOLD_KEY2 + THRESHOLD_WINDOW) ||
            (val >= THRESHOLD_KEY3 - THRESHOLD_WINDOW && val <= THRESHOLD_KEY3 + THRESHOLD_WINDOW) ||
            (val >= THRESHOLD_KEY4 - THRESHOLD_WINDOW && val <= THRESHOLD_KEY4 + THRESHOLD_WINDOW) ||
            (val >= THRESHOLD_KEY5 - THRESHOLD_WINDOW && val <= THRESHOLD_KEY5 + THRESHOLD_WINDOW)) {
            is_valid_key_val = 1;
            g_key_press_flag = 1;
        } else {
            // 无键值：仅当按键标志已置1（松开阶段），才更新缓冲区（避免混值）
            if (g_key_press_flag == 0) {
                is_valid_key_val = 1;
            } else {
                // 按键松开：清空缓冲区，重置标志
                s_adc_buf_index = 0;
                g_key_press_flag = 0;
                return; // 丢弃松开阶段的过渡值
            }
        }

        // 仅有效值才存入缓冲区
        if (is_valid_key_val) {
            EA = 0;
            g_adc_raw_values[s_adc_buf_index] = val;
            s_adc_buf_index++;
            if (s_adc_buf_index >= ADC_SAMPLE_BUF_SIZE) {
                s_adc_buf_index = 0;
                UpdateAdcFilter();  // 缓冲区满则滤波
            }
            EA = 1;
        }
    }
		SFRPAGE	=SFRPAGE_SAVE;
}

//tyep:0 液晶输出  1：输入检测
void change_io(uint8_t type)

{
    unsigned char SFRPAGE_SAVE = SFRPAGE; // 保存当前SFR页（C89兼容，char→unsigned char避免符号问题）
    SFRPAGE = CONFIG_PAGE;               // 切换到配置页（端口/交叉开关配置必须此页）
    if(type)
		{
		LVC245_OE =1;
		P2MDOUT = 0x00;   // P2口数据总线输入
		P2=0XFF;	
			
		}
    else
		{
		LVC245_OE=0;
		P2MDOUT = 0xff;   // P2口数据总线=推挽（数据传输需强驱动）			

		}


    SFRPAGE = SFRPAGE_SAVE; // 恢复原SFR页（必须！否则后续寄存器操作会出错）
}






// 关键配置（适配交直流光耦，多数表决逻辑）
#define IO_NUM             11      // 检测的IO总数（I1~I11）
#define IO_VALID_LEVEL     0       // 输入有效电平（光耦导通=低电平）
#define DETECT_INTERVAL_MS 50      // 检测间隔（50ms/次，主循环/定时器调用）
#define VOTE_WINDOW_SIZE   5       // 表决窗口：最近5次检测结果
#define VOTE_THRESHOLD     3       // 多数表决阈值：≥3次一致则确认状态
#define HIGH_LEVEL         1       // 高电平标记
#define LOW_LEVEL          0       // 低电平标记


// 全局IO状态（最终确认的稳定状态，供其他模块使用）
volatile uint16_t g_all_io_state = 0;

/**
 * @brief  读取所有11个输入IO状态（交直流光耦通用，5次3次多数表决，抗交流零点波动）
 * @note   主循环/定时器50ms调用1次，仅写满5次缓冲区后才表决，避免单次执行触发状态变化
 * @retval 无
 */
void Read_All_IO_State(void)
{
    // 1. 静态变量扩展：增加缓冲区写满标记（每个IO独立）
    static uint8_t io_history[IO_NUM][VOTE_WINDOW_SIZE] = {0};    // 每个IO最近5次电平
    static uint8_t io_ptr[IO_NUM] = {0};                          // 每个IO的缓冲区指针（0~4）
    static uint8_t io_buf_full[IO_NUM] = {0};                     // 0=未写满，1=已写满5次
    // 2. 临时变量
    uint8_t io_level[IO_NUM] = {0};    // 每个IO的当前原始电平（0=低，1=高）
    uint8_t high_count, low_count;     // 每个IO的高/低电平计数
    uint8_t i, j;                      // 循环变量
    unsigned char SFRPAGE_SAVE = SFRPAGE; // 51单片机SFR页保护（C89兼容）
    uint16_t io_state_bit = 0;		

    // ========== 步骤1：读取原始IO电平（光耦实时状态，含交流波动） ==========
    SFRPAGE = CONFIG_PAGE;  // 切换到配置页（根据硬件调整）
    // 逐个读取I1~I11的原始电平，转换为0（低）/1（高）
    io_level[0] = (I1 == IO_VALID_LEVEL) ? LOW_LEVEL : HIGH_LEVEL;   // I1
    io_level[1] = (I2 == IO_VALID_LEVEL) ? LOW_LEVEL : HIGH_LEVEL;   // I2
    io_level[2] = (I3 == IO_VALID_LEVEL) ? LOW_LEVEL : HIGH_LEVEL;   // I3
    io_level[3] = (I4 == IO_VALID_LEVEL) ? LOW_LEVEL : HIGH_LEVEL;   // I4
    io_level[4] = (I5 == IO_VALID_LEVEL) ? LOW_LEVEL : HIGH_LEVEL;   // I5
    io_level[5] = (I6 == IO_VALID_LEVEL) ? LOW_LEVEL : HIGH_LEVEL;   // I6
    io_level[6] = (I7 == IO_VALID_LEVEL) ? LOW_LEVEL : HIGH_LEVEL;   // I7
    io_level[7] = (I8 == IO_VALID_LEVEL) ? LOW_LEVEL : HIGH_LEVEL;   // I8
    io_level[8] = (I9 == IO_VALID_LEVEL) ? LOW_LEVEL : HIGH_LEVEL;   // I9
    io_level[9] = (I10 == IO_VALID_LEVEL) ? LOW_LEVEL : HIGH_LEVEL;  // I10
    io_level[10] = (I11 == IO_VALID_LEVEL) ? LOW_LEVEL : HIGH_LEVEL; // I11
    SFRPAGE = SFRPAGE_SAVE; // 恢复SFR页（必须，防止后续寄存器操作出错）

    // ========== 步骤2：每个IO独立更新历史缓冲区（环形） ==========
    for (i = 0; i < IO_NUM; i++)
    {
        // 写入当前电平到环形缓冲区（覆盖最旧的记录）
        io_history[i][io_ptr[i]] = io_level[i];
        // 指针循环（0→1→2→3→4→0）
        io_ptr[i] = (io_ptr[i] + 1) % VOTE_WINDOW_SIZE;

        // ========== 步骤3：标记缓冲区是否写满（仅第一次写满时置1） ==========
        if (io_ptr[i] == 0 && io_buf_full[i] == 0)
        {
            io_buf_full[i] = 1; // 指针回到0，说明已写满5次，标记为完成初始化
        }

        // ========== 步骤4：仅缓冲区写满后，才进行多数表决 ==========
        if (io_buf_full[i] == 0)
        {
            continue; // 未写满5次，跳过表决，全局状态保持不变
        }

        // ========== 步骤5：多数表决（统计最近5次中的高/低次数） ==========
        high_count = 0;
        low_count = 0;
        for (j = 0; j < VOTE_WINDOW_SIZE; j++)
        {
            if (io_history[i][j] == HIGH_LEVEL)
                high_count++;
            else
                low_count++;
        }

        // ========== 步骤6：根据表决结果更新全局状态 ==========
        // 全局状态位映射：I1→IO1_STATE(bit0)，I2→IO2_STATE(bit1)...I11→IO11_STATE(bit10)
        io_state_bit = (1 << i);
        if (low_count >= VOTE_THRESHOLD)
        {
            // 多数为低电平（光耦导通，有输入）→ 置位全局状态
            g_all_io_state |= io_state_bit;
        }
        else if (high_count >= VOTE_THRESHOLD)
        {
            // 多数为高电平（光耦断开，无输入）→ 清零全局状态
            g_all_io_state &= ~io_state_bit;
        }
        // 若高/低都不足3次（如2高3低则走low分支，2低3高走high分支；若2高2低1未知，保持原状态）
    }
}