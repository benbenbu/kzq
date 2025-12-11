

#include "config.h"
#include "lcd.h"
#include "system.h"


void main() {
	
	system_int();
	
	// 运行LCD功能测试
	LCD_Test();
	
    while(1) {
        // 主循环

    }
}