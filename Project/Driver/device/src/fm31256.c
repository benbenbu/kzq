#include "config.h"
#include "fm31256.h"

#include "i2c.h"


/**
 * @brief  从18h~11h寄存器读取版本号
 * @param  ver：输出，版本号结构体
 * @return 0=成功，1=失败
 */
uint8_t FM31256_Read_Version(Version_Struct *ver) {
    uint8_t ret = 0;	
    if (ver == NULL) return 1;

    // 序列号寄存器已锁定为只读（SNL=1），直接读取
    ret = FM31256_Reg_Read(SN_VER_MAIN_ADDR, &ver->main_ver);
    ret |= FM31256_Reg_Read(SN_VER_SUB_ADDR, &ver->sub_ver);
    ret |= FM31256_Reg_Read(SN_HW_VER_ADDR, &ver->hw_ver);
    return ret;
}

/**
 * @brief  从18h~11h寄存器写版本号
 * @param  ver：输出，版本号结构体
 * @return 0=成功，1=失败
 */
uint8_t FM31256_Write_Version(Version_Struct *ver) {
    uint8_t ret = 0;	
    if (ver == NULL) return 1;

    // 序列号寄存器已锁定为只读（SNL=1），直接读取
    ret =  FM31256_Reg_Write(SN_VER_MAIN_ADDR, VER_MAIN);
    ret |= FM31256_Reg_Write(SN_VER_SUB_ADDR, VER_SUB);
    ret |= FM31256_Reg_Write(SN_HW_VER_ADDR, HW_VERSION);
    return ret;
}


// --------------- FRAM存储函数实现 ---------------
/**
 * @brief  读取FRAM数据（支持连续读）
 * @param  addr: 存储地址（0x0000~0x7FFF）
 * @param  buf: 接收缓冲区（需提前分配内存）
 * @param  len: 读取长度（1~64字节，不跨页）
 * @retval 错误码（FM31256_OK/ERR_PARAM/ERR_I2C）
 */
uint8_t FM31256_FRAM_Read(uint16_t addr, uint8_t *buf, uint16_t len) {
    uint8_t err;
	  uint16_t i;
    char SFRPAGE_SAVE = SFRPAGE;
    SFRPAGE = SMB0_PAGE;	
    // 校验参数合法性
    if (buf == NULL || len == 0 || addr > 0x7FFF) 
		{
						SFRPAGE = SFRPAGE_SAVE;
			return FM31256_ERR_PARAM;
		}
    // 第一步：发送FRAM存储地址（写模式）
    err = I2C_Start();
    if (err != I2C_SUCCESS){
			SFRPAGE = SFRPAGE_SAVE;
			return FM31256_ERR_I2C;
		}
    // 发送FM31256写地址
    err = I2C_SendAddr(FM31256_SLAVE_W);
    if (err != I2C_SUCCESS) {
        I2C_Stop();
						SFRPAGE = SFRPAGE_SAVE;
        return FM31256_ERR_I2C;
    }

    // 发送存储地址高字节
    err = I2C_SendData((addr >> 8) & 0xFF);
    if (err != I2C_SUCCESS) {
        I2C_Stop();
						SFRPAGE = SFRPAGE_SAVE;
        return FM31256_ERR_I2C;
    }

    // 发送存储地址低字节
    err = I2C_SendData(addr & 0xFF);
    if (err != I2C_SUCCESS) {
        I2C_Stop();
						SFRPAGE = SFRPAGE_SAVE;
        return FM31256_ERR_I2C;
    }

    // 第二步：重复起始，切换到读模式
    err = I2C_RepeatStart();
    if (err != I2C_SUCCESS) {
        I2C_Stop();
						SFRPAGE = SFRPAGE_SAVE;
        return FM31256_ERR_I2C;
    }

    // 发送FM31256读地址
    err = I2C_SendAddr(FM31256_SLAVE_R);
    if (err != I2C_SUCCESS) {
        I2C_Stop();
				SFRPAGE = SFRPAGE_SAVE;
        return FM31256_ERR_I2C;
    }

    // 第三步：连续读取数据（最后1字节发NACK，其余发ACK）
    for ( i = 0; i < len; i++) {
        uint8_t ack = (i == len - 1) ? 0 : 1;  // 最后1字节NACK，其他ACK
        err = I2C_RecvData(ack, &buf[i]);      // 适配双参数I2C_RecvData
        if (err != I2C_SUCCESS) {
            I2C_Stop();
					  SFRPAGE = SFRPAGE_SAVE;
            return FM31256_ERR_I2C;
        }
    }

    // 第四步：发送停止条件
    I2C_Stop();
		SFRPAGE = SFRPAGE_SAVE;
    return FM31256_OK;
}

