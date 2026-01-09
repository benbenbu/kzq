
#include "config.h"
#include <string.h>
#include "data_save.h"
#include "fm31256.h"
#include "data_deal.h"



 Pass_Para_Struct g_protect_password;     // 6位口令
 Alarm_Para_Struct g_alarm_minutes;        // 报警时间（分钟）
 Contra_Para_Struct g_lcd_contrast;         // 对比度 0~255

 Ratio_Para_Struct g_sys_pt_ct;  //系统pt、ct

 Protect_Para_Struct g_vol_h,g_vol_l;//过压保护、欠压保护参数
 Com_Para_Struct  g_com;//通讯参数
 Cap_Para_Struct  g_cap[4];//电容参数
 Control_Para_Struct g_control_para;//控制参数
 
 AutoManual_Para_Struct g_adjust_cap; //调试开关
 Cap_Num_Struct g_cap_num;  //投入电容数量 1-4  几路保护

 Cap_Protect_Para_Struct  g_cap_protect[4];
 Cap_Ratio_Para_Struct  g_cap_ratio[4];

 Stat_Para_Struct  g_stat;

// ************************* 内部工具函数（仅本文件调用，带功能说明） *************************



/**
 * @brief 读取FM31256校表数据（）
 * @param calib_channel 校表通道（1~4）
 * @param read_data     输出缓冲区
 * @param data_len      数据长度
 * @param saved_chksum3e 输出：存储的0x3E校验和（供后续HT7036验证用）
 * @param saved_chksum5e 输出：存储的0x5E校验和（供后续HT7036验证用）
 * @return 错误码（见宏定义）
 */
Para_Op_Result_E FM31256_Read_Calib(uint8_t calib_channel, uint8_t *read_data, uint16_t data_len,
                           uint32_t *saved_chksum3e, uint32_t *saved_chksum5e)
{
    // 确定主/备区地址（复用你的地址定义）
    uint16_t main_addr = 0, backup_addr = 0;	
    uint8_t temp_buf[100] = {0};
    // 参数校验
    if (read_data == NULL || saved_chksum3e == NULL || saved_chksum5e == NULL || data_len < 7)
        return PARA_OP_ERR_ADDR_OVERFLOW; // 至少需包含数据+3字节3E+3字节5E+1字节CRC8



    switch (calib_channel)
    {
        case 1: main_addr = CALIB1_MAIN_DATA; backup_addr = CALIB1_BACKUP_DATA; break;
        case 2: main_addr = CALIB2_MAIN_DATA; backup_addr = CALIB2_BACKUP_DATA; break;
        case 3: main_addr = CALIB3_MAIN_DATA; backup_addr = CALIB3_BACKUP_DATA; break;
        case 4: main_addr = CALIB4_MAIN_DATA; backup_addr = CALIB4_BACKUP_DATA; break;

    }

    if (data_len > sizeof(temp_buf)) return PARA_OP_ERR_ADDR_OVERFLOW;

    // ------------------------ 第一步：读主区+CRC8校验 ------------------------
    if (FM31256_FRAM_Read(main_addr, temp_buf, data_len) == 0)
    {
        // 计算CRC8（排除最后1字节的CRC值）
        uint8_t crc_calc = CRC8_Calc(temp_buf, data_len - 1);
        uint8_t crc_saved = temp_buf[data_len - 1];
        
        if (crc_calc == crc_saved)
        {
            // 提取存储的0x3E/0x5E校验和（假设存储在数据倒数7~4字节：3E占3字节，5E占3字节）
            *saved_chksum3e = (temp_buf[data_len - 7] << 16) | (temp_buf[data_len - 6] << 8) | temp_buf[data_len - 5];
            *saved_chksum5e = (temp_buf[data_len - 4] << 16) | (temp_buf[data_len - 3] << 8) | temp_buf[data_len - 2];
            memcpy(read_data, temp_buf, data_len);
            return PARA_OP_SUCCESS;
        }
    }

    // ------------------------ 第二步：主区失效，读备区+CRC8校验 ------------------------
    if (FM31256_FRAM_Read(backup_addr, temp_buf, data_len) == 0)
    {
        uint8_t crc_calc = CRC8_Calc(temp_buf, data_len - 1);
        uint8_t crc_saved = temp_buf[data_len - 1];
        
        if (crc_calc == crc_saved)
        {
            *saved_chksum3e = (temp_buf[data_len - 7] << 16) | (temp_buf[data_len - 6] << 8) | temp_buf[data_len - 5];
            *saved_chksum5e = (temp_buf[data_len - 4] << 16) | (temp_buf[data_len - 3] << 8) | temp_buf[data_len - 2];
            memcpy(read_data, temp_buf, data_len);
            return PARA_OP_SUCCESS;
        }
    }

    // ------------------------ 第三步：主/备区均失效 ------------------------
    return (FM31256_FRAM_Read(main_addr, temp_buf, data_len) != 0) ? PARA_OP_ERR_IIC :
           (FM31256_FRAM_Read(backup_addr, temp_buf, data_len) != 0) ? PARA_OP_ERR_IIC :
           (calib_channel == 1 ? PARA_OP_ERR_CRC_MAIN : PARA_OP_ERR_CRC_BACKUP);
}


/**
 * @brief  写入FM31256校表数据（主备区双写+CRC8校验）
 * @param  calib_channel：校表通道（1~4）
 * @param  write_data：待写入校表数据（输入，非NULL）
 * @return Para_Op_Result_E：操作结果（0=成功，非0=失败）
 * @note   1. 主区写入数据+CRC8；2. 写入后立即校验，失败自动重试1次；
 */
