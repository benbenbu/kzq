#include "time.h"



// 定时器配置数组
timer_config_t timer_configs[5] = {0};

// 定时器初始化函数
void Timer_Init() {
    // 初始化所有定时器配置
    int i;
    for(i = 0; i < 5; i++) {
        timer_configs[i].state = TIMER_DISABLED;
        timer_configs[i].period_ms = 0;
        timer_configs[i].counter = 0;
        timer_configs[i].target_count = 0;
        timer_configs[i].callback_enabled = 0;
        timer_configs[i].callback = 0;
    }
    
    // 配置定时器0为16位模式，使用系统时钟/12
    TMOD &= ~0x03;  // 清除定时器0模式位
    TMOD |= 0x01;   // 设置定时器0为模式1（16位定时器）
    
    // 配置定时器1为16位模式，使用系统时钟/12
    TMOD &= ~0x30;  // 清除定时器1模式位
    TMOD |= 0x10;   // 设置定时器1为模式1（16位定时器）
    
    // 配置定时器2为16位自动重载模式
    TMR2CN = 0x00;  // 停止定时器2，清标志位
    TMR2CF = 0x08;  // 设置时钟源为系统时钟/12
    RCAP2H = 0xFF;  // 设置重载值（高字节）
    RCAP2L = 0x80;  // 设置重载值（低字节），约为1ms中断
    TMR2H = 0xFF;   // 设置初始值（高字节）
    TMR2L = 0x80;   // 设置初始值（低字节）
    
    // 配置定时器3为16位自动重载模式
    TMR3CN = 0x00;  // 停止定时器3，清标志位
    TMR3CF = 0x08;  // 设置时钟源为系统时钟/12
    RCAP3H = 0xFF;  // 设置重载值（高字节）
    RCAP3L = 0x80;  // 设置重载值（低字节）
    TMR3H = 0xFF;   // 设置初始值（高字节）
    TMR3L = 0x80;   // 设置初始值（低字节）
    
    // 配置定时器4为16位自动重载模式
    TMR4CN = 0x00;  // 停止定时器4，清标志位
    TMR4CF = 0x08;  // 设置时钟源为系统时钟/12
    RCAP4H = 0xFF;  // 设置重载值（高字节）
    RCAP4L = 0x80;  // 设置重载值（低字节）
    TMR4H = 0xFF;   // 设置初始值（高字节）
    TMR4L = 0x80;   // 设置初始值（低字节）
    
    // 系统时钟初始化
    OSCICN |= 0x80;     // 使能内部高频率振荡器
    for(i = 0; i < 1000; i++); // 等待振荡器稳定
    CLKSEL = 0x00;      // 选择内部振荡器，不分频
}

// 定时器0配置函数（使用MAIN_Fosc宏）
void Timer0_Config(uint16_t period_ms, void (*callback)(void)) {
    // 计算重载值：基于MAIN_Fosc时钟频率
    // 定时器时钟频率 = MAIN_Fosc / 12
    // 每个计数周期时间 = 12 / MAIN_Fosc 秒
    // 溢出时间 = (65536 - reload_value) * 12 / MAIN_Fosc 秒
    // reload_value = 65536 - (period_ms * MAIN_Fosc) / (12 * 1000)
    
    uint32_t temp = (uint32_t)period_ms * MAIN_Fosc;
    uint16_t reload_value = 65536 - (temp / (12 * 1000));
    
    TH0 = (reload_value >> 8) & 0xFF;
    TL0 = reload_value & 0xFF;
    
    // 更新配置
    timer_configs[0].state = TIMER_ENABLED;
    timer_configs[0].period_ms = period_ms;
    timer_configs[0].target_count = period_ms;
    timer_configs[0].callback_enabled = (callback != 0);
    timer_configs[0].callback = callback;
    
    // 启动定时器0
    TR0 = 1;
    
    // 使能定时器0中断
    ET0 = 1;
}

// 定时器1配置函数（使用MAIN_Fosc宏）
void Timer1_Config(uint16_t period_ms, void (*callback)(void)) {
    // 计算重载值：基于MAIN_Fosc时钟频率
    uint32_t temp = (uint32_t)period_ms * MAIN_Fosc;
    uint16_t reload_value = 65536 - (temp / (12 * 1000));
    
    TH1 = (reload_value >> 8) & 0xFF;
    TL1 = reload_value & 0xFF;
    
    // 更新配置
    timer_configs[1].state = TIMER_ENABLED;
    timer_configs[1].period_ms = period_ms;
    timer_configs[1].target_count = period_ms;
    timer_configs[1].callback_enabled = (callback != 0);
    timer_configs[1].callback = callback;
    
    // 启动定时器1
    TR1 = 1;
    
    // 使能定时器1中断
    ET1 = 1;
}

