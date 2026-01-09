
#include "flash.h"


// ************************* 手册对齐的宏定义（修正核心） *************************


// 1. 寄存器位定义（手册 15.1.2/15.1.3）
#define PSCTL_SFLE    0x04  // PSCTL.2：临时扇区访问使能（1=允许）
#define PSCTL_PSEE    0x02  // PSCTL.1：扇区擦除使能（1=允许）
#define PSCTL_PSWE    0x01  // PSCTL.0：Flash写使能（1=允许，MOVX指向Flash）
#define FLSCL_FLWE    0x01  // FLSCL.0：Flash写/擦除允许（1=允许）
#define CCH0CN_CHBLKW 0x01  // CCH0CN.0：块写使能（1=块写，0=单字节写）

// 2. 临时扇区地址（手册明确：0x00~0x7F 扇区1，0x80~0xFF 扇区2）
#define TEMP_SECTOR1_START  0x00U    // 扇区1：128字节（0x00~0x7F）
#define TEMP_SECTOR2_START  0x80U    // 扇区2：128字节（0x80~0xFF）
#define TEMP_SECTOR_SIZE    128U     // 单扇区大小
#define ERASE_BOTH_SECTORS  0x0400U  // 同时擦除两扇区的目标地址

// 3. COBANK 配置值（128KB Flash：COBANK=01 → PSBANK=0x40）
#define PSBANK_COBANK_128KB  0x40U

// 4. 备份有效标记+CRC偏移（扇区1末尾，不占用参数空间）
#define BACKUP_FLAG_OFFSET  0x7EU    // 扇区1：0x7E（1字节，0xAA=有效）
#define BACKUP_CRC_OFFSET   0x7FU    // 扇区1：0x7F（1字节，CRC8校验）
// 注：扇区1 0x00~0x7D 存参数（126字节），0x7E~0x7F 存标记+CRC（2字节）



// 进入 Flash 临时区操作模式（配置 COBANK+SFLE，禁止中断）
static void Flash_Temp_Enter(void) {
    EA = 0;                  // 1. 禁止中断（手册强制第一步）
    PSCTL |= PSCTL_SFLE;     // 3. 使能临时扇区访问（SFLE=1）
}

// 退出 Flash 临时区操作模式（恢复寄存器，允许中断）
static void Flash_Temp_Exit(void) {
    PSCTL &= ~(PSCTL_SFLE | PSCTL_PSEE | PSCTL_PSWE);  // 清除所有标志
    FLSCL &= ~FLSCL_FLWE;    // 禁止 Flash 写/擦除
    PSBANK = 0x00U;          // 恢复 COBANK 为默认
    EA = 1;                  // 重新允许中断（手册强制最后一步）
}


/**
 * @brief 擦除临时扇区（按手册步骤）
 * @param erase_mode：0=擦除扇区1，1=擦除扇区2，2=同时擦除两扇区
 * @return 无
 */
void Flash_Temp_Erase(uint8_t erase_mode) {
    Flash_Temp_Enter();  // 准备：禁中断+配置COBANK+SFLE

    // 4. 允许 Flash 写/擦除（FLWE=1）
    FLSCL |= FLSCL_FLWE;
    // 5. 允许扇区擦除（PSEE=1）
    PSCTL |= PSCTL_PSEE;
    // 6. 允许 Flash 写（PSWE=1，MOVX指向Flash）
    PSCTL |= PSCTL_PSWE;

    // 7. MOVX 写地址触发擦除（手册核心步骤）
    switch(erase_mode) {
        case 0:  // 擦除扇区1：写扇区1任意地址（如0x00）
            *(uint8_t xdata *)TEMP_SECTOR1_START = 0xFFU;
            break;
        case 1:  // 擦除扇区2：写扇区2任意地址（如0x80）
            *(uint8_t xdata *)TEMP_SECTOR2_START = 0xFFU;
            break;
        case 2:  // 同时擦除两扇区：写地址0x0400
            *(uint8_t xdata *)ERASE_BOTH_SECTORS = 0xFFU;
            break;
    }
    while (!(FLSCL & 0x02U));  // 等待擦除完成（FLSCL.1=1表示操作完成，手册隐含）

    // 8. 清除 PSEE（禁止扇区擦除）
    PSCTL &= ~PSCTL_PSEE;
    // 9. 清除 PSWE（MOVX指向XRAM）
    PSCTL &= ~PSCTL_PSWE;
    // 10. 清除 FLWE（禁止Flash写/擦除）
    FLSCL &= ~FLSCL_FLWE;

    Flash_Temp_Exit();  // 退出：恢复寄存器+开中断
}


/**
 * @brief 单字节写 Flash 临时区（需先擦除扇区）
 * @param addr：目标地址（0x00~0xFF，必须在临时区内）
 * @param data：待写字节
 * @return 1=成功，0=地址越界
 */
uint8_t Flash_Temp_Write_Byte(uint8_t addr, uint8_t sdata) {
	
    if (addr > 0xFFU) return 0;  // 地址越界

    Flash_Temp_Enter();

    // 4. 清除 CHBLKW（选择单字节写）
    CCH0CN &= ~CCH0CN_CHBLKW;
    // 5. 允许 Flash 写/擦除（FLWE=1）
    FLSCL |= FLSCL_FLWE;
    // 6. 允许 Flash 写（PSWE=1）
    PSCTL |= PSCTL_PSWE;

    // 7. MOVX 写字节（手册指令要求）
    *(uint8_t xdata *)addr = sdata;
    while (!(FLSCL & 0x02U));  // 等待写完成

    // 8. 清除 PSWE
    PSCTL &= ~PSCTL_PSWE;
    // 9. 清除 FLWE
    FLSCL &= ~FLSCL_FLWE;

    Flash_Temp_Exit();
    return 1;
}

/**
 * @brief 块写 Flash 临时区（2字节/块，需先擦除扇区）
 * @param start_addr：块起始地址（末位必须为0b，如0x00、0x02）
 * @param data1：第1字节（末位0b地址）
 * @param data2：第2字节（末位1b地址）
 * @return 1=成功，0=地址越界/起始地址末位不为0b
 */
uint8_t Flash_Temp_Write_Block(uint8_t start_addr, uint8_t data1, uint8_t data2) {
	
	 
    if (start_addr > 0xFEU || (start_addr & 0x01U) != 0U) return 0;  // 地址越界或末位非0b

    Flash_Temp_Enter();

    // 4. 置位 CHBLKW（选择块写）
    CCH0CN |= CCH0CN_CHBLKW;
    // 5. 允许 Flash 写/擦除（FLWE=1）
    FLSCL |= FLSCL_FLWE;
    // 6. 允许 Flash 写（PSWE=1）
    PSCTL |= PSCTL_PSWE;

    // 7. 按顺序写2字节（0b→1b），最后写1b地址触发内部写（手册强制顺序）
    *(uint8_t xdata *)start_addr = data1;          // 第1字节：末位0b
    *(uint8_t xdata *)(start_addr + 1U) = data2;  // 第2字节：末位1b（触发写操作）
    while (!(FLSCL & 0x02U));  // 等待块写完成

    // 8. 清除 PSWE
    PSCTL &= ~PSCTL_PSWE;
    // 9. 清除 FLWE
    FLSCL &= ~FLSCL_FLWE;

    Flash_Temp_Exit();
    return 1;
}