/**
 * @brief  写入FRAM数据（支持连续写，最大64字节/页）
 * @param  addr: 存储地址（0x0000~0x7FFF）
 * @param  buf: 待写数据缓冲区
 * @param  len: 写入长度（1~64字节，避免跨页）
 * @retval 错误码（FM31256_OK/ERR_PARAM/ERR_I2C）
 */
uint8_t FM31256_FRAM_Write(uint16_t addr, const uint8_t *buf, uint16_t len) {
    uint8_t err;
	  uint16_t i;
    char SFRPAGE_SAVE = SFRPAGE;
    SFRPAGE = SMB0_PAGE;	
    // 校验参数合法性
    if (buf == NULL || len == 0 || addr > 0x7FFF || len > 64) 
		{
			SFRPAGE = SFRPAGE_SAVE;
			return FM31256_ERR_PARAM;
		}

    // 1. 起始条件
    err = I2C_Start();
    if (err != I2C_SUCCESS){
			  SFRPAGE = SFRPAGE_SAVE;
			return FM31256_ERR_I2C;
		}

    // 2. 发送FM31256写地址
    err = I2C_SendAddr(FM31256_SLAVE_W);
    if (err != I2C_SUCCESS) {
        I2C_Stop();
			  SFRPAGE = SFRPAGE_SAVE;
        return FM31256_ERR_I2C;
    }

    // 3. 发送存储地址高字节
    err = I2C_SendData((addr >> 8) & 0xFF);
    if (err != I2C_SUCCESS) {
        I2C_Stop();
			  SFRPAGE = SFRPAGE_SAVE;
        return FM31256_ERR_I2C;
    }

    // 4. 发送存储地址低字节
    err = I2C_SendData(addr & 0xFF);
    if (err != I2C_SUCCESS) {
        I2C_Stop();
			  SFRPAGE = SFRPAGE_SAVE;
        return FM31256_ERR_I2C;
    }

    // 5. 连续发送数据（最多64字节）
    for ( i = 0; i < len; i++) {
        err = I2C_SendData(buf[i]);
        if (err != I2C_SUCCESS) {
            I2C_Stop();
					  SFRPAGE = SFRPAGE_SAVE;
            return FM31256_ERR_I2C;
        }
    }

    // 6. 停止条件
    I2C_Stop();
		SFRPAGE = SFRPAGE_SAVE;
    return FM31256_OK;
}

























/**
 * @brief  读取寄存器数据
 * @param  addr: 地址（0x00~0x18）
 * @param  byte: 数据
 * @retval 错误码（FM31256_OK/ERR_PARAM/ERR_I2C）
 */
uint8_t FM31256_Reg_Read(uint8_t addr, uint8_t *byte) {
    uint8_t err;
    char SFRPAGE_SAVE = SFRPAGE;
    SFRPAGE = SMB0_PAGE;	
    // 第一步：发送FRAM存储地址（写模式）
    err = I2C_Start();
    if (err != I2C_SUCCESS){
			SFRPAGE = SFRPAGE_SAVE;
			return FM31256_ERR_I2C;
		}
    // 发送FM31256写地址
    err = I2C_SendAddr(FM31256_COM_SLAVE_W);
    if (err != I2C_SUCCESS) {
        I2C_Stop();
		SFRPAGE = SFRPAGE_SAVE;
        return FM31256_ERR_I2C;
    }

    // 发送存储地址高字节
    err = I2C_SendData(addr );
    if (err != I2C_SUCCESS) {
        I2C_Stop();
		SFRPAGE = SFRPAGE_SAVE;
        return FM31256_ERR_I2C;
    }
    // 第二步：重复起始，切换到读模式
    err = I2C_RepeatStart();
    if (err != I2C_SUCCESS) {
        I2C_Stop();
						SFRPAGE = SFRPAGE_SAVE;
        return FM31256_ERR_I2C;
    }

    // 发送FM31256读地址
    err = I2C_SendAddr(FM31256_COM_SLAVE_R);
    if (err != I2C_SUCCESS) {
        I2C_Stop();
		SFRPAGE = SFRPAGE_SAVE;
        return FM31256_ERR_I2C;
    }

    // 第三步：连续读取数据（最后1字节发NACK，其余发ACK）

        err = I2C_RecvData(0, byte);      // 适配双参数I2C_RecvData
        if (err != I2C_SUCCESS) {
            I2C_Stop();
					  SFRPAGE = SFRPAGE_SAVE;
            return FM31256_ERR_I2C;
        }
    // 第四步：发送停止条件
    I2C_Stop();
	SFRPAGE = SFRPAGE_SAVE;
    return FM31256_OK;
}