// 定时器2配置函数（使用MAIN_Fosc宏）
void Timer2_Config(uint16_t period_ms, void (*callback)(void)) {
    // 计算重载值：基于MAIN_Fosc时钟频率，使用系统时钟/12
    uint32_t temp = (uint32_t)period_ms * MAIN_Fosc;
    uint16_t reload_value = 65536 - (temp / (12 * 1000));
    
    RCAP2H = (reload_value >> 8) & 0xFF;
    RCAP2L = reload_value & 0xFF;
    TMR2H = (reload_value >> 8) & 0xFF;
    TMR2L = reload_value & 0xFF;
    
    // 更新配置
    timer_configs[2].state = TIMER_ENABLED;
    timer_configs[2].period_ms = period_ms;
    timer_configs[2].target_count = period_ms;
    timer_configs[2].callback_enabled = (callback != 0);
    timer_configs[2].callback = callback;
    
    // 启动定时器2
    TR2 = 1;
    
    // 使能定时器2中断
    ET2 = 1;
}

// 定时器3配置函数（使用MAIN_Fosc宏）
void Timer3_Config(uint16_t period_ms, void (*callback)(void)) {
    // 计算重载值：基于MAIN_Fosc时钟频率，使用系统时钟/12
    uint32_t temp = (uint32_t)period_ms * MAIN_Fosc;
    uint16_t reload_value = 65536 - (temp / (12 * 1000));
    
    RCAP3H = (reload_value >> 8) & 0xFF;
    RCAP3L = reload_value & 0xFF;
    TMR3H = (reload_value >> 8) & 0xFF;
    TMR3L = reload_value & 0xFF;
    
    // 更新配置
    timer_configs[3].state = TIMER_ENABLED;
    timer_configs[3].period_ms = period_ms;
    timer_configs[3].target_count = period_ms;
    timer_configs[3].callback_enabled = (callback != 0);
    timer_configs[3].callback = callback;
    
    // 启动定时器3
    TR3 = 1;
    
    // 使能定时器3中断
//    ET3 = 1;         
		EIE2|=0X01;
}

// 定时器4配置函数（使用MAIN_Fosc宏）
void Timer4_Config(uint16_t period_ms, void (*callback)(void)) {
    // 计算重载值：基于MAIN_Fosc时钟频率，使用系统时钟/12
    uint32_t temp = (uint32_t)period_ms * MAIN_Fosc;
    uint16_t reload_value = 65536 - (temp / (12 * 1000));
    
    RCAP4H = (reload_value >> 8) & 0xFF;
    RCAP4L = reload_value & 0xFF;
    TMR4H = (reload_value >> 8) & 0xFF;
    TMR4L = reload_value & 0xFF;
    
    // 更新配置
    timer_configs[4].state = TIMER_ENABLED;
    timer_configs[4].period_ms = period_ms;
    timer_configs[4].target_count = period_ms;
    timer_configs[4].callback_enabled = (callback != 0);
    timer_configs[4].callback = callback;
    
    // 启动定时器4
    TR4 = 1;
    
    // 使能定时器4中断
//    ET4 = 1;
		EIE2|=0X04;		
}

// 定时器0中断服务程序
void Timer0_ISR() interrupt 1 {
    // 重新加载定时器值
    uint32_t temp = (uint32_t)timer_configs[0].period_ms * MAIN_Fosc;
    uint16_t reload_value = 65536 - (temp / (12 * 1000));
    TH0 = (reload_value >> 8) & 0xFF;
    TL0 = reload_value & 0xFF;
    
    // 增加计数器
    timer_configs[0].counter++;
    
    // 检查是否达到目标计数值
    if(timer_configs[0].counter >= timer_configs[0].target_count) {
        timer_configs[0].counter = 0; // 重置计数器
        
        // 执行回调函数
        if(timer_configs[0].callback_enabled && timer_configs[0].callback) {
            timer_configs[0].callback();
        }
    }
}

// 定时器1中断服务程序
void Timer1_ISR() interrupt 3 {
    // 重新加载定时器值
    uint32_t temp = (uint32_t)timer_configs[1].period_ms * MAIN_Fosc;
    uint16_t reload_value = 65536 - (temp / (12 * 1000));
    TH1 = (reload_value >> 8) & 0xFF;
    TL1 = reload_value & 0xFF;
    
    // 增加计数器
    timer_configs[1].counter++;
    
    // 检查是否达到目标计数值
    if(timer_configs[1].counter >= timer_configs[1].target_count) {
        timer_configs[1].counter = 0; // 重置计数器
        
        // 执行回调函数
        if(timer_configs[1].callback_enabled && timer_configs[1].callback) {
            timer_configs[1].callback();
        }
    }
}

