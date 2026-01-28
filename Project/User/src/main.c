

#include "config.h"
#include "menu.h"
#include "system.h"
#include "fm31256.h"
#include "data_deal.h"
#include "modbus.h"

void main() {
	
	  system_int();
    while(1) {


			
			Read_Time_Per_1s();
			change_io(1);	
			Read_All_IO_State();
			change_io(0);
			Data_Get();
			menu_disp();
	
			Frame_Parse_Main();			
			FM31256_WDG_Feed();//喂外狗

    }
}