#include "lcd.h"

/**
 * @brief 将16×18的字符数据转换为UC1698 LCD可以显示的格式
 * @param input 输入的16×18字符数据（36字节）
 * @param output 输出的转换后数据（144字节，16行×9字节/行）
 * @note 转换规则：每两个点构成一个字节，0转换为0000b，1转换为1111b
 * @note 输入数据是水平扫描的，从左到右，从上到下
 * @note 输出数据每行包含9个字节，对应18个像素（每2个像素1个字节）
 */
void Convert16x18Char(const unsigned char *input, unsigned char *output) {
    unsigned int i, j, bit_pos;
    unsigned char pixel1, pixel2;
    unsigned int output_index = 0;
    
    // 遍历16行
    for (i = 0; i < 16; i++) {
        // 遍历每行18个像素（9个字节的输出数据）
        for (j = 0; j < 9; j++) {
            // 计算当前处理的位位置
            bit_pos = i * 18 + j * 2;
            
            // 获取第一个像素点的值（0或1）
            if (input[bit_pos / 8] & (0x80 >> (bit_pos % 8))) {
                pixel1 = 0xF0;  // 1转换为1111b，放在高4位
            } else {
                pixel1 = 0x00;  // 0转换为0000b，放在高4位
            }
            
            // 获取第二个像素点的值（0或1）
            if (input[(bit_pos + 1) / 8] & (0x80 >> ((bit_pos + 1) % 8))) {
                pixel2 = 0x0F;  // 1转换为1111b，放在低4位
            } else {
                pixel2 = 0x00;  // 0转换为0000b，放在低4位
            }
            
            // 组合两个像素点的转换结果
            output[output_index++] = pixel1 | pixel2;
        }
    }
}