// 定时器2中断服务程序
void Timer2_ISR() interrupt 5 {
    TF2H = 0; // 清除定时器2溢出标志
    
    // 增加计数器
    timer_configs[2].counter++;
    
    // 检查是否达到目标计数值
    if(timer_configs[2].counter >= timer_configs[2].target_count) {
        timer_configs[2].counter = 0; // 重置计数器
        
        // 执行回调函数
        if(timer_configs[2].callback_enabled && timer_configs[2].callback) {
            timer_configs[2].callback();
        }
    }
}

// 定时器3中断服务程序
void Timer3_ISR() interrupt 16 {
    TMR3CN &= ~0x80; // 清除定时器3溢出标志
    
    // 增加计数器
    timer_configs[3].counter++;
    
    // 检查是否达到目标计数值
    if(timer_configs[3].counter >= timer_configs[3].target_count) {
        timer_configs[3].counter = 0; // 重置计数器
        
        // 执行回调函数
        if(timer_configs[3].callback_enabled && timer_configs[3].callback) {
            timer_configs[3].callback();
        }
    }
}

// 定时器4中断服务程序
void Timer4_ISR() interrupt 17 {
    TMR4CN &= ~0x80; // 清除定时器4溢出标志
    
    // 增加计数器
    timer_configs[4].counter++;
    
    // 检查是否达到目标计数值
    if(timer_configs[4].counter >= timer_configs[4].target_count) {
        timer_configs[4].counter = 0; // 重置计数器
        
        // 执行回调函数
        if(timer_configs[4].callback_enabled && timer_configs[4].callback) {
            timer_configs[4].callback();
        }
    }
}

// 停止指定定时器
void Timer_Stop(uint8_t timer_num) {
    if(timer_num >= 5) return;
    
    switch(timer_num) {
        case 0:
            TR0 = 0;    // 停止定时器0
            ET0 = 0;    // 禁止定时器0中断
            break;
        case 1:
            TR1 = 0;    // 停止定时器1
            ET1 = 0;    // 禁止定时器1中断
            break;
        case 2:
            TR2 = 0;    // 停止定时器2
            ET2 = 0;    // 禁止定时器2中断
            break;
        case 3:
            TR3 = 0;    // 停止定时器3
//            ET3 = 0;    // 禁止定时器3中断
				    EIE2&=~0X01;
            break;
        case 4:
            TR4 = 0;    // 停止定时器4
//            ET4 = 0;    // 禁止定时器4中断
		        EIE2&=~0X04;				
            break;
    }
    
    timer_configs[timer_num].state = TIMER_DISABLED;
    timer_configs[timer_num].counter = 0;
}

// 启动指定定时器
void Timer_Start(uint8_t timer_num) {
    if(timer_num >= 5) return;
    
    switch(timer_num) {
        case 0:
            TR0 = 1;    // 启动定时器0
            ET0 = 1;    // 使能定时器0中断
            break;
        case 1:
            TR1 = 1;    // 启动定时器1
            ET1 = 1;    // 使能定时器1中断
            break;
        case 2:
            TR2 = 1;    // 启动定时器2
            ET2 = 1;    // 使能定时器2中断
            break;
        case 3:
            TR3 = 1;    // 启动定时器3
//            ET3 = 1;    // 使能定时器3中断
						EIE2|=0X01;
            break;
        case 4:
            TR4 = 1;    // 启动定时器4
//            ET4 = 1;    // 使能定时器4中断
		        EIE2|=0X04;				
            break;
    }
    
    timer_configs[timer_num].state = TIMER_ENABLED;
}

// 获取定时器状态
timer_state_t Timer_GetState(uint8_t timer_num) {
    if(timer_num >= 5) return TIMER_DISABLED;
    return timer_configs[timer_num].state;
}

// 示例回调函数
void timer0_callback() {
    // 定时器0回调函数
    // 可以在这里执行定时任务
}

void timer1_callback() {
    // 定时器1回调函数
    // 可以在这里执行定时任务
}

void timer2_callback() {
    // 定时器2回调函数
    // 可以在这里执行定时任务
    // 更新Modbus超时计数器
    if(modbus_timer_active) {
        modbus_timeout_counter++;
        
        // 检查是否超时
        if(modbus_timeout_counter >= MODBUS_TIMEOUT_MS) {
            // 超时，标记接收完成
            if(modbus_rx_index > 0) {
                modbus_rx_complete = 1;
            }
            
            // 重置计时器
            modbus_timer_active = 0;
        }
    }	
	
	
}

void timer3_callback() {
    // 定时器3回调函数
    // 可以在这里执行定时任务
}

void timer4_callback() {
    // 定时器4回调函数
    // 可以在这里执行定时任务
}