/**
 * @brief  写入寄存器数据
 * @param  addr: 存储地址（0x0000~0x7FFF）
 * @param  byte: 待写数据字节
 * @retval 错误码（FM31256_OK/ERR_PARAM/ERR_I2C）
 */
uint8_t FM31256_Reg_Write(uint8_t addr,  uint8_t byte) {
    uint8_t err;
    char SFRPAGE_SAVE = SFRPAGE;
    SFRPAGE = SMB0_PAGE;	
    // 1. 起始条件
    err = I2C_Start();
    if (err != I2C_SUCCESS){
			  SFRPAGE = SFRPAGE_SAVE;
			return FM31256_ERR_I2C;
		}

    // 2. 发送FM31256写地址
    err = I2C_SendAddr(FM31256_COM_SLAVE_W);
    if (err != I2C_SUCCESS) {
        I2C_Stop();
			  SFRPAGE = SFRPAGE_SAVE;
        return FM31256_ERR_I2C;
    }

    // 3. 发送存储地址高字节
    err = I2C_SendData(addr );
    if (err != I2C_SUCCESS) {
        I2C_Stop();
			  SFRPAGE = SFRPAGE_SAVE;
        return FM31256_ERR_I2C;
    }

        err = I2C_SendData(byte);
        if (err != I2C_SUCCESS) {
            I2C_Stop();
					  SFRPAGE = SFRPAGE_SAVE;
            return FM31256_ERR_I2C;
				}
    // 6. 停止条件
    I2C_Stop();
	SFRPAGE = SFRPAGE_SAVE;
    return FM31256_OK;
}



// --------------- 1/4写保护---------------

uint8_t FM31256_Write_Protect(uint8_t onf)
{

    uint8_t err;
    if(onf)
		err=FM31256_Reg_Write(FM31256_REG_COMPANION_CTL,0x08);//打开1/4写保护  ，2.6v低电复位
    else
		err=FM31256_Reg_Write(FM31256_REG_COMPANION_CTL,0x00);//关闭写保护  ，2.6v低电复位			
    return err;


}






// --------------- RTC时间函数实现（适配Time_Struct）---------------
/**
 * @brief  读取RTC当前日期（BCD转十进制，存入用户结构体）
 * @param  date: 输出参数（用户的Date_Struct指针）
 * @retval 错误码（FM31256_OK/ERR_PARAM/ERR_I2C）
 */
uint8_t FM31256_RTC_Read_Date(Date_Struct *date) {
    uint8_t err;
    uint8_t rtc_buf[3];  

    if (date == NULL) return FM31256_ERR_PARAM;

    // 2. R位置1（捕获RTC静态值到寄存器，避免读时跳变）
		err=FM31256_Reg_Write(FM31256_RTC_CTRL,RTC_CTRL_R_MASK);

    // --------------------------------------------------------------------------------------------------
		err=FM31256_Reg_Read(FM31256_RTC_DAY,&rtc_buf[0]);
		err=FM31256_Reg_Read(FM31256_RTC_MONTH,&rtc_buf[1]);
		err=FM31256_Reg_Read(FM31256_RTC_YEAR,&rtc_buf[2]);  
    // 1. R位置0（准备捕获，必须先置0才能触发下次捕获）
		err=FM31256_Reg_Write(FM31256_RTC_CTRL,0);
    // -------------------------- 修正：数组映射（贴合手册寄存器顺序）--------------------------
    // rtc_buf[0] = 0x02（秒）、rtc_buf[1] = 0x03（分）、rtc_buf[2] = 0x04（时）
    // rtc_buf[3] = 0x05（星期）、rtc_buf[4] = 0x06（日）、rtc_buf[5] = 0x07（月）、rtc_buf[6] = 0x08（年）
    date->day    = BCD_TO_DEC(rtc_buf[0]);  // 日对应rtc_buf[4]（原buf[3]是星期）
    date->month  = BCD_TO_DEC(rtc_buf[1]);  // 月对应rtc_buf[5]（原buf[4]是日）
    date->year   = BCD_TO_DEC(rtc_buf[2]);  // 年对应rtc_buf[6]（保留原有）
    // --------------------------------------------------------------------------------------------------
    return err;
}
/**
 * @brief  读取RTC当前时间（BCD转十进制，存入用户结构体）
 * @param  time: 输出参数（用户的Date_Struct指针）
 * @retval 错误码（FM31256_OK/ERR_PARAM/ERR_I2C）
 */
