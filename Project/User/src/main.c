

#include "config.h"
#include "menu.h"
#include "system.h"
#include "fm31256.h"


void main() {
	
	  system_int();
    while(1) {


			
			Read_Time_Per_1s();
			menu_disp();
			
			FM31256_WDG_Feed();//喂外狗

    }
}