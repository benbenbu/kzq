

#include "delay.h"



#ifndef T_MODE
#define T_MODE 1  // 默认为1T模式
#endif



// 微秒延时函数
void Delay_us(unsigned int us) {
    unsigned int i;
    for (i = 0; i < us; i++)
    {
        // 22个_NOP_()实现约1μs延时 (994.62ns)
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