uint8_t FM31256_RTC_Read_Time(Time_Struct *time) {
    uint8_t err;
    uint8_t rtc_buf[3];  

    if (time == NULL) return FM31256_ERR_PARAM;

    // 2. R位置1（捕获RTC静态值到寄存器，避免读时跳变）
		err=FM31256_Reg_Write(FM31256_RTC_CTRL,RTC_CTRL_R_MASK);
    // --------------------------------------------------------------------------------------------------
		err=FM31256_Reg_Read(FM31256_RTC_SEC,&rtc_buf[0]);
		err=FM31256_Reg_Read(FM31256_RTC_MIN,&rtc_buf[1]);
		err=FM31256_Reg_Read(FM31256_RTC_HOUR,&rtc_buf[2]);  
    // 1. R位置0（准备捕获，必须先置0才能触发下次捕获）
		err=FM31256_Reg_Write(FM31256_RTC_CTRL,0);

    // -------------------------- 修正：数组映射（贴合手册寄存器顺序）--------------------------
    // rtc_buf[0] = 0x02（秒）、rtc_buf[1] = 0x03（分）、rtc_buf[2] = 0x04（时）
    // rtc_buf[3] = 0x05（星期）、rtc_buf[4] = 0x06（日）、rtc_buf[5] = 0x07（月）、rtc_buf[6] = 0x08（年）
    time->second = BCD_TO_DEC(rtc_buf[0]);  // 秒对应rtc_buf[0]（原buf[2]是时）
    time->minute = BCD_TO_DEC(rtc_buf[1]);  // 分对应rtc_buf[1]（原buf[3]是秒）
    time->hour   = BCD_TO_DEC(rtc_buf[2]);  // 时对应rtc_buf[2]（原buf[4]是分）
    // --------------------------------------------------------------------------------------------------
    return err;
}

static uint8_t Date_Validate(Date_Struct *date)
{
	     uint8_t max_day = 31;
    // 基础范围校验
    if (date->month < 1 || date->month > 12) {
        return 1;  // 月份非法
    }
    
    if (date->day < 1) {
        return 1;  // 日期小于1
    }

    // 各月份最大日期

    switch(date->month) {
        case 2:  // 2月，处理闰年
            // 闰年判断：能被4整除且不是整百年，或能被400整除（这里只处理0-99年，简化为能被4整除）
            if (((date->year % 4) == 0) && (date->year != 0)) {
                max_day = 29;
            } else {
                max_day = 28;
            }
            break;
        case 4: case 6: case 9: case 11:  // 小月
            max_day = 30;
            break;
        default:  // 大月
            max_day = 31;
            break;
    }

    // 日期超出当月最大天数
    if (date->day > max_day) {
        return 1;
    }

    // 年份范围校验（根据FM31256实际支持范围调整，通常是0-99）
    if (date->year > 99) {
        return 1;
    }

    return 0;  // 日期合法
}
/**
 * @brief FM31256 RTC日期写入函数（增加日期校验）
 * @param date 日期结构体指针
 * @return 错误码：0-成功，FM31256_ERR_PARAM-参数空，FM31256_ERR_DATE-日期非法，其他-寄存器操作错误
 */
uint8_t FM31256_RTC_Write_Date(Date_Struct *date) {
    uint8_t err;
    uint8_t rtc_buf[3];
    
    // 1. 参数空指针校验
    if (date == NULL) {
        return FM31256_ERR_PARAM;
    }

    // 2. 新增：日期合法性校验
    err = Date_Validate(date);
    if (err != 0) {
        return err;  // 日期非法，直接返回错误
    }

    // 3. 十进制转BCD，填充RTC缓冲区
    rtc_buf[0] = DEC_TO_BCD(date->day);    // 日期
    rtc_buf[1] = DEC_TO_BCD(date->month);  // 月份
    rtc_buf[2] = DEC_TO_BCD(date->year);   // 年份

    // 4. W位置1（冻结RTC，允许写时间寄存器）
    err = FM31256_Reg_Write(FM31256_RTC_CTRL, RTC_CTRL_W_MASK);
    if (err != 0) {
        return err;  // 写入控制寄存器失败，返回错误
    }

    // 注：原代码此处是读寄存器，应该是笔误，修正为写寄存器
    err = FM31256_Reg_Write(FM31256_RTC_DAY, rtc_buf[0]);
    if (err != 0) return err;
    
    err = FM31256_Reg_Write(FM31256_RTC_MONTH, rtc_buf[1]);
    if (err != 0) return err;
    
    err = FM31256_Reg_Write(FM31256_RTC_YEAR, rtc_buf[2]);  
    if (err != 0) return err;

    // 5. R位置0（准备捕获，必须先置0才能触发下次捕获）
    err = FM31256_Reg_Write(FM31256_RTC_CTRL, 0);

    return err;
}
/**
 * @brief 时间合法性校验函数
 * @param time 时间结构体指针
 * @return 错误码：FM31256_OK-合法，FM31256_ERR_TIME-非法
 */
