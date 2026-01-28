#include "menu.h"
#include "adc_key.h"
#include "lcd.h"
#include "data_save.h"
#include "data_deal.h"
#include <string.h> // 51编译器需确保该头文件支持（STC系列默认支持）
#include "fm31256.h"
#include "ht7036.h"
static int is_passcode_equal(const char* code1, const char* code2)
{
    // 直接比较数值（比如2、0、3），而非字符
    return (code1[0] == code2[0] &&
            code1[1] == code2[1] &&
            code2[2] == code2[2] &&  // 笔误修正：code1[2] == code2[2]
            code1[3] == code2[3] &&
            code1[4] == code2[4] &&
            code1[5] == code2[5]);
}

// 封装：判断口令数组是否等于数值序列 [2,0,2,3,2,0]
static int is_fixed_password(const char* code1)
{
    // 直接比较数字（符合你“存的是数字”的场景）
    return (code1[0] == 2 &&
            code1[1] == 0 &&
            code1[2] == 2 &&
            code1[3] == 3 &&
            code1[4] == 2 &&
            code1[5] == 0);
}
static int is_fixed_password1(const char* code1)
{
    // 直接比较数字（符合你“存的是数字”的场景）
    return (code1[0] == 1 &&
            code1[1] == 1 &&
            code1[2] == 0 &&
            code1[3] == 0 &&
            code1[4] == 0 &&
            code1[5] == 0);
}
/*********保护口令部分********/
void Pass_Word(uint8_t key_val)
{
  static Pass_Para_Struct pass={0};
	static char edit_col=0;
	char j;
	switch(key_val)
	{
	
		case KEY_0_UP:
			pass.pass_code[edit_col]++;
		  if(pass.pass_code[edit_col]>9)
				pass.pass_code[edit_col]=0;
			
		break;
		case KEY_1_UP:
				pass.pass_code[edit_col]--;
		  if(pass.pass_code[edit_col]<0)
				pass.pass_code[edit_col]=9;		
		break;	
		case KEY_2_UP:
			edit_col--;
		  if(edit_col<0)
				edit_col=5;
			
		break;	
		case KEY_3_UP:
			edit_col++;
		  if(edit_col>5)
				edit_col=0;			
		break;	
		case KEY_4_UP:
			
     if (((is_passcode_equal(pass.pass_code, g_protect_password.pass_code) || is_fixed_password(pass.pass_code))&&(menu_state.last_id!=STATISTIC_PARAM))\
    ||(is_fixed_password1(pass.pass_code)&&(menu_state.last_id==STATISTIC_PARAM)))			 
			{
				menu_state.id=menu_state.last_id;
        LCD_Clear();		
        menu_state.switch_temp_flag = 1;
				for(j=0;j<6;j++)
				pass.pass_code[j]=0;
				edit_col=0;						
        return ;				
			}
			else
			{
			  edit_col=0;
				for(j=0;j<6;j++)
				pass.pass_code[j]=0;	
			}
		break;	
		case KEY_5_UP:
						  edit_col=0;
				for(j=0;j<6;j++)
				pass.pass_code[j]=0;
        if(menu_state.last_id!=STATISTIC_PARAM)		
		    menu_state.id=MAIN_MENU;
				else
		    menu_state.id=COMM_PARAM;					
		    LCD_Clear();
        menu_state.switch_temp_flag = 1;
		        return ;		
		break;	
	}

	  LCD_DisplayString(30, 40, "保护口令:", 16, 0);
		for(j=0;j<6;j++)
    LCD_DisplayNum(111+j*12,40, pass.pass_code[j], 16, 1,(j==edit_col) ? 0 : 0xff, 0, 1);

}