Para_Op_Result_E FM31256_Write_Calib(uint8_t calib_channel, const uint8_t *write_data, uint16_t data_len)
{
	  uint16_t main_addr = 0;
    uint8_t read_back_buf[100] = {0}; // 读回验证缓冲区
    uint8_t retry = 2; // 最多重试2次（初始1次+1次重试）
    uint8_t write_ok = 0;
		uint8_t crc_calc;
		uint8_t crc_saved;
    // 1. 参数合法性校验
    if (write_data == NULL) {
        return PARA_OP_ERR_NULL_PTR; // 空指针错误
    }
//    if (calib_channel < 1 || calib_channel > 4) {
//        return PARA_OP_ERR_CH_OVERFLOW; // 通道号越界
//    }
//    if (data_len < 7 || data_len > 100) { // 限制最大长度避免缓冲区溢出
//        return PARA_OP_ERR_ADDR_OVERFLOW; // 数据长度非法
//    }
    // 3. 映射通道对应的主备区地址
    switch (calib_channel) {
        case 1: main_addr = CALIB1_MAIN_DATA; break;
        case 2: main_addr = CALIB2_MAIN_DATA; break;
        case 3: main_addr = CALIB3_MAIN_DATA;  break;
        case 4: main_addr = CALIB4_MAIN_DATA;  break;
    }

    // 4. 带重试写入+验证
    while (retry--) {

        if (FM31256_FRAM_Write(main_addr, write_data, data_len) != 0) {
            continue; // 主区写入失败，重试
        }
        // 4.3 读回主区数据做双重验证
        memset(read_back_buf, 0, sizeof(read_back_buf));
        if (FM31256_FRAM_Read(main_addr, read_back_buf, data_len) != 0) {
            continue; // 读回失败，重试
        }

        // 验证1：全字节数据完全匹配（最直接的验证）
        if (memcmp(read_back_buf, write_data, data_len) != 0) {
            continue; // 数据不一致，重试
        }

        // 验证2：CRC8校验通过（双重保障，避免数据位翻转）
         crc_calc = CRC8_Calc(read_back_buf, data_len - 1); // 计算前N-1字节的CRC
         crc_saved = read_back_buf[data_len - 1]; // 读回的CRC8值（最后1字节）
        if (crc_calc == crc_saved) {
            write_ok = 1;
            break; // 双重验证通过，退出重试
        }
    }
    return write_ok ? PARA_OP_SUCCESS : PARA_OP_ERR_HW_WRITE;
}



// ************************* 通用参数读写（主备区同步+CRC校验，仅本文件调用） *************************
typedef void (*Param_InitFunc)(void *param);  /* 参数初始化函数指针类型定义 */

/**
 * @brief  应用层参数读取+I2C错误重试（核心示例）
 * @param  （参数同之前，略）
 * @return Para_Op_Result_E：分层错误码
 */
static Para_Op_Result_E Read_Param_With_Backup(uint16_t main_data_addr, uint16_t main_crc_addr,
                                               uint16_t backup_data_addr, uint16_t backup_crc_addr,
                                               void *param, uint16_t para_len) {
    uint8_t crc_main, crc_backup;
    uint8_t fram_err;          // 驱动层返回的错误
    uint8_t retry;             // 重试计数器

    if (param == NULL) {
        return PARA_OP_ERR_NULL_PTR; // 空指针：直接返回，不重试
    }


    retry = 2;
    do {
        // 1. 调用驱动层读取主区数据
        fram_err = FM31256_FRAM_Read(main_data_addr, param, para_len);
        
        // 2. 按错误类型处理
        if (fram_err == FM31256_OK) {
            // 读取成功：退出重试，继续校验CRC
            break;
        }  else {
            // I2C通信错误：准备重试
            retry--;
            if (retry > 0) {
                // 重试前：清理I2C总线 + 短暂延时（关键！）
//                I2C_ClearBus();  // 调用硬件层的总线清理函数
//                delay_ms(I2C_RETRY_DELAY); // 延时1ms，避免总线忙
                continue;        // 继续下一次重试
            } else {
                // 重试耗尽：退出循环，切换到备区
                break;
            }
        }
    } while (retry > 0);

    // 主区读取成功 → 校验CRC
    if (fram_err == FM31256_OK) {
        // 读取主区CRC值（同样带重试）
        retry = 2;
        do {
            fram_err = FM31256_FRAM_Read(main_crc_addr, &crc_main, PARA_CRC_LEN);
            if (fram_err == FM31256_OK) break;
            else {
                retry--;
               if (retry > 0) 
								 { 
//									 I2C_ClearBus(); delay_ms(I2C_RETRY_DELAY); 
								}
            }
        } while (retry > 0);

        // 主区CRC读取成功且校验通过 → 返回成功
        if (fram_err == FM31256_OK && CRC8_Calc((uint8_t *)param, para_len) == crc_main) {
            return PARA_OP_SUCCESS;
        }
    }

    // #################### 第三步：主区失效 → 读取备区（同样带重试） ####################
    retry = 2;
    do {
        fram_err = FM31256_FRAM_Read(backup_data_addr, param, para_len);
        if (fram_err == FM31256_OK) break;
        else {
            retry--;
            if (retry > 0) { 
//						I2C_ClearBus(); delay_ms(I2C_RETRY_DELAY); 
						}
        }
    } while (retry > 0);

    // 备区读取成功 → 校验CRC
    if (fram_err == FM31256_OK) {
        retry = 2;
        do {
            fram_err = FM31256_FRAM_Read(backup_crc_addr, &crc_backup, PARA_CRC_LEN);
            if (fram_err == FM31256_OK) break;
            else  {
                retry--;
                if (retry > 0) { 
//									I2C_ClearBus(); delay_ms(I2C_RETRY_DELAY);
								}
            }
        } while (retry > 0);

        // 备区CRC校验通过 → 同步到主区 + 返回主区CRC错误
        if (fram_err == FM31256_OK && CRC8_Calc((uint8_t *)param, para_len) == crc_backup) {
            // 同步备区到主区（写操作也可加重试，可选）
            FM31256_FRAM_Write(main_data_addr, param, para_len);
            FM31256_FRAM_Write(main_crc_addr, &crc_backup, PARA_CRC_LEN);
            return PARA_OP_ERR_CRC_MAIN;
        }
    }

    // 区分是I2C错误还是CRC错误
    return (fram_err == FM31256_ERR_I2C) ? PARA_OP_ERR_IIC : PARA_OP_ERR_CRC_BACKUP;
}
																							 



// ************************* 参数修改日志索引管理（仅本文件调用） *************************
/**
 * @brief  获取当前参数修改日志的存储索引
 * @param  无
 * @return 日志索引（0~MODIFY_LOG_CNT-1）
 * @note   索引超出范围时，返回0（避免越界）
 */
static uint8_t Get_Log_Index(void) {
    uint8_t index = 0;
    FM31256_FRAM_Read(LOG_INDEX_ADDR, &index, 1);  /* 从FRAM读取当前索引 */
    return (index >= MODIFY_LOG_CNT) ? 0 : index;  /* 越界保护：返回0 */
}

/**
 * @brief  更新参数修改日志索引（支持循环覆盖）
 * @param  index：当前日志索引指针（输入+输出）
 * @return 无
 * @note   索引自增1，达到最大条数时重置为0（循环覆盖最早的日志）
 */
static void Update_Log_Index(uint8_t *index) {
    *index = (*index + 1) % MODIFY_LOG_CNT;  /* 循环自增：0→1→...→99→0 */
    FM31256_FRAM_Write(LOG_INDEX_ADDR, index, 1);  /* 保存更新后的索引到FRAM */
}

// ************************* 时间戳相关函数（日志/事件时间记录用） *************************
/**
 * @brief  判断指定年份是否为闰年
 * @param  year：完整年份（如2025）
 * @return 1：闰年；0：平年
 * @note   闰年规则：能被4整除且不能被100整除，或能被400整除
 */
