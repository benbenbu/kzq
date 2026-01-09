#include "menu.h"
#include "adc_key.h"
#include "lcd.h"
#include "data_save.h"


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
  if (key_val == KEY_0_UP)
  {
  }
  if (key_val == KEY_4_UP)
  {
  }
  if (key_val == KEY_5_UP)
  {

    menu_state.id = MAIN_MENU;
    LCD_Clear();
    menu_state.switch_temp_flag = 1;
    return;
  }
  LCD_DisplayString(84, 0, "采样校准", 16, 0);
  LCD_DisplayString(0, 18, "零漂校准:", 16, 0);
  LCD_DisplayString(0, 36, "增益校准:", 16, 0);
  LCD_DisplayString(0, 54, "功率校准:", 16, 0);
  LCD_DisplayString(0, 72, "相位校准:", 16, 0);
  LCD_DisplayString(84, 18, "停止", 16, 1);
  LCD_DisplayString(84, 36, "停止", 16, 0);
  LCD_DisplayString(84, 54, "停止", 16, 0);
  LCD_DisplayString(84, 72, "停止", 16, 0);

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