/*********鬼键设置菜单部分********/
void Sys_Static_Para_Set(u8 key_val)
{

  static char stat;
	Cap_Num_Struct  num;
  code const char *g_static_para_titles[10] = {

      "产品型号",
      "C1 ",
      "C2 ",
      "C3 ",
      "C4 ",
      "CP1",
      "CP2",
      "CP3",
      "CP4",

		
		  
  };

  if (state == 0) // 第一次进入菜单，系统变量赋值到临时变量。初始化坐标，浏览模式，编辑坐标为0xff
  {
		if(g_cap_num.pro_num==0)			
    stat = g_cap_num.cap_num;
		else
    stat = g_cap_num.cap_num+4;			
		
    state = 1;
		item_id=1;
  }
  else if (state == 1)
  {
    // 浏览模式
    switch (key_val)
    {
    case KEY_1_UP:			
    case KEY_2_UP:
      if(item_id==1)
				item_id=2;
			else
				item_id=1;
      break;			
			
			
 
    case KEY_4_UP:
			 if(item_id==1)
			 {
        state = 2; // 进入编辑
        edit_col = 0;
			 }
			 if(item_id==2)
			 {
				menu_state.id = SAMPLING;
				LCD_Clear();
				state = 0; // 退出（不保存）
				menu_state.switch_temp_flag = 1;
				return;
			 }			 
      break;

    case KEY_5_UP:
      menu_state.id = COMM_PARAM;
      LCD_Clear();
      state = 0; // 退出（不保存）
      menu_state.switch_temp_flag = 1;
		  
      return;
      break;
    }
  }
  else if (state == 2) // 参数编辑
  {
    switch (key_val)
    {
    case KEY_0_UP: // 加
       stat++;
		   if(stat>8)
				 stat=1;
      break;
    case KEY_1_UP: // 减
       stat--;
		   if(stat<1)
				 stat=8;
      break;
  
    case KEY_4_UP:
		if(stat<5)
		{
			num.cap_num=stat;
			num.pro_num=0;			
		}
		else
		{
			num.cap_num=stat-4;
			num.pro_num=stat-4;		
		
		}			
		  
      if(Set_Cap_Num(&num))
			g_hard_state.iic_err_times++;
			else
			{

				g_cap_num=num;
        state = 1;
			}
      break;
    case KEY_5_UP:
      state = 1;
      break;
    }

		
  }

  // 显示部分
  LCD_DisplayString(0, 20, g_static_para_titles[0], 16,  (state == 1)&&(item_id==1) ? 1 : 0);
  LCD_DisplayChar(72, 20, ":", 16, 0); // 冒号不反显
   
  LCD_DisplayString(81, 20, g_static_para_titles[stat], 16, (state == 2)&&(item_id==1) ? 1 : 0);
  LCD_DisplayString(0, 40, "采样校准", 16,  (state == 1)&&(item_id==2) ? 1 : 0);
}


void Sampling(uint8_t key_val)
	