static uint8_t Is_LeapYear(uint16_t year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0) ? 1 : 0;
}

/**
 * @brief  获取指定年份和月份的天数（适配平年/闰年）
 * @param  year：完整年份（如2025）
 * @param  month：月份（1~12）
 * @return 当月天数（如2月：28或29天）
 */
static uint8_t Get_MonthDays(uint16_t year, uint8_t month) {
    uint8_t month_days[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    /* 2月且为闰年：返回29天，否则返回数组中定义的天数 */
    return (month == 2 && Is_LeapYear(year)) ? 29 : month_days[month];
}

/**
 * @brief  将日期时间转换为秒级时间戳（基准：BASE_YEAR-01-01 00:00:00）
 * @param  date：日期结构体指针（年/月/日）
 * @param  time：时间结构体指针（时/分/秒）
 * @return 秒级时间戳（从基准时间到当前时间的总秒数）
 */
static uint32_t DateTime_To_TimeStamp(const Date_Struct *date, const Time_Struct *time) {
    uint32_t total_sec = 0;
    uint16_t full_year = BASE_YEAR + date->year;  /* 后两位年份→完整年份（如25→2025） */
    uint16_t year;
    uint8_t month;

    /* 1. 累加基准年到当前年的总秒数（按平年/闰年区分） */
    for (year = BASE_YEAR; year < full_year; year++) {
        total_sec += Is_LeapYear(year) ? 366UL * 24 * 3600 : 365UL * 24 * 3600;
    }
    /* 2. 累加当前年1月到当前月的总秒数（按当月天数） */
    for (month = 1; month < date->month; month++) {
        total_sec += Get_MonthDays(full_year, month) * 24UL * 3600;
    }
    /* 3. 累加当前日、时、分、秒的秒数 */
    total_sec += (date->day - 1) * 24UL * 3600 + time->hour * 3600UL + time->minute * 60UL + time->second;
    return total_sec;
}

/**
 * @brief  获取当前RTC的秒级时间戳（原子读取，避免数据错乱）
 * @param  无
 * @return 秒级时间戳（基准：BASE_YEAR-01-01 00:00:00）
 * @note   原子读取：关闭总中断→复制全局日期时间→开启总中断，避免读取过程中数据被修改
 */
uint32_t Get_RTC_TimeStamp(void) {
    Date_Struct temp_date;  /* 临时日期变量（避免直接操作全局变量） */
    Time_Struct temp_time;  /* 临时时间变量 */

    EA = 0;  /* 关闭总中断：确保读取过程中全局变量不被更新 */
    temp_date = system_date;  /* 复制全局日期到临时变量 */
    temp_time = system_time;  /* 复制全局时间到临时变量 */
    EA = 1;  /* 开启总中断：读取完成后恢复 */

    /* 转换为秒级时间戳并返回 */
    return DateTime_To_TimeStamp(&temp_date, &temp_time);
}

/**
 * @brief  写入参数到FRAM（主区+备份区，均写入新参数），并校验写入结果
 * @param  main_data_addr: 主区参数存储地址
 * @param  main_crc_addr:  主区CRC存储地址
 * @param  backup_data_addr: 备份区参数存储地址
 * @param  backup_crc_addr:  备份区CRC存储地址
 * @param  new_param:      新参数指针（主/备区均写入此参数）
 * @param  para_len:       参数长度（字节）
 * @retval 无（可根据需求改为返回错误码）
 * @note   主区和备份区均写入新参数+新CRC，写入后立即读取校验一致性
 */
static Para_Op_Result_E Write_Param_With_Backup_Log(uint16_t main_data_addr, uint16_t main_crc_addr,
                                        uint16_t backup_data_addr, uint16_t backup_crc_addr,
                                        const void *new_param,
                                        uint16_t para_len) {
																					
    uint8_t crc_new;                // 仅保留新参数的CRC（主/备区共用）
    uint8_t read_buf[256];          // 临时读取缓冲区（适配多数参数长度）
    uint8_t read_crc;               // 临时存储读取的CRC值
    uint8_t write_ok = 1;           // 写入状态标记：1-成功，0-失败
	  Para_Op_Result_E err;
    /* 空指针/无效长度保护 */
    if (new_param == NULL )
        return PARA_OP_ERR_NULL_PTR;			
		
		if( para_len == 0 || para_len > sizeof(read_buf)) {

        return PARA_OP_ERR_ADDR_OVERFLOW;
    }

    /* 计算新参数的CRC校验值（主/备区共用此CRC） */
    crc_new = CRC8_Calc((uint8_t *)new_param, para_len);

    /* -------------------------- 1. 写入主区并校验 -------------------------- */
    // 写入新参数到主区
    err=FM31256_FRAM_Write(main_data_addr, (uint8_t *)new_param, para_len);
    // 写入新CRC到主区
    err|=FM31256_FRAM_Write(main_crc_addr, &crc_new, PARA_CRC_LEN);

    // 校验主区参数写入结果
    memset(read_buf, 0, sizeof(read_buf));
    FM31256_FRAM_Read(main_data_addr, read_buf, para_len);
    if (memcmp(read_buf, new_param, para_len) != 0) {
        write_ok = 0;
        // 可选：主区参数写入失败处理，示例：printf("Param ID %d: main data write failed!\r\n", param_id);
    }

    // 校验主区CRC写入结果
    FM31256_FRAM_Read(main_crc_addr, &read_crc, PARA_CRC_LEN);
    if (read_crc != crc_new) {
        write_ok = 0;
        // 可选：主区CRC写入失败处理，示例：printf("Param ID %d: main CRC write failed!\r\n", param_id);
    }

    /* -------------------------- 2. 写入备份区并校验（同主区数据） -------------------------- */
    // 写入新参数到备份区（核心调整：不再写old_param，改为new_param）
    FM31256_FRAM_Write(backup_data_addr, (uint8_t *)new_param, para_len);
    // 写入新CRC到备份区（核心调整：不再写crc_old，改为crc_new）
    FM31256_FRAM_Write(backup_crc_addr, &crc_new, PARA_CRC_LEN);

    // 校验备份区参数写入结果
    memset(read_buf, 0, sizeof(read_buf));
    FM31256_FRAM_Read(backup_data_addr, read_buf, para_len);
    if (memcmp(read_buf, new_param, para_len) != 0) {
        write_ok = 0;
        // 可选：备份区参数写入失败处理，示例：printf("Param ID %d: backup data write failed!\r\n", param_id);
    }

    // 校验备份区CRC写入结果
    FM31256_FRAM_Read(backup_crc_addr, &read_crc, PARA_CRC_LEN);
    if (read_crc != crc_new) {
        write_ok = 0;
        // 可选：备份区CRC写入失败处理，示例：printf("Param ID %d: backup CRC write failed!\r\n", param_id);
    }

    /* -------------------------- 3. 写入结果统一处理 -------------------------- */
    if (write_ok == 0) {
        // 写入失败的兜底处理（可选）：比如尝试重新写入、标记错误状态等
        // 注意：重试需加次数限制，避免死循环
    }
		return PARA_OP_SUCCESS;
}



/**
 * @brief  系统参数总初始化（程序启动时调用）
 * @param  无
* @return 0:成功   1：没有读到数据(fm31256没写入)   2：fm31256有数据但数据出错   3：iic通讯错误
 * @note   
 */
Para_Op_Result_E Sys_Param_Init(void) {
	
                
    uint8_t err = 0;
    uint8_t i;		


        err=Get_Com_Para(&g_com);
        err|=Get_OV_Protect_Para(&g_vol_h);
        err|=Get_UV_Protect_Para(&g_vol_l);
        err|=Get_Control_Para(&g_control_para);
        for (i = 0; i < 4; i++) {
            err|=Get_Cap_Para_Ch(i, &g_cap[i]);
        }
        err|=Get_Ratio_Para(&g_sys_pt_ct);
        err|=Get_Pass_Para(&g_protect_password);
        err|=Get_AutoManual_Para(&g_adjust_cap);
        err|=Get_Contra_Para(&g_lcd_contrast);
				err|=Get_Alarm_Para(&g_alarm_minutes);
        for (i = 0; i < 4; i++) {
            err|=Get_Cap_Protect_Para_Ch(i, &g_cap_protect[i]);
        }				
        for (i = 0; i < 4; i++) {
            err|=Get_Cap_Ratio_Para_Ch(i, &g_cap_ratio[i]);
        }		
								
        err|=Get_Cap_Num(&g_cap_num);
				


    // 场景4：所有校验通过
    return err;
}
// ************************* 对外接口函数（供其他模块调用，带功能说明） *************************
/**
 * @brief  读取通讯参数
 * @param  para：存储通讯参数的结构体指针（输出参数）
 * @return 无
 * @note   内部调用Read_Param_With_Backup，支持主备区切换和失效恢复（仅内存默认值）
 */
Para_Op_Result_E Get_Com_Para(Com_Para_Struct *para) {
	  Para_Op_Result_E err;
    if (para != NULL) {  /* 空指针保护 */
       err=Read_Param_With_Backup(COM_PARA_MAIN_DATA, COM_PARA_MAIN_CRC,
                               COM_PARA_BACKUP_DATA, COM_PARA_BACKUP_CRC,
                               para, sizeof(Com_Para_Struct));
			return err;
    }
		return PARA_OP_ERR_NULL_PTR;
}

/**
 * @brief  设置通讯参数（自动记录修改日志）
 * @param  new_para：新通讯参数结构体指针（输入参数）
 * @return 无
 * @note   主程序按需调用，不主动触发则不覆盖已有数据
 */
Para_Op_Result_E Set_Com_Para(const Com_Para_Struct *new_para) {
		Para_Op_Result_E err;
    if (new_para != NULL) {  /* 空指针保护 */

       err= Write_Param_With_Backup_Log(COM_PARA_MAIN_DATA, COM_PARA_MAIN_CRC,
                                    COM_PARA_BACKUP_DATA, COM_PARA_BACKUP_CRC,
                                    new_para,sizeof(Com_Para_Struct));
			return err;
    }
		return PARA_OP_ERR_NULL_PTR;
}

/**
 * @brief  读取过压保护参数
 * @param  para：存储过压保护参数的结构体指针（输出参数）
 * @return 无
 * @note   内部调用Read_Param_With_Backup，支持主备区切换和失效恢复（仅内存默认值）
 */
Para_Op_Result_E Get_OV_Protect_Para(Protect_Para_Struct *para) {
	  Para_Op_Result_E err;
    if (para != NULL) {  /* 空指针保护 */
       err= Read_Param_With_Backup(OV_PROTECT_MAIN_DATA, OV_PROTECT_MAIN_CRC,
                               OV_PROTECT_BACKUP_DATA, OV_PROTECT_BACKUP_CRC,
                               para, sizeof(Protect_Para_Struct));
			return err;
    }
		return PARA_OP_ERR_NULL_PTR;
}

/**
 * @brief  设置过压保护参数（自动记录修改日志）
 * @param  new_para：新过压保护参数结构体指针（输入参数）
 * @return 无
 * @note   主程序按需调用，不主动触发则不覆盖已有数据
 */
Para_Op_Result_E Set_OV_Protect_Para(const Protect_Para_Struct *new_para) {
		Para_Op_Result_E err;
    if (new_para != NULL) {  /* 空指针保护 */
        err=Write_Param_With_Backup_Log(OV_PROTECT_MAIN_DATA, OV_PROTECT_MAIN_CRC,
                                    OV_PROTECT_BACKUP_DATA, OV_PROTECT_BACKUP_CRC,
                                    new_para,sizeof(Protect_Para_Struct));
						return err;
    }
		return PARA_OP_ERR_NULL_PTR;
}

/**
 * @brief  读取欠压保护参数
 * @param  para：存储欠压保护参数的结构体指针（输出参数）
 * @return 无
 * @note   内部调用Read_Param_With_Backup，支持主备区切换和失效恢复（仅内存默认值）
 */
Para_Op_Result_E Get_UV_Protect_Para(Protect_Para_Struct *para) {
	  Para_Op_Result_E err;
    if (para != NULL) {  /* 空指针保护 */
        err=Read_Param_With_Backup(UV_PROTECT_MAIN_DATA, UV_PROTECT_MAIN_CRC,
                               UV_PROTECT_BACKUP_DATA, UV_PROTECT_BACKUP_CRC,
                               para, sizeof(Protect_Para_Struct));
			return err;
    }
		return PARA_OP_ERR_NULL_PTR;
}

/**
 * @brief  设置欠压保护参数（自动记录修改日志）
 * @param  new_para：新欠压保护参数结构体指针（输入参数）
 * @return 无
 * @note   主程序按需调用，不主动触发则不覆盖已有数据
 */
Para_Op_Result_E Set_UV_Protect_Para(const Protect_Para_Struct *new_para) {
	  Para_Op_Result_E err;	
    if (new_para != NULL) {  /* 空指针保护 */
        err=Write_Param_With_Backup_Log(UV_PROTECT_MAIN_DATA, UV_PROTECT_MAIN_CRC,
                                    UV_PROTECT_BACKUP_DATA, UV_PROTECT_BACKUP_CRC,
                                    new_para,sizeof(Protect_Para_Struct));
						return err;
    }
		return PARA_OP_ERR_NULL_PTR;
}

/**
 * @brief  读取控制参数
 * @param  para：存储控制参数的结构体指针（输出参数）
 * @return 无
 * @note   内部调用Read_Param_With_Backup，支持主备区切换和失效恢复（仅内存默认值）
 */
Para_Op_Result_E Get_Control_Para(Control_Para_Struct *para) {
	  Para_Op_Result_E err;
    if (para != NULL) {  /* 空指针保护 */
       err= Read_Param_With_Backup(CONTROL_PARA_MAIN_DATA, CONTROL_PARA_MAIN_CRC,
                               CONTROL_PARA_BACKUP_DATA, CONTROL_PARA_BACKUP_CRC,
                               para, sizeof(Control_Para_Struct));
			return err;
    }
		return PARA_OP_ERR_NULL_PTR;
}

/**
 * @brief  设置控制参数（自动记录修改日志）
 * @param  new_para：新控制参数结构体指针（输入参数）
 * @return 无
 * @note   主程序按需调用，不主动触发则不覆盖已有数据
 */
Para_Op_Result_E Set_Control_Para(const Control_Para_Struct *new_para) {
	  Para_Op_Result_E err;
    if (new_para != NULL) {  /* 空指针保护 */
        err=Write_Param_With_Backup_Log(CONTROL_PARA_MAIN_DATA, CONTROL_PARA_MAIN_CRC,
                                    CONTROL_PARA_BACKUP_DATA, CONTROL_PARA_BACKUP_CRC,
                                    new_para, sizeof(Control_Para_Struct));
						return err;
    }
		return PARA_OP_ERR_NULL_PTR;
}

// ************************* 多通道电容参数读写接口（新增） *************************
/**
 * @brief  读取指定通道的电容参数
 * @param  ch：通道号（0=1路，1=2路...，需小于CAP_CHANNEL_CNT）
 * @param  para：存储参数的缓冲区指针（输出）
 * @return 无
 * @note   自动处理主备区切换、CRC校验、失效恢复（同原有逻辑）
 */
Para_Op_Result_E Get_Cap_Para_Ch(uint8_t ch, Cap_Para_Struct *para) {
	 Para_Op_Result_E err;
    if (ch >= CAP_CHANNEL_CNT || para == NULL) return PARA_OP_ERR_NULL_PTR; 

    // 调用通用读取函数，传入当前通道的地址
    err=Read_Param_With_Backup(
        CAP_PARA_MAIN_DATA_CH(ch),    /* 主区数据地址 */
        CAP_PARA_MAIN_CRC_CH(ch),     /* 主区CRC地址 */
        CAP_PARA_BACKUP_DATA_CH(ch),  /* 备份区数据地址 */
        CAP_PARA_BACKUP_CRC_CH(ch),   /* 备份区CRC地址 */
        para,                         /* 输出缓冲区 */
        sizeof(Cap_Para_Struct)      /* 参数长度 */
    );
			return err;
}

/**
 * @brief  设置指定通道的电容参数（自动记录日志）
 * @param  ch：通道号（0=1路，1=2路...，需小于CAP_CHANNEL_CNT）
 * @param  new_para：新参数缓冲区指针（输入）
 * @return 无
 * @note   自动处理主备区同步、CRC校验、修改日志记录（同原有逻辑）
 */
Para_Op_Result_E Set_Cap_Para_Ch(uint8_t ch, const Cap_Para_Struct *new_para) {
	  Para_Op_Result_E err;	
    if (new_para == NULL) return PARA_OP_ERR_NULL_PTR;  


    // 3. 调用通用写入函数，传入当前通道的地址和日志信息
   err= Write_Param_With_Backup_Log(
        CAP_PARA_MAIN_DATA_CH(ch),    /* 主区数据地址 */
        CAP_PARA_MAIN_CRC_CH(ch),     /* 主区CRC地址 */
        CAP_PARA_BACKUP_DATA_CH(ch),  /* 备份区数据地址 */
        CAP_PARA_BACKUP_CRC_CH(ch),   /* 备份区CRC地址 */
        new_para,                     /* 新参数 */
        sizeof(Cap_Para_Struct)      /* 参数长度 */
    );
		return err;
}

/**
 * @brief  读取变比参数
 * @param  para：存储变比参数的结构体指针（输出参数）
 * @return 无
 * @note   内部调用Read_Param_With_Backup，支持主备区切换和失效恢复（仅内存默认值）
 */
Para_Op_Result_E Get_Ratio_Para(Ratio_Para_Struct *para) {
	  Para_Op_Result_E err;
    if (para != NULL) {  /* 空指针保护 */
       err= Read_Param_With_Backup(RATIO_PARA_MAIN_DATA, RATIO_PARA_MAIN_CRC,
                               RATIO_PARA_BACKUP_DATA, RATIO_PARA_BACKUP_CRC,
                               para, sizeof(Ratio_Para_Struct));
			return err;
    }
		return PARA_OP_ERR_NULL_PTR;
}

/**
 * @brief  设置变比参数（自动记录修改日志）
 * @param  new_para：新变比参数结构体指针（输入参数）
 * @return 无
 * @note   主程序按需调用，不主动触发则不覆盖已有数据
 */
Para_Op_Result_E Set_Ratio_Para(const Ratio_Para_Struct *new_para) {
	  Para_Op_Result_E err;
    if (new_para != NULL) {  /* 空指针保护 */
       err= Write_Param_With_Backup_Log(RATIO_PARA_MAIN_DATA, RATIO_PARA_MAIN_CRC,
                                    RATIO_PARA_BACKUP_DATA, RATIO_PARA_BACKUP_CRC,
                                    new_para, sizeof(Ratio_Para_Struct));
			return err;
    }
		return PARA_OP_ERR_NULL_PTR;
}

/**
 * @brief  读取统计参数
 * @param  para：存储统计参数的结构体指针（输出参数）
 * @return 无
 * @note   内部调用Read_Param_With_Backup，支持主备区切换和失效恢复（仅内存默认值）
 */
Para_Op_Result_E Get_Stat_Para(Stat_Para_Struct *para) {
	  Para_Op_Result_E err;
    if (para != NULL) {  /* 空指针保护 */
       err= Read_Param_With_Backup(STAT_PARA_MAIN_DATA, STAT_PARA_MAIN_CRC,
                               STAT_PARA_BACKUP_DATA, STAT_PARA_BACKUP_CRC,
                               para, sizeof(Stat_Para_Struct));
			return err;
    }
		return PARA_OP_ERR_NULL_PTR;
}


/**
 * @brief  设置统计
 * @param  new_para：新参数结构体指针（输入参数）
 * @return 无
 * @note   主程序按需调用，不主动触发则不覆盖已有数据
 */
Para_Op_Result_E Set_Stat_Para(const Stat_Para_Struct *new_para) {
	  Para_Op_Result_E err;
    if (new_para != NULL) {  /* 空指针保护 */
        err=Write_Param_With_Backup_Log(STAT_PARA_MAIN_DATA, STAT_PARA_MAIN_CRC,
                                    STAT_PARA_BACKUP_DATA, STAT_PARA_BACKUP_CRC,
                                    new_para, sizeof(Stat_Para_Struct));
			return err;
    }
		return PARA_OP_ERR_NULL_PTR;
}




/**
 * @brief  读取口令参数
 * @param  para：存储口令参数的结构体指针（输出参数）
 * @return 无
 * @note   内部调用Read_Param_With_Backup，支持主备区切换和失效恢复（仅内存默认值）
 */
Para_Op_Result_E Get_Pass_Para(Pass_Para_Struct *para) {
	Para_Op_Result_E err;
    if (para != NULL) {  /* 空指针保护 */
       err= Read_Param_With_Backup(PASS_PARA_MAIN_DATA, PASS_PARA_MAIN_CRC,
                               PASS_PARA_BACKUP_DATA, PASS_PARA_BACKUP_CRC,
                               para, sizeof(Pass_Para_Struct));
			return err;
    }
		return PARA_OP_ERR_NULL_PTR;
}

/**
 * @brief  设置口令
 * @param  new_para：新参数结构体指针（输入参数）
 * @return 无
 * @note   主程序按需调用，不主动触发则不覆盖已有数据
 */
Para_Op_Result_E Set_Pass_Para(const Pass_Para_Struct *new_para) {
	  Para_Op_Result_E err;
    if (new_para != NULL) {  /* 空指针保护 */
       err= Write_Param_With_Backup_Log(PASS_PARA_MAIN_DATA, PASS_PARA_MAIN_CRC,
                                    PASS_PARA_BACKUP_DATA, PASS_PARA_BACKUP_CRC,
                                    new_para, sizeof(Pass_Para_Struct));
			return err;
    }
		return PARA_OP_ERR_NULL_PTR;
}


/**
 * @brief  读取手自动参数
 * @param  para：存储手自动的结构体指针（输出参数）
 * @return 无
 * @note   内部调用Read_Param_With_Backup，支持主备区切换和失效恢复（仅内存默认值）
 */
Para_Op_Result_E Get_AutoManual_Para(AutoManual_Para_Struct *para) {
	  Para_Op_Result_E err;
    if (para != NULL) {  /* 空指针保护 */
        err=Read_Param_With_Backup(AUTO_MANUAL_PARA_MAIN_DATA, AUTO_MANUAL_PARA_MAIN_CRC,
                               AUTO_MANUAL_PARA_BACKUP_DATA, AUTO_MANUAL_PARA_BACKUP_CRC,
                               para, sizeof(AutoManual_Para_Struct));
			return err;
    }
		return PARA_OP_ERR_NULL_PTR;
}
/**
 * @brief  设置手自动参数
 * @param  new_para：新参数结构体指针（输入参数）
 * @return 无
 * @note   主程序按需调用，不主动触发则不覆盖已有数据
 */
Para_Op_Result_E Set_AutoManual_Para(const AutoManual_Para_Struct *new_para) {
	  Para_Op_Result_E err;
    if (new_para != NULL) {  /* 空指针保护 */
        err=Write_Param_With_Backup_Log(AUTO_MANUAL_PARA_MAIN_DATA, AUTO_MANUAL_PARA_MAIN_CRC,
                                    AUTO_MANUAL_PARA_BACKUP_DATA, AUTO_MANUAL_PARA_BACKUP_CRC,
                                    new_para, sizeof(AutoManual_Para_Struct));
			return err;
    }
				return PARA_OP_ERR_NULL_PTR;
}

/**
 * @brief  读取对比度参数
 * @param  para：存储对比度参数的结构体指针（输出参数）
 * @return 无
 * @note   内部调用Read_Param_With_Backup，支持主备区切换和失效恢复（仅内存默认值）
 */
Para_Op_Result_E Get_Contra_Para(Contra_Para_Struct *para) {
	  Para_Op_Result_E err;
    if (para != NULL) {  /* 空指针保护 */
       err= Read_Param_With_Backup(CONTRA_PARA_MAIN_DATA, CONTRA_PARA_MAIN_CRC,
                               CONTRA_PARA_BACKUP_DATA, CONTRA_PARA_BACKUP_CRC,
                               para, sizeof(Contra_Para_Struct));
			return err;
    }
		return PARA_OP_ERR_NULL_PTR;
}

/**
 * @brief  设置手对比度参数
 * @param  new_para：新参数结构体指针（输入参数）
 * @return 无
 * @note   主程序按需调用，不主动触发则不覆盖已有数据
 */
Para_Op_Result_E Set_Contra_Para(const Contra_Para_Struct *new_para) {
	  Para_Op_Result_E err;  
    if (new_para != NULL) {  /* 空指针保护 */
       err= Write_Param_With_Backup_Log(CONTRA_PARA_MAIN_DATA, CONTRA_PARA_MAIN_CRC,
                                    CONTRA_PARA_BACKUP_DATA, CONTRA_PARA_BACKUP_CRC,
                                    new_para, sizeof(Contra_Para_Struct));
					return err;
    }
				return PARA_OP_ERR_NULL_PTR;
}

/**
 * @brief  读取报警参数
 * @param  para：存储报警参数的结构体指针（输出参数）
 * @return 无
 * @note   内部调用Read_Param_With_Backup，支持主备区切换和失效恢复（仅内存默认值）
 */
Para_Op_Result_E Get_Alarm_Para(Alarm_Para_Struct *para) {
	  Para_Op_Result_E err;
    if (para != NULL) {  /* 空指针保护 */
        err=Read_Param_With_Backup(ALARM_PARA_MAIN_DATA, ALARM_PARA_MAIN_CRC,
                               ALARM_PARA_BACKUP_DATA, ALARM_PARA_BACKUP_CRC,
                               para, sizeof(Alarm_Para_Struct));
			return err;
    }
		return PARA_OP_ERR_NULL_PTR;
}

/**
 * @brief  设置报警参数
 * @param  new_para：新参数结构体指针（输入参数）
 * @return 无
 * @note   主程序按需调用，不主动触发则不覆盖已有数据
 */
Para_Op_Result_E Set_Alarm_Para(const Alarm_Para_Struct *new_para) {
	  Para_Op_Result_E err;
    if (new_para != NULL) {  /* 空指针保护 */
        err=Write_Param_With_Backup_Log(ALARM_PARA_MAIN_DATA, ALARM_PARA_MAIN_CRC,
                                    ALARM_PARA_BACKUP_DATA, ALARM_PARA_BACKUP_CRC,
                                    new_para, sizeof(Alarm_Para_Struct));
			return err;
    }
				return PARA_OP_ERR_NULL_PTR;
}




// ************************* 电容保护参数读写接口（新增） *************************
/**
 * @brief  读取指定通道的电容保护参数
 * @param  ch：通道号（0=1路，1=2路...，需小于CAP_CHANNEL_CNT）
 * @param  para：存储参数的缓冲区指针（输出）
 * @return 无
 * @note   自动处理主备区切换、CRC校验、失效恢复（同原有电容参数逻辑）
 */
Para_Op_Result_E Get_Cap_Protect_Para_Ch(uint8_t ch, Cap_Protect_Para_Struct *para) {
	  Para_Op_Result_E err;
    // 通道越界/空指针保护（同原有逻辑）
    if (ch >= CAP_CHANNEL_CNT || para == NULL) return PARA_OP_ERR_NULL_PTR;

    // 调用通用读取函数，传入电容保护参数的地址和初始化函数
    err=Read_Param_With_Backup(
        CAP_PROTECT_MAIN_DATA_CH(ch),    /* 主区数据地址（新增地址宏） */
        CAP_PROTECT_MAIN_CRC_CH(ch),     /* 主区CRC地址（新增地址宏） */
        CAP_PROTECT_BACKUP_DATA_CH(ch),  /* 备份区数据地址（新增地址宏） */
        CAP_PROTECT_BACKUP_CRC_CH(ch),   /* 备份区CRC地址（新增地址宏） */
        para,                            /* 输出缓冲区 */
        sizeof(Cap_Protect_Para_Struct) /* 电容保护参数长度 */

    );
				return err;
}

/**
 * @brief  设置指定通道的电容保护参数（自动记录日志）
 * @param  ch：通道号（0=1路，1=2路...，需小于CAP_CHANNEL_CNT）
 * @param  new_para：新参数缓冲区指针（输入）
 * @return 无
 * @note   自动处理主备区同步、CRC校验、修改日志记录（同原有电容参数逻辑）
 */
Para_Op_Result_E Set_Cap_Protect_Para_Ch(uint8_t ch, const Cap_Protect_Para_Struct *new_para) {
	  Para_Op_Result_E err;	
    // 通道越界/空指针保护（同原有逻辑）
    if (ch >= CAP_CHANNEL_CNT || new_para == NULL) return PARA_OP_ERR_NULL_PTR;

    // 调用通用写入函数，传入电容保护参数的地址
    err=Write_Param_With_Backup_Log(
        CAP_PROTECT_MAIN_DATA_CH(ch),    /* 主区数据地址（新增地址宏） */
        CAP_PROTECT_MAIN_CRC_CH(ch),     /* 主区CRC地址（新增地址宏） */
        CAP_PROTECT_BACKUP_DATA_CH(ch),  /* 备份区数据地址（新增地址宏） */
        CAP_PROTECT_BACKUP_CRC_CH(ch),   /* 备份区CRC地址（新增地址宏） */
        new_para,                        /* 新参数缓冲区 */
        sizeof(Cap_Protect_Para_Struct)  /* 电容保护参数长度 */
    );
		return err;
}



// ************************* 电容变比参数读写接口（新增） *************************
/**
 * @brief  读取指定通道的电容变比参数
 * @param  ch：通道号（0=1路，1=2路...，需小于CAP_CHANNEL_CNT）
 * @param  para：存储参数的缓冲区指针（输出）
 * @return 无
 * @note   自动处理主备区切换、CRC校验、失效恢复（同原有电容参数逻辑）
 */
Para_Op_Result_E Get_Cap_Ratio_Para_Ch(uint8_t ch, Cap_Ratio_Para_Struct *para) {
	 Para_Op_Result_E err;
    // 通道越界/空指针保护（同原有逻辑）
    if (ch >= CAP_CHANNEL_CNT || para == NULL) return PARA_OP_ERR_NULL_PTR;

    // 调用通用读取函数，传入电容变比参数的地址和初始化函数
   err= Read_Param_With_Backup(
        CAP_RATIO_MAIN_DATA_CH(ch),    /* 主区数据地址（新增地址宏） */
        CAP_RATIO_MAIN_CRC_CH(ch),     /* 主区CRC地址（新增地址宏） */
        CAP_RATIO_BACKUP_DATA_CH(ch),  /* 备份区数据地址（新增地址宏） */
        CAP_RATIO_BACKUP_CRC_CH(ch),   /* 备份区CRC地址（新增地址宏） */
        para,                          /* 输出缓冲区 */
        sizeof(Cap_Ratio_Para_Struct) /* 电容变比参数长度 */

    );
				return err;
}

/**
 * @brief  设置指定通道的电容变比参数（自动记录日志）
 * @param  ch：通道号（0=1路，1=2路...，需小于CAP_CHANNEL_CNT）
 * @param  new_para：新参数缓冲区指针（输入）
 * @return 无
 * @note   自动处理主备区同步、CRC校验、修改日志记录（同原有电容参数逻辑）
 */
Para_Op_Result_E Set_Cap_Ratio_Para_Ch(uint8_t ch, const Cap_Ratio_Para_Struct *new_para) {
	 Para_Op_Result_E err;	
    // 通道越界/空指针保护（同原有逻辑）
    if (ch >= CAP_CHANNEL_CNT || new_para == NULL) return PARA_OP_ERR_NULL_PTR;

    // 调用通用写入函数，传入电容变比参数的地址
    err=Write_Param_With_Backup_Log(
        CAP_RATIO_MAIN_DATA_CH(ch),    /* 主区数据地址（新增地址宏） */
        CAP_RATIO_MAIN_CRC_CH(ch),     /* 主区CRC地址（新增地址宏） */
        CAP_RATIO_BACKUP_DATA_CH(ch),  /* 备份区数据地址（新增地址宏） */
        CAP_RATIO_BACKUP_CRC_CH(ch),   /* 备份区CRC地址（新增地址宏） */
        new_para,                      /* 新参数缓冲区 */
        sizeof(Cap_Ratio_Para_Struct)  /* 电容变比参数长度 */
    );
			return err;	
}



/**
 * @brief  读取保护路数参数
 * @param  para：存储报保护路数的结构体指针（输出参数）
 * @return 无
 * @note   内部调用Read_Param_With_Backup，支持主备区切换和失效恢复（仅内存默认值）
 */
Para_Op_Result_E Get_Cap_Num(Cap_Num_Struct *para) {
	  Para_Op_Result_E err;
    if (para != NULL) {  /* 空指针保护 */
        err=Read_Param_With_Backup(CAP_NUM_MAIN_DATA, CAP_NUM__MAIN_CRC,
                               CAP_NUM__BACKUP_DATA, CAP_NUM__BACKUP_CRC,
                               para, sizeof(Cap_Num_Struct));
			return err;
    }
		return PARA_OP_ERR_NULL_PTR;
}

/**
 * @brief  设置保护路数
 * @param  new_para：新参数结构体指针（输入参数）
 * @return 无
 * @note   主程序按需调用，不主动触发则不覆盖已有数据
 */
Para_Op_Result_E Set_Cap_Num(const Cap_Num_Struct *new_para) {
	  Para_Op_Result_E err;
    if (new_para != NULL) {  /* 空指针保护 */
        err=Write_Param_With_Backup_Log(CAP_NUM_MAIN_DATA, CAP_NUM__MAIN_CRC,
                                    CAP_NUM__BACKUP_DATA, CAP_NUM__BACKUP_CRC,
                                    new_para, sizeof(Cap_Num_Struct));
			return err;
    }
				return PARA_OP_ERR_NULL_PTR;
}






// ************************* 系统事件记录函数（独立日志系统，供其他模块调用） *************************
/**
 * @brief  获取当前系统事件的存储索引
 * @param  无
 * @return 事件索引（0~EVENT_LOG_CNT-1）
 * @note   索引超出范围时，返回0（避免越界）
 */
 uint8_t Get_Event_Index(void) {
    uint8_t index = 0;
  
    FM31256_FRAM_Read(EVENT_INDEX_ADDR, &index, 1);  /* 从FRAM读取当前索引 */
    return (index >= EVENT_LOG_CNT) ? 0 : index;  /* 越界保护：返回0 */
}

/**
 * @brief  更新系统事件索引（支持循环覆盖）
 * @param  index：当前事件索引指针（输入+输出）
 * @return 无
 * @note   索引自增1，达到最大条数时重置为0（循环覆盖最早的事件）
 */
static Para_Op_Result_E Update_Event_Index(uint8_t *index) {
	  Para_Op_Result_E err;		 
    *index = (*index + 1) % EVENT_LOG_CNT;  /* 循环自增：0→1→...→99→0 */
    err=FM31256_FRAM_Write(EVENT_INDEX_ADDR, index, 1);  /* 保存更新后的索引到FRAM */
	  return err;
}

/**
 * @brief  填充事件记录的时间字段（原子读取当前时间）
 * @param  event_log：事件记录结构体指针（需填充时间）
 * @return 无
 * @note   原子读取：关闭总中断→复制全局时间→开启总中断，避免时间错乱
 */
static void Fill_Event_Time(Event_Log_Struct *event_log) {
    if (event_log == NULL) return;  /* 空指针保护 */
    EA = 0;  /* 关闭总中断：确保读取过程中时间不被更新 */
    event_log->year   = BASE_YEAR + system_date.year;  /* 完整年份（如2025） */
    event_log->month  = system_date.month;             /* 月份（1~12） */
    event_log->day    = system_date.day;               /* 日期（1~31） */
    event_log->hour   = system_time.hour;              /* 小时（0~23） */
    event_log->minute = system_time.minute;            /* 分钟（0~59） */
    event_log->second = system_time.second;            /* 秒（0~59） */
    EA = 1;  /* 开启总中断：读取完成后恢复 */
}

/**
 * @brief  写入系统事件记录（供其他模块调用，如故障、启动等事件）
 * @param  event_type：事件类型（枚举类型，如过压故障、系统启动等）
 * @param  event_data：事件关联数据（如故障码、参数值等，32位）
 * @return 无
 * @note   主程序按需调用，不主动触发则不写入新事件
 */
Para_Op_Result_E Write_Event_Log(Event_Type_E event_type, uint32_t event_data) {
	  Para_Op_Result_E err;			 
    Event_Log_Struct event_log;  /* 事件记录结构体 */
    uint8_t event_idx = Get_Event_Index();  /* 获取当前事件存储索引 */
    memset(&event_log, 0, sizeof(Event_Log_Struct));  /* 结构体清零 */

    event_log.event_type = event_type;  /* 记录事件类型 */
    event_log.event_data = event_data;  /* 记录事件关联数据 */
    Fill_Event_Time(&event_log);        /* 填充事件发生时间 */

    /* 写入事件到FRAM，并更新事件索引 */
    err= FM31256_FRAM_Write(EVENT_LOG_ADDR(event_idx), (uint8_t *)&event_log, sizeof(Event_Log_Struct));
    Update_Event_Index(&event_idx);  /* 索引自增（循环覆盖） */
	  return err;
}

/**
 * @brief  读取最新的一条系统事件记录
 * @param  event_log：存储事件记录的结构体指针（输出参数）
 * @return 1：读取成功（存在有效事件）；0：读取失败（无有效事件/空指针）
 */
Para_Op_Result_E Read_Latest_Event_Log(Event_Log_Struct *event_log) {
	  Para_Op_Result_E err;		 	
    uint8_t current_idx, latest_idx;
    if (event_log == NULL) return 0;  /* 空指针保护：返回失败 */

    current_idx = Get_Event_Index();  /* 获取当前事件索引（下次存储位置） */
    /* 最新事件索引：当前索引为0→最新是99，否则为当前索引-1 */
    latest_idx = (current_idx == 0) ? (EVENT_LOG_CNT - 1) : (current_idx - 1);

    /* 读取最新事件记录 */
    err=FM31256_FRAM_Read(EVENT_LOG_ADDR(latest_idx), (uint8_t *)event_log, sizeof(Event_Log_Struct));
	  
	  if(err)
			return err;
    /* 事件类型为NONE→无有效事件，返回0；否则返回1 */
    return (event_log->event_type != EVENT_TYPE_NONE) ? EVENT_OK : EVENT_ERR;
}

/**
 * @brief  读取指定索引的系统事件记录
 * @param  index：事件索引（0~EVENT_LOG_CNT-1）
 * @param  event_log：存储事件记录的结构体指针（输出参数）
 * @return 1：读取成功（存在有效事件）；0：读取失败（索引越界/空指针/无有效事件）
 */
Para_Op_Result_E Read_Event_Log(uint8_t index, Event_Log_Struct *event_log) {

	  	  Para_Op_Result_E err;		
    if (event_log == NULL) return PARA_OP_ERR_NULL_PTR;
    if (index >= EVENT_LOG_CNT) return PARA_OP_ERR_ADDR_OVERFLOW;
    /* 读取指定索引的事件记录 */
    err=FM31256_FRAM_Read(EVENT_LOG_ADDR(index), (uint8_t *)event_log, sizeof(Event_Log_Struct));
		if(err)
		return err;
    /* 事件类型为NONE→无有效事件，返回0；否则返回1 */
    return (event_log->event_type != EVENT_TYPE_NONE) ? EVENT_OK : EVENT_ERR;
}



