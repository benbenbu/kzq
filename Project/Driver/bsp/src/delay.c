
#include <intrins.h>
#include "delay.h"



#ifndef T_MODE
#define T_MODE 1  // 默认为1T模式
#endif



// 微秒延时函数
void Delay_us(unsigned int us) {
    // 核心：volatile强制变量i不被优化，保证循环逻辑完整
     unsigned int i;  
    // for循环：编译器无法精简volatile变量的循环
    for (i = 0; i < us; i++)
    {
        // 22个_nop_()，每个_nop_()对应1个汇编NOP指令，编译器无法优化
        _nop_(); _nop_(); _nop_(); _nop_(); _nop_();
        _nop_(); _nop_(); _nop_(); _nop_(); _nop_();
        _nop_(); _nop_(); _nop_(); _nop_(); _nop_();
        _nop_(); _nop_(); _nop_(); _nop_(); _nop_();
        _nop_(); _nop_();
    }
}

// 毫秒延时函数
void Delay_ms(unsigned int ms) {
     unsigned int i;  
    
    for(i = 0; i < ms; i++) {
        // 使用精确的微秒延时来实现毫秒延时
        Delay_us(1000);
    }
}