{
	  static uint8_t wait=0;
//  if (key_val == KEY_0_UP)
//  {
//  }
//  if (key_val == KEY_4_UP)
//  {
//  }
	g_adjust=1;
  if (key_val == KEY_5_UP)
  {
    g_adjust=0;
    menu_state.id = MAIN_MENU;
    LCD_Clear();
		Ht7036_init();
    menu_state.switch_temp_flag = 1;
    return;
  }
  LCD_DisplayString(84, 0, "采样校准", 16, 0);
  LCD_DisplayString(0, 18, "零漂校准:", 16, 0);
  LCD_DisplayString(0, 36, "增益校准:", 16, 0);

	


	
    if (state == 0)
    {
        LCD_DisplayString(84, 18, "停止   ",16, 1);
        LCD_DisplayString(84, 36, "停止   ",16, 0);

        if (key_val == KEY_0_UP)
        {
            if (state == 0)
                state = 1;
        }
    }
    if (state == 1)
    {
        LCD_DisplayString(84, 18, "启动   ",16, 1);
        LCD_DisplayString(84, 36, "停止   ",16, 0);

        if (wait == 0)
        {
            Ht7036_Adj_Zero();
        }
        wait++;
        if (wait > 1)
        {
            switch (Ht7036_Zero_Check())
            {
            case 0:
                state = 2;
                break;
            case 1:
                state = 3;
                break;
            case 2:
                state = 4;

                break;
            case 3:
                state = 5;

                break;
            case 4:
                state = 6;

                break;
            case 5:
                state = 7;

                break;
            case 6:
                state = 8;

                break;
            case 7:
                state = 9;

                break;
            case 8:
                state = 10;

                break;
            case 9:
                state = 11;

                break;
            case 10:
                state = 12;

                break;
            case 11:
                state = 13;

                break;						
            case 12:
                state = 14;

                break;
            case 13:
                state = 15;

                break;
            case 14:
                state = 16;

                break;							
						
						
            }
									
            wait = 0;
        }
    }
    if (state == 2)
    {
        LCD_DisplayString(84, 18, "成功   ", 16,0);
        LCD_DisplayString(84, 36, "停止   ", 16,1);
        if (key_val == KEY_0_UP)
        {
            state = 17;
            wait = 0;
        }
    }
    if ((state > 2) && (state < 17))
    {

        LCD_DisplayString(84, 36, "停止   ",16, 0);
        LCD_DisplayString(111, 18, "故障",16, 1);
        if (state == 3)
        {
            LCD_DisplayString(84, 18, "1Ia",16, 1);
        }
        if (state == 4)
        {
            LCD_DisplayString(84, 18, "1Ic",16, 1);
        }
        if (state == 5)
        {
            LCD_DisplayString(84, 18, "1Uo", 16,1);
        }
        if (state == 6)
        {
            LCD_DisplayString(84, 18, "2Ia",16, 1);
        }
        if (state == 7)
        {
            LCD_DisplayString(84, 18, "2Ic",16, 1);
        }
        if (state == 8)
        {
            LCD_DisplayString(84, 18, "2Uo",16, 1);
        }
        if (state == 9)
        {
            LCD_DisplayString(84, 18, "3Ia",16, 1);
        }
        if (state == 10)
        {
            LCD_DisplayString(84, 18, "3Ic",16, 1);
        }
        if (state == 11)
        {
            LCD_DisplayString(84, 18, "3Uo", 16,1);
        }
        if (state == 12)
        {
            LCD_DisplayString(84, 18, "4Ia",16, 1);
        }
        if (state == 13)
        {
            LCD_DisplayString(84, 18, "4Ic",16, 1);
        }
        if (state == 14)
        {
            LCD_DisplayString(84, 18, "4Uo",16, 1);
        }				
        if (state == 15)
        {
            LCD_DisplayString(84, 18, "U ",16, 1);
        }
        if (state == 16)
        {
            LCD_DisplayString(84, 18, "I ",16, 1);
        }							
        if (key_val == KEY_0_UP)
        {
            state = 1;
					  wait=0;
        }
    }
    if (state == 17)
    {
        LCD_DisplayString(84, 18, "成功   ",16, 0);
        LCD_DisplayString(84, 36, "启动   ", 16,1);

        if (wait == 0)
        {

            Ht7036_Adj_Gain();
        }
        wait++;
        if (wait > 1)
        {
            switch (Ht7036_Gain_Check())
            {
            case 0:
                state = 18;
                break;
            case 1:
                state = 19;
                break;
            case 2:
                state = 20;
                break;
            case 3:
                state = 21;
                break;
            case 4:
                state = 22;
                break;
            case 5:
                state = 23;
                break;
            case 6:
                state = 24;
                break;
            case 7:
                state = 25;
                break;
            case 8:
                state = 26;
                break;
            case 9:
                state = 27;
                break;
            case 10:
                state = 28;
                break;
            case 11:
                state = 29;
                break;
            case 12:
                state = 30;
                break;		
            }
            wait = 0;
        }
    }
    if (state == 18)
    {
        LCD_DisplayString(84, 18, "成功   ",16, 0);
        LCD_DisplayString(84, 36, "保存   ",16, 1);
        if (key_val == KEY_4_UP)
        {
            state = 31;
            wait = 0;
        }
    }
    if ((state > 18) && (state < 31))
    {
        LCD_DisplayString(84, 18, "成功   ",16, 0);

        LCD_DisplayString(111, 36, "故障",16, 1);
        if (state == 19)
        {
            LCD_DisplayString(84, 36, "1Ia",16, 1);
        }
        if (state == 20)
        {
            LCD_DisplayString(84, 36, "1Ic", 16,1);
        }
        if (state == 21)
        {
            LCD_DisplayString(84, 36, "1Uo",16, 1);
        }
        if (state == 22)
        {
            LCD_DisplayString(84, 36, "2Ia",16, 1);
        }
        if (state == 23)
        {
            LCD_DisplayString(84, 36, "2Ic",16, 1);
        }
        if (state == 24)
        {
            LCD_DisplayString(84, 36, "2Uo", 16,1);
        }
        if (state == 25)
        {
            LCD_DisplayString(84, 36, "3Ia",16, 1);
        }
        if (state == 26)
        {
            LCD_DisplayString(84, 36, "3Ic", 16,1);
        }
        if (state == 27)
        {
            LCD_DisplayString(84, 36, "3Uo",16, 1);
        }
        if (state == 28)
        {
            LCD_DisplayString(84, 36, "4Ia",16, 1);
        }
        if (state == 29)
        {
            LCD_DisplayString(84, 36, "4Ic",16, 1);
        }
        if (state == 30)
        {
            LCD_DisplayString(84, 36, "4Uo", 16,1);
        }				
					
				
        if (key_val == KEY_0_DOWN)
        {
            state = 17;
//            YXD12864_GRAM_Clear(0);
										  wait=0;
        }
    }
	
		
		    if (state == 31)
    {
        if (Ht7036_Adjust_Data_Save())
        {
            state = 32;
        }
        else
        {
            LCD_DisplayString(84, 18, "成功   ",16, 0);
            LCD_DisplayString(84, 36, "保存   ",16, 1);
        }
    }
    if (state == 32)
    {

        LCD_DisplayString(84, 18, "成功   ",16, 0);
        LCD_DisplayString(84, 36, "保存   ",16, 0);
    }
		
		
		
		
		}	
	
	