static uint8_t Time_Validate(Time_Struct *time)
{
    // 校验小时范围（24小时制）
    if (time->hour > 23) {
        return 1;
    }
    
    // 校验分钟范围
    if (time->minute > 59) {
        return 1;
    }
    
    // 校验秒范围
    if (time->second > 59) {
        return 1;
    }
    
    return FM31256_OK;  // 时间合法
}

/**
 * @brief  写入RTC时间（十进制转BCD，从用户结构体读取）
 * @param  time: 输入参数（用户的Time_Struct指针）
 * @retval 错误码（FM31256_OK/ERR_PARAM/ERR_TIME/ERR_I2C）
 */
uint8_t FM31256_RTC_Write_Time( Time_Struct *time) {
    uint8_t err;
    uint8_t rtc_buf[3];
    
    // 1. 空指针参数校验
    if (time == NULL) {
        return FM31256_ERR_PARAM;
    }

    // 2. 新增：时间合法性校验
    err = Time_Validate(time);
    if (err != FM31256_OK) {
        return err;  // 时间非法，直接返回错误
    }

    // 3. 十进制转BCD，填充RTC缓冲区
    rtc_buf[0] = DEC_TO_BCD(time->second);  // 0x02：秒（BCD）
    rtc_buf[1] = DEC_TO_BCD(time->minute);  // 0x03：分（BCD）
    rtc_buf[2] = DEC_TO_BCD(time->hour);    // 0x04：时（BCD，24小时制）

    // 4. W位置1（冻结RTC，允许写时间寄存器）
    err = FM31256_Reg_Write(FM31256_RTC_CTRL, RTC_CTRL_W_MASK);
    if (err != FM31256_OK) {
        return FM31256_ERR_I2C;  // I2C写失败
    }

    // 5. 修正原代码逻辑错误：将读寄存器改为写寄存器（核心修正）
    err = FM31256_Reg_Write(FM31256_RTC_SEC, rtc_buf[0]);
    if (err != FM31256_OK) return FM31256_ERR_I2C;
    
    err = FM31256_Reg_Write(FM31256_RTC_MIN, rtc_buf[1]);
    if (err != FM31256_OK) return FM31256_ERR_I2C;
    
    err = FM31256_Reg_Write(FM31256_RTC_HOUR, rtc_buf[2]);  
    if (err != FM31256_OK) return FM31256_ERR_I2C;

    // 6. R位置0（准备捕获，必须先置0才能触发下次捕获）
    err = FM31256_Reg_Write(FM31256_RTC_CTRL, 0);
    if (err != FM31256_OK) {
        return FM31256_ERR_I2C;
    }

    return FM31256_OK;  // 全部操作成功
}





// --------------- 看门狗函数实现 ---------------
/**
 * @brief  启用FM31256看门狗
 * @param  timeout: 溢出时间（WDG_1S~WDG_32S）
 * @retval 错误码（FM31256_OK/ERR_PARAM/ERR_I2C）
 */
uint8_t FM31256_WDG_Enable(uint8_t timeout) {
    uint8_t err;
		uint8_t wdg_cfg;
    if (timeout > WDG_32S) return FM31256_ERR_PARAM;
    wdg_cfg = 0x80 | (timeout & 0x07);	
		err=FM31256_Reg_Write(FM31256_REG_WDT_CTRL,wdg_cfg);	
    return err;
}

/**
 * @brief  禁用FM31256看门狗
 * @retval 错误码（FM31256_OK/ERR_I2C）
 */
uint8_t FM31256_WDG_Disable(void) {
    uint8_t err;	
		err=FM31256_Reg_Write(FM31256_REG_WDT_CTRL,0x00);	
    return err;
}

/**
 * @brief  看门狗喂狗（溢出前调用，避免复位）
 * @retval 错误码（FM31256_OK/ERR_I2C）
 */
uint8_t FM31256_WDG_Feed(void) {
    uint8_t err;
		err=FM31256_Reg_Write(FM31256_REG_WDT_RESTART,0x0A);		
    return err;
}