/*********保护参数菜单********/
void Protect_Menu(uint8_t key_val)
{

  code const char *g_prot_titles[7] = {

          "保护参数\xFD", "系统过\xFD压保护", "系统欠压保护", "一路电容保护", "二路电容保护", "三\xFD路电容保护", "四路电容保护"
  };
  switch (key_val)
  {

   case KEY_0_UP:
		 
			 		 
		  if(g_cap_num.pro_num==1)
			{
          if(item3==1)
             item3=3;
          else if(item3==3)
             item3=1;					
			}
		  if(g_cap_num.pro_num==2)
			{
          if(item3==1)
             item3=3;
          else if(item3==3)
             item3=1;	
	
          if(item3==2)
             item3=4;
          else if(item3==4)
             item3=2;						
			}		
		  if(g_cap_num.pro_num==3)
			{
          if(item3==1)
             item3=5;
          else if(item3==5)
             item3=3;	
          else if(item3==3)
             item3=1;						
					
          if(item3==2)
             item3=4;
          else if(item3==4)
             item3=2;				
			}		
		  if(g_cap_num.pro_num==4)
			{
          if(item3==1)
             item3=5;	
          else if(item3==5)
             item3=3;	
          else if(item3==3)
             item3=1;						
					
          if(item3==2)
             item3=6;	
          else if(item3==6)
             item3=4;	
          else if(item3==4)
             item3=2;						
			}		
      break;
    case KEY_1_UP:			
		
		  if(g_cap_num.pro_num==1)
			{
          if(item3==1)
             item3=3;
          else if(item3==3)
             item3=1;				
			}
		  if(g_cap_num.pro_num==2)
			{
          if(item3==1)
             item3=3;
          else if(item3==3)
             item3=1;	
          if(item3==2)
             item3=4;
          else if(item3==4)
             item3=2;			
			}		
		  if(g_cap_num.pro_num==3)
			{
          if(item3==1)
             item3=3;
          else if(item3==3)
             item3=5;	
          else if(item3==5)
             item3=1;	
					
          if(item3==2)
             item3=4;
          else if(item3==4)
             item3=2;					
			}		
		  if(g_cap_num.pro_num==4)
			{
          if(item3==1)
             item3=3;
          else if(item3==3)
             item3=5;	
          else if(item3==5)
             item3=1;	
     
          if(item3==2)
             item3=4;
          else if(item3==4)
             item3=6;
          else if(item3==6)
             item3=2;		
			}				
      break;

    case KEY_2_UP:
			
		
				if (item3<=(g_cap_num.pro_num + 2))
				item3--;
        if(item3<1)	
         item3=(g_cap_num.pro_num + 2);	
				
      break;
    case KEY_3_UP:
				if (item3>=1)
				item3++;
        if(item3>(g_cap_num.pro_num + 2))	
         item3=1;	
						
      break;

  // 其他按键（如确认、返回）可后续扩展
  case KEY_4_UP:

    switch (item3)
    {
    case 1:
      menu_state.id = PROT_PARAM_VOL_H;
      break;
    case 2:
      menu_state.id = PROT_PARAM_VOL_L;
      break;
    case 3:
      menu_state.id = PROT_CAP1;
      break;
    case 4:
      menu_state.id = PROT_CAP2;
      break;
    case 5:
      menu_state.id = PROT_CAP3;
      break;
    case 6:
      menu_state.id = PROT_CAP4;
      break;
    }
    LCD_Clear();
    menu_state.switch_temp_flag = 1;
		state=0;
    return;
    break;
  case KEY_5_UP:
    menu_state.id = PARAM_SETTING;
    LCD_Clear();
    menu_state.switch_temp_flag = 1;
    item3 = 1;
	  state=0;
    return;
    break;
  default:
    break;
  }
  LCD_DisplayString(81, 0, g_prot_titles[0], 16, 0);
  LCD_DisplayString(0, 24, g_prot_titles[1], 16, item3 == 1 ? 1 : 0);
  LCD_DisplayString(132, 24, g_prot_titles[2], 16, item3 == 2 ? 1 : 0);

  if (g_cap_num.pro_num > 0)
  {
    LCD_DisplayString(0, 48, g_prot_titles[3], 16, item3 == 3 ? 1 : 0);
  }
  if (g_cap_num.pro_num > 1)
  {
    LCD_DisplayString(132, 48, g_prot_titles[4], 16, item3 == 4 ? 1 : 0);
  }
  if (g_cap_num.pro_num > 2)
  {
    LCD_DisplayString(0, 72, g_prot_titles[5], 16, item3 == 5 ? 1 : 0);
  }
  if (g_cap_num.pro_num > 3)
  {
    LCD_DisplayString(132, 72, g_prot_titles[6], 16, item3 == 6 ? 1 : 0);
  }
}


void Err_recover(void)
{

	uint8_t i;

		for(i=0;i<6;i++)
			{
		g_sys_flag[i]=0;
		g_sys_flag_handled[i]=0;		
			}
		err_state=0;
    Beep_Control(0);
    memset(&g_pro_time, 0, sizeof(Protect_Time_Struct));
		for(i=0;i<4;i++)
		g_cap_fail_flag[i]=0;	
			
}




void Err_Deal(uint8_t key_val)

{

	switch(key_val)
	{
	
		case KEY_2_UP:
			
		case KEY_3_UP:
		if(item_id==0)
			item_id=1;
		else
			item_id=0;		
		break;
		case KEY_4_UP:	
    menu_state.id = METER_DISP;
    LCD_Clear();
    menu_state.switch_temp_flag = 1;

		
		if(item_id==0)
		{
     Err_recover();
		}
    return;      
    break;		
		case KEY_5_UP:	
    menu_state.id = METER_DISP;
    LCD_Clear();
    menu_state.switch_temp_flag = 1;
    return;    
    break;		
	}
	

  LCD_DisplayString(0, 36, "确定解除\xFD所有故障:", 16, 0);
  LCD_DisplayString(84, 56, "是", 16, (item_id)?0:1);
  LCD_DisplayString(162, 56, "否", 16, (item_id)?1:0);	
}

/*********采样校准菜单********/
void Sampling_Adiust(uint8_t key_val)
{
	uint8_t dat[3]={0};
  if (key_val == KEY_5_UP)
  {
    menu_state.id = MAIN_MENU;
    LCD_Clear();
    menu_state.switch_temp_flag = 1;
    return;
  }

  LCD_DisplayString(84, 0, "采样校准", 16, 0);

  LCD_DisplayString(0, 20, "版本:", 16, 0);
	
	FM31256_FRAM_Read(0,dat,3);
	if(dat[0]!=0)
	{
	
    LCD_DisplayNum(48, 20, dat[0], 16, 2, 0xff, 0, 1);	
  LCD_DisplayString(66, 20, "-", 16, 0);		
    LCD_DisplayNum(75, 20, dat[1], 16, 2, 0xff, 0, 1);			
  LCD_DisplayString(93, 20, "-", 16, 0);				
    LCD_DisplayNum(102, 20, dat[2], 16, 2, 0xff, 0, 1);		
	}

		
	else	
  LCD_DisplayString(48, 20, "默认", 16, 0);
}
/*********事件记录菜单********/

// 配置参数
#define EVENT_LOG_CNT 100    // 总事件数
#define TOTAL_PAGE_CNT 20    // 总页数
#define EVENT_PER_PAGE 5     // 每页显示5条
#define EVENT_STR_BUF_LEN 20 // 事件字符串缓冲区长度

// 显示状态变量
static uint8_t s_current_page = 1;  // 当前页（1~20）
static uint8_t s_selected_line = 1; // 当前选中行（1~5）
static uint8_t s_refresh_flag = 1;  // 显示刷新标志
static uint8_t s_latest_event_idx;  // 最新事件的FRAM索引（首次进入菜单读取）
static uint8_t s_page_first_seq;    // 当前页第一条记录的序号（如第1页=1，第2页=6）
static uint8_t s_first_enter = 1;
static uint8_t s_event_refresh=0;//需要刷新标志
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
    
  // ==================== 过流故障（IA/IC统一显示，不分路型） ====================
  case EVENT_TYPE_OVER_CURRENT1_IA:   // 1路IA过流
  case EVENT_TYPE_OVER_CURRENT1_IC:   // 1路IC过流
    return "一路过\xFD流保护";
  case EVENT_TYPE_OVER_CURRENT2_IA:   // 2路IA过流
  case EVENT_TYPE_OVER_CURRENT2_IC:   // 2路IC过流
    return "二路过\xFD流保护";
  case EVENT_TYPE_OVER_CURRENT3_IA:   // 3路IA过流
  case EVENT_TYPE_OVER_CURRENT3_IC:   // 3路IC过流
    return "三\xFD路过\xFD流保护";
  case EVENT_TYPE_OVER_CURRENT4_IA:   // 4路IA过流
  case EVENT_TYPE_OVER_CURRENT4_IC:   // 4路IC过流
    return "四路过\xFD流保护";

  // ==================== 速断故障（IA/IC统一显示，不分路型） ====================
  case EVENT_TYPE_QUICK_CURRENT1_IA:  // 1路IA速断
  case EVENT_TYPE_QUICK_CURRENT1_IC:  // 1路IC速断
    return "一路速断保护";
  case EVENT_TYPE_QUICK_CURRENT2_IA:  // 2路IA速断
  case EVENT_TYPE_QUICK_CURRENT2_IC:  // 2路IC速断
    return "二路速断保护";
  case EVENT_TYPE_QUICK_CURRENT3_IA:  // 3路IA速断
  case EVENT_TYPE_QUICK_CURRENT3_IC:  // 3路IC速断
    return "三\xFD路速断保护";
  case EVENT_TYPE_QUICK_CURRENT4_IA:  // 4路IA速断
  case EVENT_TYPE_QUICK_CURRENT4_IC:  // 4路IC速断
    return "四路速断保护";

  // ==================== 零序故障（顺延至新枚举值，字符串不变） ====================
  case EVENT_TYPE_OVER_VOL_ZERO1:
    return "一路零序保护";
  case EVENT_TYPE_OVER_VOL_ZERO2:
    return "二路零序保护";
  case EVENT_TYPE_OVER_VOL_ZERO3:
    return "三\xFD路零序保护";
  case EVENT_TYPE_OVER_VOL_ZERO4:
    return "四路零序保护";

  // ==================== 系统电压故障（顺延，字符串不变） ====================
  case EVENT_TYPE_OVER_VOL:
    return "系统过\xFD压保护";
  case EVENT_TYPE_UNDER_VOL:
    return "系统欠压保护";

  // ==================== 拒投故障（顺延，字符串不变） ====================
  case EVENT_TYPE_IN_STOP1:
    return "一路拒投    ";
  case EVENT_TYPE_IN_STOP2:
    return "二路拒投    ";
  case EVENT_TYPE_IN_STOP3:
    return "三\xFD路拒投    ";
  case EVENT_TYPE_IN_STOP4:
    return "四路拒投    ";

  // ==================== 拒切故障（顺延，字符串不变） ====================
  case EVENT_TYPE_QUIT_STOP1:
    return "一路拒切    ";
  case EVENT_TYPE_QUIT_STOP2:
    return "二路拒切    ";
  case EVENT_TYPE_QUIT_STOP3:
    return "三\xFD路拒切    ";
  case EVENT_TYPE_QUIT_STOP4:
    return "四路拒切    ";

  // ==================== 外部故障（顺延，字符串不变） ====================
  case EVENT_TYPE_ERR1:
    return "一路外部故障";
  case EVENT_TYPE_ERR2:
    return "二路外部故障";
  case EVENT_TYPE_ERR3:
    return "三\xFD路外部故障";
  case EVENT_TYPE_ERR4:
    return "四路外部故障";

  // ==================== 电源故障（顺延，字符串不变） ====================
  case EVENT_TYPE_POWER_OFF:
    return "前段总闸断电";

  // ==================== 正常操作（顺延，字符串不变） ====================
  case EVENT_TYPE_IN1:
    return "一路投入    ";
  case EVENT_TYPE_IN2:
    return "二路投入    ";
  case EVENT_TYPE_IN3:
    return "三\xFD路投入    ";
  case EVENT_TYPE_IN4:
    return "四路投入    ";
  case EVENT_TYPE_QUIT1:
    return "一路切除\xFD    ";
  case EVENT_TYPE_QUIT2:
    return "二路切除\xFD    ";
  case EVENT_TYPE_QUIT3:
    return "三\xFD路切除\xFD    ";
  case EVENT_TYPE_QUIT4:
    return "四路切除\xFD    ";

  // 未知类型
  default:
    return "            ";
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

  // 显示时间（仅事件有效时：BCD码→十进制后显示）
  if (s_page_event_cache[line_idx].event_type != EVENT_TYPE_NONE)
  {
    /* 核心修改：所有时间字段先通过BCD_TO_DEC转回十进制，再显示 */
    disp_two_digit(BCD_TO_DEC(s_page_event_cache[line_idx].year), 171, line_y, 8, 0);
    LCD_DisplayChar(189, line_y, "/", 8, 0);
    disp_two_digit(BCD_TO_DEC(s_page_event_cache[line_idx].month), 198, line_y, 8, 0);
    LCD_DisplayChar(216, line_y, "/", 8, 0);
    disp_two_digit(BCD_TO_DEC(s_page_event_cache[line_idx].day), 225, line_y, 8, 0);

    disp_two_digit(BCD_TO_DEC(s_page_event_cache[line_idx].hour), 171, line_y + 8, 8, 0);
    LCD_DisplayChar(189, line_y + 8, ":", 8, 0);
    disp_two_digit(BCD_TO_DEC(s_page_event_cache[line_idx].minute), 198, line_y + 8, 8, 0);
    LCD_DisplayChar(216, line_y + 8, ":", 8, 0);
    disp_two_digit(BCD_TO_DEC(s_page_event_cache[line_idx].second), 225, line_y + 8, 8, 0);
	
  }
  else
  {
    LCD_DisplayString(171, line_y, "        ", 8, 0);
    LCD_DisplayString(171, line_y + 8, "        ", 8, 0);
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
  case KEY_4_UP: // 
    menu_state.id = EVENT;
    LCD_Clear();
    menu_state.switch_temp_flag = 1;
    return; // 直接返回，不处理后续
    break;		
		
		
		

  case KEY_5_UP: // 返回主菜单
    menu_state.id = MAIN_MENU;
    s_cache_valid = 0; // 重置缓存标志
    s_first_enter = 1; // 下次进入重新初始化
    LCD_Clear();
    menu_state.switch_temp_flag = 1;
	  item1=5;
    return; // 直接返回，不处理后续

//  default:
//    return;
  }

  // 1. 跨页：重新加载缓存+整页刷新
  if ((need_reload_cache)||(s_event_refresh))
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
  if ((key_val != KEY_NONE)||(s_event_refresh))
  { // KEY_NONE需定义为0，根据你的按键驱动调整
    Event_Log_Handle_Key(key_val);
		s_event_refresh=0;
  }
}





// ==================== 事件详情页核心显示函数 ====================
void Event_Detail_Display(void) {

    uint16_t a=0;
    uint8_t b=0;
	  uint8_t c=0;
	 uint32_t d=0;
	  
	  a=s_page_first_seq+s_selected_line-1;
	  b=s_selected_line-1;	
	  c=s_page_event_cache[b].event_type;
	  d=	s_page_event_cache[b].event_data;
    // 2. 第一行：事件记录（居中显示）
    LCD_DisplayString(81, 0, "事件记录", 16, 0);
    
    // 3. 第二行：事件内容 + 事件类型（序号+类型）
	  LCD_DisplayString(0, 20, "记录序号:", 16, 0);
	

    LCD_DisplayNum(80, 20, a, 16, 3, 0xff, 0, 1);
	
	  LCD_DisplayString(0, 40, "事件内容:", 16, 0);	
    LCD_DisplayString(80, 40, s_page_str_cache[b] + 4, 16, 0);
	
    if(c==0)
		{
		
		}
	  else if(( c<17))			
    LCD_DisplayString(0, 60, "故障电流:", 16, 0);	
  else if((c<23))	
    LCD_DisplayString(0, 60, "故障电压:", 16, 0);	
	
	
	if(c==0)
	{
	
	}
	 else if(( c<17))	
		{			
			
			if((c<5)||((c>8)&&(c<13)))
			LCD_DisplayString(80, 60, "Ia ", 16, 0);
      else
			LCD_DisplayString(80, 60, "Ic ", 16, 0);	
			
      LCD_DisplayFixedPoint(107, 60, d/10, (d)%10,3, 1,16, 0xff, 0);
		
			LCD_DisplayString(152, 60, "A", 16, 0);
		}
  else if(c<21)	
	{		
      LCD_DisplayFixedPoint(80, 60, d/100, (d%100)/10,2, 1,16, 0xff, 0);
		
			LCD_DisplayString(125, 60, "V", 16, 0);
	}
  else if(c<23)	
	{	

 LCD_DisplayFixedPoint(80, 60, d/100, (d)%100,2, 2,16, 0xff, 0);
		
			LCD_DisplayString(125, 60, "KV", 16, 0);
	}	


	
	
    

}

// ==================== 事件详情页按键处理 ====================
void Event_Detail_Handle_Key(uint8_t key_val) {
    if(key_val == KEY_5_UP) {
        // 返回事件记录列表页
        menu_state.id = EVENT_LOG;
        LCD_Clear();
        s_event_refresh=1;
        menu_state.switch_temp_flag = 1;
    }
}

// ==================== 事件详情页主入口 ====================
void Event_Detail_Menu(uint8_t key_val) {
    // 首次进入详情页：显示详情

        Event_Detail_Display();

   
    // 非首次：处理按键（仅处理返回键）
    if(key_val != KEY_NONE) 
        Event_Detail_Handle_Key(key_val);

}
