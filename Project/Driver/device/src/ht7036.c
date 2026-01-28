

#include "HT7036.h"
#include "spi.h"
#include "fm31256.h"
#include "data_save.h"
#include "delay.h"
#include "data_deal.h"
/******
*
*
*  为了不使用浮点数计算，采集的4路电流寄存器值扩大1000倍，最小分辨率1ma，采集的4路零序电压扩大10倍，最小分辨率0.1v
*  1uo-g_meter_chip[0].v2 ua   1ia-g_meter_chip[0].v1 ia    1ic-g_meter_chip[0].v3  ib  
*  2uo-g_meter_chip[0].v4 ub   2ia-g_meter_chip[0].v5  ic   2ic-g_meter_chip[0].v6  uc

*  3uo-g_meter_chip[1].v2    3ia-g_meter_chip[1].v1     3ic-g_meter_chip[1].v3    
*  4uo-g_meter_chip[1].v4    4ia-g_meter_chip[1].v5     4ic-g_meter_chip[1].v6  

*/





MeterChipStatus  g_meter_chip[4]={0};//2不用，0-3对应4路芯片


/* 内部函数 */

static void Ht_7036_Rest(void);
static char Ht7036_Check(uint8_t num);
static uint8_t Ht7036_Spi_Read(uint8_t num ,uint8_t address,u32 *read_data);
static  uint8_t Ht7036_Spi_Write(uint8_t num,unsigned char address, unsigned long write_data);
static uint8_t Ht7036_Spi_Read_Check(uint8_t num ,uint8_t address,u32 *read_data);
static  uint8_t Ht7036_Spi_Write_Check(uint8_t num,unsigned char address, unsigned long write_data);


static uint8_t Ht7036_Comcheck(uint8_t num, uint32_t rx_data, uint8_t cmd);
static uint8_t Ht7036_Read_BCKREG(uint8_t num, uint32_t *bck_data);

//static void Ht7036_Put_Data(Phase a, Phase_DataTypeDef *Phase_Datatypedef);
//static void Ht7036_Phase_Zero(Phase phase);
//static void Ht7036_Phase_Gain(Phase phase);
//static void Ht7036_Phase_Power(Phase phase);
//static void Ht7036_Phase_Phase(Phase phase);








/**
 * @description:  计量芯片初始化
 * @return {*}  0：正常，1，失败
 * @Date: 2024-01-10 15:12:47
 */

uint8_t Ht7036_init(void)
{
  uint8_t err=OK;
	uint8_t num=3;;
	Ht_7036_Rest();

	while(num--)
	{
  err|=Ht7036_Config(1);
	if(g_cap_num.cap_num>2)
  err|=Ht7036_Config(2);	
//  err=Ht7036_Config(3);	
  err|=Ht7036_Config(4);	
		if(err==0)
			break;
	err=0;
	}
	if(num>0)
		return 0;//成功
	else
		return 1;//失败

}


void Ht7036_read(char num)
{
    uint8_t err = 0;
    u32 flag = 0; 
    u32 flag1 = 0; 
    u32 flag2 = 0; 	
    MeterChipStatus *p_chip = &g_meter_chip[num-1];
    p_chip->data_ready = 0; // 每次读取先清零就绪标志

    if (num < 1 || num > 4) {
        // 无效芯片编号，直接返回（避免访问g_meter_chip[0]或g_meter_chip[5+]）
        return;
    }
    // ------------------------ 7036芯片处理（num=1/2） ------------------------
    if (num < 3) {
        err |= Ht7036_Spi_Read_Check(num, Checksum_Register1, &flag1);

        // 1. SPI通信错误处理
        if (err) {
            p_chip->err_flag |= 0x01; // 标记SPI通信错误
            return;
        }			
        err |= Ht7036_Spi_Read_Check(num, Checksum_Register2, &flag2);
        // 1. SPI通信错误处理
        if (err) {
            p_chip->err_flag |= 0x01; // 标记SPI通信错误
            return;
        }				
			  if((p_chip->check1!=flag1)||(p_chip->check2!=flag2))
				{
			      p_chip->err_flag |= 0x02; // 校表错误
//			      return;
				}
        // 读取7036状态标志寄存器（INTFlag）
        err |= Ht7036_Spi_Read_Check(num, r_INTFlag, &flag);

        // 1. SPI通信错误处理
        if (err) {
            p_chip->err_flag |= 0x01; // 标记SPI通信错误
            return;
        }

        // 2.1 未校表标志（0x01）
        if (flag & HT7036_FLAG_UNCALIB) {
            p_chip->err_flag |= 0x02; // 标记未校表
//					  return;
        }

        // 2.2 有效值更新标志（0x02）
        if (flag & HT7036_FLAG_VALID_UPD) {
            // 读取前清零数据，避免脏数据残留
            p_chip->u_a = 0;
            p_chip->u_b = 0;
            p_chip->u_c = 0;
            p_chip->i_a = 0;
            p_chip->i_b = 0;
            p_chip->i_c = 0;

            // 修正：传寄存器地址+指针参数（核心错误修复）
            err |= Ht7036_Spi_Read_Check(num, r_UaRms, &(p_chip->u_a)); // A相电压
            err |= Ht7036_Spi_Read_Check(num, r_UbRms, &(p_chip->u_b)); // B相电压（修正地址）
            err |= Ht7036_Spi_Read_Check(num, r_UcRms, &(p_chip->u_c)); // C相电压（修正地址）
            err |= Ht7036_Spi_Read_Check(num, r_IaRms, &(p_chip->i_a)); // A相电流（修正地址）
            err |= Ht7036_Spi_Read_Check(num, r_IbRms, &(p_chip->i_b)); // B相电流（修正地址）
            err |= Ht7036_Spi_Read_Check(num, r_IcRms, &(p_chip->i_c)); // C相电流（修正地址）

            // SPI读取数据错误处理
            if (err) {
                p_chip->err_flag |= 0x01; // 标记SPI通信错误
                return;
            }

            p_chip->data_ready = 1; // 标记数据就绪
        }
    // ------------------------ 7053芯片处理（num=3/4） ------------------------
    } else {
		
        err |= Ht7036_Spi_Read_Check(num, SUMChecksum_Add_7053, &flag1);

        // 1. SPI通信错误处理
        if (err&READ_ERR) {
            p_chip->err_flag |= 0x01; // 标记SPI通信错误
            return;
        }			
			
			  if(p_chip->check2!=flag1)
				{
			      p_chip->err_flag |= 0x02; // 校表错误
//			      return;
				}
        // 读取7053状态标志寄存器（EMUIF）
        err |= Ht7036_Spi_Read_Check(num, EMUIF, &flag);

        // 1. SPI通信错误处理
        if (err&READ_ERR) {
            p_chip->err_flag |= 0x01; // 标记SPI通信错误
            return;
        }

        // 2.3 有效值更新标志（0x80）
        if (flag & HT7053_FLAG_VALID_UPD) {
            // 读取前清零数据，避免脏数据残留
            p_chip->u_a = 0;
            p_chip->i_a = 0;
            p_chip->rp = 0;
            p_chip->rq = 0;

            // 修正：传寄存器地址+指针参数（核心错误修复）
            err |= Ht7036_Spi_Read_Check(num, Rms_I1, &(p_chip->i_a));  // 电流有效值
            err |= Ht7036_Spi_Read_Check(num, Rms_U, &(p_chip->u_a));   // 电压有效值
            err |= Ht7036_Spi_Read_Check(num, PowerP1, &(p_chip->rp));  // 有功功率
            err |= Ht7036_Spi_Read_Check(num, PowerQ1, &(p_chip->rq));  // 无功功率

            // SPI读取数据错误处理
            if (err&READ_ERR) {
                p_chip->err_flag |= 0x01; // 标记SPI通信错误
                return;
            }

            p_chip->data_ready = 1; // 标记数据就绪
        }
    }
}







/**
 * @description: ht7036初始化
 * @param {u8} type
 * @return {*}  0x11校表数据不对，
 * @Author: num：1-4 
 * @Date: 2024-01-10 16:46:58
 */
char Ht7036_Config(u8 num)
{
    unsigned char dad[40];
    uint8_t err=0;
	  uint32_t check1,check2;//fm31256存的校表和
	  uint32_t check3,check4;	//计量芯片读出的校表和  
    uint8_t len;
	  FloatToBytesUnion a={0};
//    err|=FM31256_Write_Protect(0);//关闭写保护	
	
    if(num<3)
			len=HT7036_CALIB_LEN;
		else
			len=HT7053_CALIB_LEN;			
	
	  err|=FM31256_Read_Calib( num,dad,len,&check1,&check2);

	  if(num<3)
		{
		err|=Ht7036_Spi_Write_Check(num,Adjust_Write, Adjust_Enable);      // 打开校准数据写
		err|=Ht7036_Spi_Write_Check(num,w_ModeCfg, 0xA87EUL);   // 填写模式配置寄存器      1.8Mhz 更新速率28.8，稳定慢速，外部ub
		err|=Ht7036_Spi_Write_Check(num,w_EMCfg, 0x0000UL);     // 算法控制器 三相四线夹角采用算法2
		err|=Ht7036_Spi_Write_Check(num,w_EMUCfg, 0x0904UL);    // 填写EMU单元配置寄存器  关闭基波谐波测量  采样28.8k，高通增益校正，
		err|=Ht7036_Spi_Write_Check(num,w_EMUIE, 0x03UL);     // 开启数据更新中断
		err|=Ht7036_Spi_Write_Check(num,w_ModuleCFG, 0x3C27); // 填写模拟模块使能寄存器  开启高通，低速spi
		err|=Ht7036_Spi_Write_Check(num,w_PGACtrl, 0x0000);   // 各路ADC增益均为1
		err|=Ht7036_Spi_Write_Check(num,w_Hfconst, Meter_HFConst);
		err|=Ht7036_Spi_Write_Check(num,w_TCcoffA, 0xFF00); //
		err|=Ht7036_Spi_Write_Check(num,w_TCcoffB, 0x0DB8); //
		err|=Ht7036_Spi_Write_Check(num,w_TCcoffC, 0xD1DA); //

    err|=Ht7036_Spi_Write(num,w_UaRmsoffse, (uint16_t)dad[0] << 8 | dad[1]);
    err|=Ht7036_Spi_Write(num,w_UbRmsoffse, (uint16_t)dad[2] << 8 | dad[3]);		
    err|=Ht7036_Spi_Write(num,w_UcRmsoffse, (uint16_t)dad[4] << 8 | dad[5]);		
    err|=Ht7036_Spi_Write(num,w_IaRmsoffse, (uint16_t)dad[6] << 8 | dad[7]);
    err|=Ht7036_Spi_Write(num,w_IbRmsoffse, (uint16_t)dad[8] << 8 | dad[9]);
    err|=Ht7036_Spi_Write(num,w_IcRmsoffse, (uint16_t)dad[10] << 8 | dad[11]);
    err|=Ht7036_Spi_Write(num,w_UgainA, (uint16_t)dad[12] << 8 | dad[13]);
    err|=Ht7036_Spi_Write(num,w_UgainB, (uint16_t)dad[14] << 8 | dad[15]);
    err|=Ht7036_Spi_Write(num,w_UgainC, (uint16_t)dad[16] << 8 | dad[17]);
    err|=Ht7036_Spi_Write(num,w_IgainA, (uint16_t)dad[18] << 8 | dad[19]);
    err|=Ht7036_Spi_Write(num,w_IgainB, (uint16_t)dad[20] << 8 | dad[21]);
    err|=Ht7036_Spi_Write(num,w_IgainC, (uint16_t)dad[22] << 8 | dad[23]);		
    err|=Ht7036_Spi_Write_Check(num,Adjust_Write, Adjust_Disable); // 关闭校准数据写	
		
//    err|=Ht7036_Spi_Write_Check(num,Adjust_Read, Adjust_Disable); // 使能读计量数据				
//    err|=Ht7036_Spi_Write_Check(num,Adjust_Read, Adjust_Enable); // 使能读校表数据				
    err|=Ht7036_Spi_Read_Check(num,Checksum_Register1, &check3); // 读校表和		
    err|=Ht7036_Spi_Read_Check(num,Checksum_Register2, &check4); // 读校表和		
		
		if((check3!=check1)&&(check4!=check2))
			return err|0x10;
		g_meter_chip[num-1].check1=check1;
		g_meter_chip[num-1].check2=check2;		
		
	}
		else
		{
		err|=Ht7036_Spi_Write_Check(num,EMUIE, 0x0800UL);      // 有效值更新中断 
		err|=Ht7036_Spi_Write_Check(num,WPREG, WRITE_OPEN_1);      // 打开写保护1 ，
		err|=Ht7036_Spi_Write_Check(num,EMUCFG, 0x2000UL);   // 电压正向过零， 通道1
		err|=Ht7036_Spi_Write_Check(num,FreqCFG, 0x3BUL);    // 采样频率7.2k，有效值更新27.4k  1.8m频率
		err|=Ht7036_Spi_Write_Check(num,ModuleEn, 0x007EUL);     // 开启高通，开启能量计量
		err|=Ht7036_Spi_Write_Check(num,ANAEN, 0x003B); // 开启第一路电压电流
		err|=Ht7036_Spi_Write_Check(num,ADCCON, 0x0000);   // 电压电流ADC增益均为1
		err|=Ht7036_Spi_Write_Check(num,WPREG, WRITE_OPEN_2);      // 打开写保护1 ，			
		err|=Ht7036_Spi_Write_Check(num,Hfconst, 0x0040);

			

	
		err|=Ht7036_Spi_Write_Check(num,WPREG, 0);      // 关闭写保护	
    err|=Ht7036_Spi_Read_Check(num,SUMChecksum_Add_7053, &check3); // 读校表和
		
		a.bytes[0]=dad[0];
		a.bytes[1]=dad[1];
		a.bytes[2]=dad[2];
		a.bytes[3]=dad[3];			
	  g_meter_chip[num-1].g_u	=a.f_val;
		a.bytes[0]=dad[4];
		a.bytes[1]=dad[5];
		a.bytes[2]=dad[6];
		a.bytes[3]=dad[7];			
	  g_meter_chip[num-1].g_i	=a.f_val;		
		
		a.bytes[0]=dad[8];
		a.bytes[1]=dad[9];
		a.bytes[2]=dad[10];
		a.bytes[3]=dad[11];			
	  g_meter_chip[num-1].g_w	=a.f_val;		
					
		if((check3!=check2))		
		   return err|0x10;
		g_meter_chip[num-1].check2=check2;		
		}
   return 0;

}










//4个计量芯片硬件复位
static void Ht_7036_Rest(void)
{
    // 硬件复位
    SPI_RST = 0;
    Delay_ms(2);
    SPI_RST = 1;
    Delay_ms(5);	
}







/********** SPI读函数（无校验） **********/
static uint8_t Ht7036_Spi_Read(uint8_t num, uint8_t address, u32 *read_data)
{
    u8 high_byte = 0, mid_byte = 0, low_byte = 0;
    u16 timeout = 0;
    u8 spi_err = 0;

    // 第一步：参数合法性校验（优先检查，避免空指针）
    if (read_data == NULL || num < 1 || num > 4) {
        return 1; // 参数错误：指针空/num非法
    }

    // 第二步：初始化输出数据（避免野值）
    *read_data = 0x00;

    // 第三步：拉低片选
    switch(num) {
        case 1: SPI_CS1 = 0; break;
        case 2: SPI_CS2 = 0; break;
        case 3: SPI_CS3 = 0; break;
        case 4: SPI_CS4 = 0; break;			
    }

    // 第四步：发送读命令（地址）
    spi_err = SPI_Transceive_Byte(address, &high_byte);
    if (spi_err != SPI_OK) {
        goto ERR_EXIT; // 命令发送失败，跳转到错误处理
    }

//		Delay_us(2);
    // 第五步：读取24位有效数据
    spi_err = SPI_Transceive_Byte(0xFF, &high_byte);  // 高8位
    if (spi_err != SPI_OK) goto ERR_EXIT;

    spi_err = SPI_Transceive_Byte(0xFF, &mid_byte);   // 中8位
    if (spi_err != SPI_OK) goto ERR_EXIT;

    spi_err = SPI_Transceive_Byte(0xFF, &low_byte);   // 低8位
    if (spi_err != SPI_OK) goto ERR_EXIT;

    // 第六步：等待SPI总线空闲
    timeout = 0;
    while ((SPI0CFG & SPIBSY) != 0) {
        if (timeout++ > 100) break;
    }

    // 第七步：拼接24位数据（输出到指针）
    *read_data = ((u32)high_byte << 16) | ((u32)mid_byte << 8) | low_byte;

    // 第八步：拉高片选
    switch(num) {
        case 1: SPI_CS1 = 1; break;
        case 2: SPI_CS2 = 1; break;
        case 3: SPI_CS3 = 1; break;
        case 4: SPI_CS4 = 1; break;				
    }
		

    // 所有步骤成功，返回OK
    return SPI_OK;

// 错误处理：强制拉高片选，返回对应错误码
ERR_EXIT:
    switch(num) {
        case 1: SPI_CS1 = 1; break;
        case 2: SPI_CS2 = 1; break;
        case 3: SPI_CS3 = 1; break;
        case 4: SPI_CS4 = 1; break;	
    }
    return READ_ERR;
}

/********** SPI写函数（无校验） **********/
static uint8_t Ht7036_Spi_Write(uint8_t num, unsigned char address, unsigned long write_data)
{
    // C89规范：所有变量声明放在函数开头
    uint16_t timeout = 0;
    uint8_t dat = 0;          // 接收缓冲（SPI全双工，接收值无意义）
    uint8_t err = 0;          // SPI收发错误标志
    uint8_t high_byte = 0;    // 写入数据高8位
    uint8_t mid_byte = 0;     // 写入数据中8位
    uint8_t low_byte = 0;     // 写入数据低8位

    // 1. 参数合法性校验（优先过滤非法片选号）
    if (num < 1 || num > 4) { // 支持num=1~4（含SPI_CS4）
        return PARA_ERR;
    }

    // 2. 拆分24位写入数据为3个字节
    high_byte = (uint8_t)(write_data >> 16); // 提取bit16~23
    mid_byte  = (uint8_t)(write_data >> 8);  // 提取bit8~15
    low_byte  = (uint8_t)(write_data);       // 提取bit0~7

    // 3. 拉低对应片选（CS），启动SPI通讯
    switch(num) { // 替换冗余if-else，逻辑更清晰
        case 1: SPI_CS1 = 0; break;
        case 2: SPI_CS2 = 0; break;
        case 3: SPI_CS3 = 0; break;
        case 4: SPI_CS4 = 0; break;
    }

    // 4. 发送写命令（寄存器地址，ATT7022E写命令Bit7=1，需确认！）
    // 注：ATT7022E写命令需将Bit7置1（与读命令Bit7=0区分），若地址已含写位可省略
    address |= 0x80; // 强制置位Bit7为写命令（关键！依手册确认）
    err = SPI_Transceive_Byte(address, &dat);
    if (err != 0) {  // 收发失败直接跳转到错误处理
        goto ERR_EXIT;
    }

    // 5. 连续发送3个数据字节（全双工，接收值忽略）
    err = SPI_Transceive_Byte(high_byte, &dat);
    if (err != 0) goto ERR_EXIT;

    err = SPI_Transceive_Byte(mid_byte, &dat);
    if (err != 0) goto ERR_EXIT;

    err = SPI_Transceive_Byte(low_byte, &dat);
    if (err != 0) goto ERR_EXIT;

    // 6. 等待SPI总线彻底空闲（避免提前拉高CS导致数据截断）
    timeout = 0;
    while ((SPI0CFG & SPIBSY) != 0) {
        if (timeout++ > 100) { // 超时兜底，防止死等
            err = WRITE_ERR;
            goto ERR_EXIT;
        }
    }

    // 7. 正常退出：拉高对应片选
    switch(num) {
        case 1: SPI_CS1 = 1; break;
        case 2: SPI_CS2 = 1; break;
        case 3: SPI_CS3 = 1; break;
        case 4: SPI_CS4 = 1; break;
    }
    return OK; // 写入成功

// 错误退出：强制拉高所有涉及的片选，返回错误
ERR_EXIT:
    SPI_CS1 = 1;
    SPI_CS2 = 1;
    SPI_CS3 = 1;
    SPI_CS4 = 1;
    return WRITE_ERR;
}

// -------------------------- 业务层：带校验的SPI读 --------------------------
/**
 * @brief  带完整校验的SPI读函数（校验和+BCKREG）
 * @param  num: 片选号(1~4)
 * @param  address: 寄存器地址（不含读写位）
 * @param  read_data: 读取的24位数据输出
 * @retval 状态码（OK/READ_ERR/CHECKSUM_ERR/BCKREG_ERR等）
 */
static uint8_t Ht7036_Spi_Read_Check(uint8_t num, uint8_t address, uint32_t *read_data) {
    uint8_t ret = OK;
    uint32_t bck_data = 0;
    uint8_t read_cmd = address & 0x7F; // 读命令（Bit7=0）
//		uint8_t retry = 2;
//    uint8_t bck_read_ret = READ_ERR; // 初始化BCKREG读取状态为“失败”
//    uint32_t retry_bck = 0;	
//    uint32_t retry_data = 0;	
    // 参数校验
    if (read_data == NULL || num < 1 || num > 4) {
        return PARA_ERR;
    }
    *read_data = 0;

    // 1. 底层物理层读取
    ret = Ht7036_Spi_Read(num, address, read_data);
    if (ret != OK) {
        return READ_ERR;
    }

    // 2. 跳过校验寄存器的自校验（避免递归）
//    if (address == COM_CHECKSUM_REG || address == BCKREG_REG) {
//        return OK;
//    }

    // 3. 校验和验证
    ret = Ht7036_Comcheck(num, *read_data, read_cmd);
    if (ret != OK) {
        return CHECKSUM_ERR;
    }

//   while (retry--) {
//        // 调用重构后的BCKREG读取函数，获取“操作状态”而非数据值
//   bck_read_ret = Ht7036_Read_BCKREG(num, &bck_data);
//   if (bck_read_ret == OK) { // 仅当“读取操作成功”时，退出重试
//            break;
//        }
//    }
//    // 判断：BCKREG读取操作是否成功
//    if (bck_read_ret != OK) {
//        return READ_ERR; // 多次重试仍读取失败（硬件/通讯问题）
//    }
//    // BCKREG读取成功 → 对比数据（即使数据为0，也正常对比）
//    if (bck_data != *read_data) {
//        // 重试一次读操作
//        ret = Ht7036_Spi_Read(num, address, &retry_data);
//        if (ret != OK) {
//            return READ_ERR;
//        }
//        // 验证重试后的数据与BCKREG是否一致（再次读BCKREG）

//        ret = Ht7036_Read_BCKREG(num, &retry_bck);
//        if (ret != OK || retry_data != retry_bck) {
//            return BCKREG_ERR; // 数据不一致（非读取失败）
//        }
//        // 重试成功，更新数据
//        *read_data = retry_data;
//    }

    return OK;
}

// -------------------------- 业务层：带校验的SPI写 --------------------------
/**
 * @brief  带完整校验的SPI写函数（校验和+BCKREG）
 * @param  num: 片选号(1~4)
 * @param  address: 寄存器地址（不含读写位）
 * @param  write_data: 待写入的24位数据
 * @retval 状态码（OK/WRITE_ERR/CHECKSUM_ERR/BCKREG_ERR等）
 */
static uint8_t Ht7036_Spi_Write_Check(uint8_t num, uint8_t address, uint32_t write_data) {
    uint8_t ret = OK;
    uint32_t bck_data = 0;
    uint8_t write_cmd = address | 0x80; // 写命令（Bit7=1）
//		uint8_t retry = 2;
    uint8_t bck_read_ret = READ_ERR; // 初始化BCKREG读取状态为“失败”
    uint32_t retry_bck = 0;	
    // 参数校验
    if (num < 1 || num > 4) {
        return SPI_PARA_ERR;
    }

    // 1. 底层物理层写入
    ret = Ht7036_Spi_Write(num, address, write_data);
    if (ret != OK) {
        return WRITE_ERR;
    }

    // 2. 跳过校验寄存器的自校验
//    if (address == COM_CHECKSUM_REG || address == BCKREG_REG) {
//        return OK;
//    }

    // 3. 校验和验证
    ret = Ht7036_Comcheck(num, write_data, write_cmd);
    if (ret != OK) {
        return CHECKSUM_ERR;
    }

//		while (retry--) {
//        bck_read_ret = Ht7036_Read_BCKREG(num, &bck_data);
//        if (bck_read_ret == OK) {
//            break;
//        }
//    }
//    if (bck_read_ret != OK) {
//        return READ_ERR;
//    }
    // 对比数据（即使写入的是0，也正常判断）
//    if (bck_data != write_data) {
//        // 重试一次写操作
//        ret = Ht7036_Spi_Write(num, address, write_data);
//        if (ret != OK) {
//            return WRITE_ERR;
//        }
//        // 再次验证BCKREG

//        ret = Ht7036_Read_BCKREG(num, &retry_bck);
//        if (ret != OK || retry_bck != write_data) {
//            return BCKREG_ERR;
//        }
//    }

    return OK;
}


/********** 通讯校验和函数（调用底层函数读取0x2E） **********/
static u8 Ht7036_Comcheck(u8 num, u32 rx_data, u8 cmd) {

    u32 check_reg_data = 0;
    u8 cmd_from_reg = 0;
    u16 sum_from_reg = 0;
    u16 sum_local = 0;
    u8 high_byte = 0, mid_byte = 0, low_byte = 0;
    u8 ret = SPI_OK;
	  u8 add;

    // 1. 参数校验
    if (num < 1 || num > 4) {
        return SPI_PARA_ERR;
    }
		
		if(num<3)
			add=COMChecksum_Add_7036;
		else
			add=COMChecksum_7053;			

    // 2. 调用【底层函数】读取0x2E寄存器（关键：避免调用业务层函数）
    ret = Ht7036_Spi_Read(num, add, &check_reg_data);
    if (ret != SPI_OK) {
        return READ_ERR; // 读0x2E失败
    }

    // 3. 拆分0x2E寄存器数据
    cmd_from_reg = (check_reg_data >> 16) & 0xFF;  // 高8位=上一次命令
    sum_from_reg = check_reg_data & 0xFFFF;        // 低16位=累加和

    // 4. 拆分本地读取的24位数据
    high_byte = (rx_data >> 16) & 0xFF;
    mid_byte  = (rx_data >> 8)  & 0xFF;
    low_byte  = rx_data & 0xFF;

    // 5. 本地计算累加和
    sum_local = cmd;                // 命令字节
    sum_local += high_byte;         // 高8位
    sum_local += mid_byte;          // 中8位
    sum_local += low_byte;          // 低8位

    // 6. 校验命令和累加和
    if (cmd_from_reg != cmd) {
        return CHECKSUM_ERR;
    }
    if (sum_from_reg != sum_local) {
        return CHECKSUM_ERR;
    }

    return OK;
}

/**
 * @brief  读取BCKREG寄存器（从机回执）
 * @param  num: 片选号(1~4)
 * @param  bck_data: 输出24位BCKREG数据（即使数据为0，也正常输出）
 * @retval 状态码（OK=读取成功；SPI_TIMEOUT/READ_ERR=读取失败）
 */
static uint8_t Ht7036_Read_BCKREG(uint8_t num, uint32_t *bck_data) {
    uint8_t ret = OK;
    uint8_t add;
    // 参数校验
    if (bck_data == NULL || num < 1 || num > 4) {
        return SPI_PARA_ERR;
    }
    *bck_data = 0; // 初始化输出
		if(num<3)
			add=r_BckReg_36;
		else
			add=BackupData_53;	
    // 调用底层函数读取BCKREG（0x2F）
    ret = Ht7036_Spi_Read(num, add, bck_data);
    if (ret != OK) {
        return READ_ERR; // 明确标记“读取失败”
    }
    return OK; // 读取成功（无论数据是否为0）
}










/**
 * @description: ht7036 分相零漂
 * @param {u8} *adj_zero_data
 * @return {*}
 * @Author: zf
 * @Date: 2024-01-26 11:32:00
 */
 void Ht7036_Phase_Zero(uint8_t num, uint8_t phase)
{
    uint32_t att;
    uint8_t address1, address2;

    if (phase == 1)
    {
        address1 = r_IaRms;
        address2 = w_IaRmsoffse;
    }
    if (phase == 2)
    {
        address1 = r_IbRms;
        address2 = w_IbRmsoffse;
    }
    if (phase == 3)
    {
        address1 = r_UaRms;
        address2 = w_UaRmsoffse;
    }
		

    if (phase == 4)
    {
        address1 = r_IcRms;
        address2 = w_IcRmsoffse;
    }
    if (phase == 5)
    {
        address1 = r_UcRms;
        address2 = w_UcRmsoffse;

    }
    if (phase == 6)
    {
        address1 = r_UbRms;
        address2 = w_UbRmsoffse;

    }
        Ht7036_Spi_Read_Check(num,address1,&att);
        att=att*att;     
        att = att>>15;
		 if((phase!=3)&&(phase!=6))
     att=att/6;		
        Ht7036_Spi_Write_Check(num,address2, att);

		
}

/**
 * @description: ht7036 零漂校准
 * @param {u8} *adj_zero_data
 * @return {*}
 * @Author: zf
 * @Date: 2024-01-26 11:32:00
 */
void Ht7036_Adj_Zero(void)
{	
	uint8_t num=1;
	uint8_t j;
//		Ht_7036_Rest();	
//    Ht7036_Spi_Write(0xC3, 0x000000); // 清教表数据

//        Ht7036_Spi_Write(0xC9, 0x00005A);      // 打开校准数据写
//        Ht7036_Spi_Write(w_ModeCfg, 0xB87F);   // 填写模式配置寄存器0xB97E       1.8Mhz 更新速率284ms。 开启uabc,iabc.
//        Ht7036_Spi_Write(w_EMCfg, 0x0000);     // 算法控制器 三相四线夹角采用算法2
//        Ht7036_Spi_Write(w_EMUCfg, 0x0c04);    // 填写EMU单元配置寄存器  F804 关闭基波谐波测量 Fc04 开启基波谐波测量。
//        Ht7036_Spi_Write(w_EMUIE, 0x20c3);     // 开启数据更新中断,sag，过流中断
//        Ht7036_Spi_Write(w_ModuleCFG, 0x3427); // 填写模拟模块使能寄存器
//        Ht7036_Spi_Write(w_PGACtrl, 0x0000);   // 各路ADC增益均为1
//        Ht7036_Spi_Write(w_Hfconst, Meter_HFConst);
//        Ht7036_Spi_Write(w_TCcoffA, 0xFF00); //
//        Ht7036_Spi_Write(w_TCcoffB, 0x0DB8); //
//        Ht7036_Spi_Write(w_TCcoffC, 0xD1DA); //
	
	if(g_cap_num.cap_num>2)	
			j=3;
	else
		j=2;
    for(num=1;num<j;num++)
	  {
			Ht7036_Spi_Write_Check(num,0xC9, 0x00005A); // 打开校准数据写


			Ht7036_Spi_Write_Check(num,w_UaRmsoffse, 0);		
			Ht7036_Spi_Write_Check(num,w_UbRmsoffse, 0);		
			Ht7036_Spi_Write_Check(num,w_UcRmsoffse, 0);	
			Ht7036_Spi_Write_Check(num,w_IaRmsoffse, 0);	
			Ht7036_Spi_Write_Check(num,w_IbRmsoffse, 0);
			Ht7036_Spi_Write_Check(num,w_IcRmsoffse, 0);
	    Ht7036_Spi_Write_Check(num,0xC9, 0x000001); // 关闭校准数据写
			
		}
		Delay_ms(100);
		Ht7036_Spi_Write_Check(1,0xC9, 0x00005A); // 打开校准数据写
		Ht7036_Spi_Write_Check(2,0xC9, 0x00005A); // 打开校准数据写		
    Ht7036_Phase_Zero(1, 1);
    Ht7036_Phase_Zero(1, 2);		
    Ht7036_Phase_Zero(1, 3);	
    Ht7036_Phase_Zero(1, 4);
    Ht7036_Phase_Zero(1, 5);		
    Ht7036_Phase_Zero(1, 6);	
		
    Ht7036_Phase_Zero(2, 1);		
    Ht7036_Phase_Zero(2, 2);
		Ht7036_Phase_Zero(2, 3);
		
    Ht7036_Phase_Zero(2, 4);
    Ht7036_Phase_Zero(2, 5);		
    Ht7036_Phase_Zero(2, 6);			
		
	    Ht7036_Spi_Write_Check(1,0xC9, 0x000001); // 关闭校准数据写
	    Ht7036_Spi_Write_Check(2,0xC9, 0x000001); // 关闭校准数据写
}
char Ht7036_Zero_Check(void)
{
    uint32_t att;

    Ht7036_Spi_Read_Check(1,r_IaRms,&att);

    if (att > 0X500)
        return 1;
     Ht7036_Spi_Read_Check(1,r_IbRms,&att);

    if (att > 0X500)

        return 2;
    Ht7036_Spi_Read_Check(1,r_UaRms,&att);

    if (att > 0X500)

        return 3;

    Ht7036_Spi_Read_Check(1,r_IcRms,&att);

    if (att > 0X500)
        return 4;
     Ht7036_Spi_Read_Check(1,r_UcRms,&att);

    if (att > 0X500)
        return 5;
     Ht7036_Spi_Read_Check(1,r_UbRms,&att);
		
    if (att > 0X500)
        return 6;
		if(g_cap_num.cap_num>2)
		{
    Ht7036_Spi_Read_Check(2,r_IaRms,&att);

    if (att > 0X500)
        return 7;
     Ht7036_Spi_Read_Check(2,r_IbRms,&att);

    if (att > 0X500)

        return 8;
    Ht7036_Spi_Read_Check(2,r_UaRms,&att);

    if (att > 0X500)

        return 9;

    Ht7036_Spi_Read_Check(2,r_IcRms,&att);

    if (att > 0X500)
        return 10;
    Ht7036_Spi_Read_Check(2,r_UcRms,&att);

    if (att > 0X500)
        return 11;
    att = Ht7036_Spi_Read_Check(2,r_UbRms,&att);
		
    if (att > 0X500)
        return 12;
	}			
				
    Ht7036_Spi_Read_Check(4,Rms_U,&att);

    if (att > 0X500)
        return 13;
    Ht7036_Spi_Read_Check(4,Rms_I1,&att);
		
    if (att > 0X500)
        return 14;				
		return 0;
}

/**
 * @description: ht7036 分相增益
 * @param {u8} *adj_zero_data
 * @return {*}
 * @Author: zf
 * @Date: 2024-01-26 11:32:00
 */
static void Ht7036_Phase_Gain(uint8_t num, uint8_t phase )
{
    uint32_t att;
    float a;
    uint8_t address1, address2;
    if (phase == 1)
    {
        address1 = r_IaRms;
        address2 = w_IgainA;
    }
    if (phase == 2)
    {
        address1 = r_IbRms;
        address2 = w_IgainB;
    }
    if (phase == 3)
    {
        address1 = r_UaRms;
        address2 = w_UgainA;
    }
		

    if (phase == 4)
    {
        address1 = r_IcRms;
        address2 = w_IgainC;
    }
    if (phase == 5)
    {
        address1 = r_UcRms;
        address2 = w_UgainC;

    }
    if (phase == 6)
    {
        address1 = r_UbRms;
        address2 = w_UgainB;

    }
     Ht7036_Spi_Read_Check(num,address1,&att);
		
    a = att / 8192.0;
		
		
		 if((phase!=3)&&(phase!=6))
		 {
     a=a/6;							
     a = (ADJUST_CURRENT / a) - 1.0;
		 }
		 else
    a = (ADJUST_VOLTAGE / a) - 1.0;	
		 
    if (a >= 0)
        att = a * 32768;
    else
        att = (uint32_t)(65536.0 + a * 32768.0);

    Ht7036_Spi_Write_Check(num,address2, att);




}
void Ht7036_Adj_Gain(void)
{
  uint8_t num,j;
	
		if(g_cap_num.cap_num>2)	
			j=3;
	else
		j=2;
    for(num=1;num<j;num++)
	  {
			Ht7036_Spi_Write_Check(num,0xC9, 0x00005A); // 打开校准数据写

			Ht7036_Spi_Write_Check(num,w_UgainA, 0);
   			
		
			Ht7036_Spi_Write_Check(num,w_UgainB, 0);
   			
		
			Ht7036_Spi_Write_Check(num,w_UgainC, 0);
   			
	
			Ht7036_Spi_Write_Check(num,w_IgainA, 0);
   			
		
			Ht7036_Spi_Write_Check(num,w_IgainB, 0);
   			
		
			Ht7036_Spi_Write_Check(num,w_IgainC, 0);
	    Ht7036_Spi_Write_Check(num,0xC9, 0x000001); // 关闭校准数据写
				
		}
		Delay_ms(100);
		
		Ht7036_Spi_Write_Check(1,0xC9, 0x00005A); // 打开校准数据写
		Ht7036_Spi_Write_Check(2,0xC9, 0x00005A); // 打开校准数据写		
    Ht7036_Phase_Gain(1, 1);
    Ht7036_Phase_Gain(1, 2);		
    Ht7036_Phase_Gain(1, 3);	
    Ht7036_Phase_Gain(1, 4);
    Ht7036_Phase_Gain(1, 5);		
    Ht7036_Phase_Gain(1, 6);	
		
    Ht7036_Phase_Gain(2, 1);		
    Ht7036_Phase_Gain(2, 2);
		Ht7036_Phase_Gain(2, 3);
    Ht7036_Phase_Gain(2, 4);
    Ht7036_Phase_Gain(2, 5);		
    Ht7036_Phase_Gain(2, 6);		

		Ht7036_Spi_Write_Check(1,0xC9, 0x000001); // 关闭校准数据写
		Ht7036_Spi_Write_Check(2,0xC9, 0x000001); // 关闭校准数据写			
		
}

char Ht7036_Gain_Check(void)
{
    uint32_t att;
    float a;

     Ht7036_Spi_Read_Check(1,r_IaRms,&att);
    a =  (float)att/ 8192.0/6 ;
    if ((((a - ADJUST_CURRENT) / ADJUST_CURRENT) > 0.005) || (((a - ADJUST_CURRENT) / ADJUST_CURRENT) < (-0.005)))
        return 1;
		
		
		
    Ht7036_Spi_Read_Check(1,r_IbRms,&att);
   a =  (float)att / 8192.0/6 ;
    if ((((a - ADJUST_CURRENT) / ADJUST_CURRENT) > 0.005) || (((a - ADJUST_CURRENT) / ADJUST_CURRENT) < (-0.005)))
        return 2;
		
		
    Ht7036_Spi_Read_Check(1,r_UaRms,&att);
   a =  (float)att / 8192.0 ;
    if ((((a - ADJUST_VOLTAGE) / ADJUST_VOLTAGE) > 0.005) || (((a - ADJUST_VOLTAGE) / ADJUST_VOLTAGE) < (-0.005)))
        return 3;	

		
    Ht7036_Spi_Read_Check(1,r_IcRms,&att);
    a =  (float)att / 8192.0/6;
    if ((((a - ADJUST_CURRENT) / ADJUST_CURRENT) > 0.005) || (((a - ADJUST_CURRENT) / ADJUST_CURRENT) < (-0.005)))
        return 4;
		
		
    Ht7036_Spi_Read_Check(1,r_UcRms,&att);
    a = (float)att / 8192.0/6 ;
    if ((((a - ADJUST_CURRENT) / ADJUST_CURRENT) > 0.005) || (((a - ADJUST_CURRENT) / ADJUST_CURRENT) < (-0.005)))
        return 5;
		
     Ht7036_Spi_Read_Check(1,r_UbRms,&att);
    a = (float)att / 8192.0;
    if ((((a - ADJUST_VOLTAGE) / ADJUST_VOLTAGE) > 0.005) || (((a - ADJUST_VOLTAGE) / ADJUST_VOLTAGE) < (-0.005)))    
        return 6;
		
		
     Ht7036_Spi_Read_Check(2,r_IaRms,&att);
    a =  (float)att/ 8192.0/6 ;
    if ((((a - ADJUST_CURRENT) / ADJUST_CURRENT) > 0.005) || (((a - ADJUST_CURRENT) / ADJUST_CURRENT) < (-0.005)))
        return 7;
		
   Ht7036_Spi_Read_Check(2,r_IbRms,&att);
   a =  (float)att / 8192.0/6 ;
    if ((((a - ADJUST_CURRENT) / ADJUST_CURRENT) > 0.005) || (((a - ADJUST_CURRENT) / ADJUST_CURRENT) < (-0.005)))
        return 8;
		
     Ht7036_Spi_Read_Check(1,r_UaRms,&att);
   a =  (float)att / 8192.0 ;
    if ((((a - ADJUST_VOLTAGE) / ADJUST_VOLTAGE) > 0.005) || (((a - ADJUST_VOLTAGE) / ADJUST_VOLTAGE) < (-0.005)))
        return 9;

				
     Ht7036_Spi_Read_Check(1,r_IcRms,&att);
    a =  (float)att / 8192.0/6;
    if ((((a - ADJUST_CURRENT) / ADJUST_CURRENT) > 0.005) || (((a - ADJUST_CURRENT) / ADJUST_CURRENT) < (-0.005)))
        return 10;
		
    Ht7036_Spi_Read_Check(1,r_UcRms,&att);
    a = (float)att / 8192.0 / 6;

    if ((((a - ADJUST_CURRENT) / ADJUST_CURRENT) > 0.005) || (((a - ADJUST_CURRENT) / ADJUST_CURRENT) < (-0.005)))
        return 11;
		
    Ht7036_Spi_Read_Check(1,r_UbRms,&att);
    a = (float)att / 8192.0;
    if ((((a - ADJUST_VOLTAGE) / ADJUST_VOLTAGE) > 0.005) || (((a - ADJUST_VOLTAGE) / ADJUST_VOLTAGE) < (-0.005)))    
        return 12;

		
	return 0;
}

///**
// * @description: ht7036 分相功率校正
// * @param {u8} *adj_zero_data
// * @return {*}
// * @Author: zf
// * @Date: 2024-01-26 11:32:00
// */
//static void Ht7036_Phase_Power(Phase phase)
//{
//    uint32_t att;
//    float a;
//    uint8_t address1, address2, address3;
//    if (phase == Phase_A)
//    {
//        address1 = r_Qa;
//        address2 = w_PgainA;
//        address3 = w_QgainA;
//    }
//    if (phase == Phase_B)
//    {
//        address1 = r_Pb;
//        address2 = w_PgainB;
//        address3 = w_QgainB;
//    }
//    if (phase == Phase_C)
//    {
//        address1 = r_Pc;
//        address2 = w_PgainC;
//        address3 = w_QgainC;
//    }
//    att = Ht7036_Read_Count_Data(address1);
////		printf("p1=%lu",att);

//    if (att > 0x800000)
//    {
//        a = 0x1000000 - att;
//        a = a * Meter_K;
//    }
//    else
//    {
//        a = att;
//        a = a * Meter_K;
//    }
//    a = (a - ADJUST_POWER) / ADJUST_POWER;
//    a = -a / (1 + a);
//    if (a >= 0)
//        att = a * 32768;
//    else
//        att = 65536 + a * 32768;
////		printf("p=%lu",att);
//    Ht7036_Spi_Write(address2, att);
//    Ht7036_Spi_Write(address3, att);
//}
//void Ht7036_Adj_Power(void)
//{

//    Ht7036_Spi_Write(0xC9, 0x00005A); // 打开校准数据写
//    Ht7036_Spi_Write(w_PgainA, 0);
////    Ht7036_Spi_Write(w_PgainB, 0);
////    Ht7036_Spi_Write(w_PgainC, 0);
//    Ht7036_Spi_Write(w_QgainA, 0);
////    Ht7036_Spi_Write(w_QgainB, 0);
////    Ht7036_Spi_Write(w_QgainC, 0);
//				Delay_ms(800);
//    Ht7036_Phase_Power(Phase_A);
//				Delay_ms(800);
////    Ht7036_Phase_Power(Phase_B);
////    Ht7036_Phase_Power(Phase_C);
//    Ht7036_Spi_Write(0xC9, 0x000001); // 关闭校准数据写
//	
//				
//	Ht7036_Spi_Write(0xC6, 0x00005A);			
//	
//    Ht7036_Spi_Write(0xC6, 0x000000);	

//}

//char Ht7036_Power_Check(void)
//{
//    uint32_t att;
//    float a;
//    att = Ht7036_Read_Count_Data(r_Qa);

//    if (att > 0x800000)
//    {
//        a = 0x1000000 - att;
//        a = a * Meter_K;
//    }
//    else
//    {
//        a = att;
//        a = a * Meter_K;
//    }

//    if ((((a - ADJUST_POWER) / ADJUST_POWER) > 0.005) || (((a - ADJUST_POWER) / ADJUST_POWER) < (-0.005)))
//        return 1;
////    att = Ht7036_Read_Count_Data(r_Pb);

////    if (att > 0x800000)
////    {
////        a = 0x1000000 - att;
////        a = a * Meter_K;
////    }
////    else
////    {
////        a = att;
////        a = a * Meter_K;
////    }

////    if ((((a - ADJUST_POWER) / ADJUST_POWER) > 0.005) || (((a - ADJUST_POWER) / ADJUST_POWER) < (-0.005)))
////        return 2;
////    att = Ht7036_Read_Count_Data(r_Pc);

////    if (att > 0x800000)
////    {
////        a = 0x1000000 - att;
////        a = a * Meter_K;
////    }
////    else
////    {
////        a = att;
////        a = a * Meter_K;
////    }

////    if ((((a - ADJUST_POWER) / ADJUST_POWER) > 0.005) || (((a - ADJUST_POWER) / ADJUST_POWER) < (-0.005)))
////        return 3;

//		else
//        return 0;
//    
//}

//char Ht7036_Pf_Check(void)
//{
//    uint32_t att;
//    float a;
//    att = Ht7036_Read_Count_Data(r_Pfa);

//    if (att > 0x800000)
//    {
//       a = ((float)att - 0x1000000);
//        a = (a / 8388608.0);
//    }
//    else
//    {
//        a = (att / 8388608.0);
//			
//    }
//		
//		
//		
////    if ((((a - 1.0) / 1.0) > 0.005) || (((a - 1.0) /1.0) < (-0.005)))
////        return 1;

////		else
//        return 0;
//    
//}


///**
// * @description: ht7036 分相相位校正
// * @param {u8} *adj_zero_data
// * @return {*}
// * @Author: zf
// * @Date: 2024-01-26 11:32:00
// */
//static void Ht7036_Phase_Phase(Phase phase)
//{
//    uint32_t att;
//    float a;
//    uint8_t address1, address2, address3;
//    if (phase == Phase_A)
//    {
//        address1 = r_Qa;
//        address2 = w_PhSregApq0;
//        address3 = w_PhSregApq1;
//    }
//    if (phase == Phase_B)
//    {
//        address1 = r_Pb;
//        address2 = w_PhSregBpq0;
//        address3 = w_PhSregBpq1;
//    }
//    if (phase == Phase_C)
//    {
//        address1 = r_Pc;
//        address2 = w_PhSregCpq0;
//        address3 = w_PhSregCpq1;
//    }
//    att = Ht7036_Read_Count_Data(address1);
////		printf("q1=%lu",att);
//    if (att > 0x800000)
//    {
//        a = 0x1000000 - att;
//        a = a * Meter_K;
//    }
//    else
//    {
//        a = att;
//        a = a * Meter_K;
//    }
//    a = (a - ADJUST_PHASE) / ADJUST_PHASE;
//    a = -a / 1.732;
//    if (a >= 0)
//        att = a * 32768;
//    else
//        att = 65536 + a * 32768;
////				printf("q=%lu",att);
//    Ht7036_Spi_Write(address2, att);
//    Ht7036_Spi_Write(address3, att);
//}
//void Ht7036_Adj_Phase(void)
//{	

//    Ht7036_Spi_Write(0xC9, 0x00005A); // 打开校准数据写
//    Ht7036_Spi_Write(w_PhSregApq0, 0);
//    Ht7036_Spi_Write(w_PhSregApq1, 0);
////    Ht7036_Spi_Write(w_PhSregBpq0, 0);
////    Ht7036_Spi_Write(w_PhSregBpq1, 0);
////    Ht7036_Spi_Write(w_PhSregCpq0, 0);
////    Ht7036_Spi_Write(w_PhSregCpq1, 0);
//				  Delay_ms(800);
//    Ht7036_Phase_Phase(Phase_A);
//					  Delay_ms(800);
////    Ht7036_Phase_Phase(Phase_B);
////    Ht7036_Phase_Phase(Phase_C);
//    Ht7036_Spi_Write(0xC9, 0x000001); // 关闭校准数据写
//		Ht7036_Spi_Write(0xC6, 0x00005A);			

//    Ht7036_Spi_Write(0xC6, 0x000000);	
//}

//char Ht7036_Phase_Check(void)
//{
//    uint32_t att;
//    float a;
//    att = Ht7036_Read_Count_Data(r_Qa);

////	rs_data(att);
//    if (att > 0x800000)
//    {
//        a = 0x1000000 - att;
//        a = a * Meter_K;
//    }
//    else
//    {
//        a = att;
//        a = a * Meter_K;
//    }

//    if ((((a - ADJUST_PHASE) / (ADJUST_PHASE)) > 0.005) || (((a - ADJUST_PHASE) / (ADJUST_PHASE)) < (-0.005)))
//        return 1;
////    att = Ht7036_Read_Count_Data(r_Pb);
////    if (att > 0x800000)
////    {
////        a = 0x1000000 - att;
////        a = a * Meter_K;
////    }
////    else
////    {
////        a = att;
////        a = a * Meter_K;
////    }

////    if ((((a - ADJUST_PHASE) / (ADJUST_PHASE)) > 0.005) || (((a - ADJUST_PHASE) / (ADJUST_PHASE)) < (-0.005)))
////        return 2;
////    att = Ht7036_Read_Count_Data(r_Pc);
////    if (att > 0x800000)
////    {
////        a = 0x1000000 - att;
////        a = a * Meter_K;
////    }
////    else
////    {
////        a = att;
////        a = a * Meter_K;
////    }

////    if ((((a - ADJUST_PHASE) / (ADJUST_PHASE)) > 0.005) || (((a - ADJUST_PHASE) / (ADJUST_PHASE)) < (-0.005)))
////        return 3;
//    else
//    {

//        return 0;
//    }
//}

///**
// * @description:
// * @return {*}
// * @Author: zf
// * @Date: 2024-01-26 11:32:42
// */
//static void Ht7036_Put_Data(Phase a, Phase_DataTypeDef *Phase_Datatypedef)
//{
//    uint32_t att;
//    uint8_t u, i, p, q, pf, freq, uthd, ithd, u_pg, p_pg, ep, eq;
//    freq = r_Freq;
//    if (a == Phase_A)
//    {
//        u = r_UaRms;
//        i = r_IaRms;
//        u_pg = r_YUaUb;
//        pf = r_Pfa;
//        p_pg = r_Pga;
//        p = r_Pa;
//        q = r_Qa;
//        uthd = r_LineUaRrms;
//        ithd = r_LineIaRrms;
//        ep = r_Epa;
//        eq = r_Eqa;
//    }
//    if (a == Phase_B)
//    {
//        u = r_UbRms;
//        i = r_IbRms;
//        u_pg = r_YUbUc;
//        pf = r_Pfb;
//        p_pg = r_Pgb;
//        p = r_Pb;
//        q = r_Qb;
//        uthd = r_LineUbRrms;
//        ithd = r_LineIbRrms;
//        ep = r_Epb;
//        eq = r_Eqb;
//    }
//    if (a == Phase_C)
//    {
//        u = r_UcRms;
//        i = r_IcRms;
//        u_pg = r_YUaUc;
//        pf = r_Pfc;
//        p_pg = r_Pgc;
//        p = r_Pc;
//        q = r_Qc;
//        uthd = r_LineUcRrms;
//        ithd = r_LineIcRrms;
//        ep = r_Epc;
//        eq = r_Eqc;
//    }
//    if (a == Phase_T)
//    {
//        u = r_UtRms;
//        i = r_ItRms;
//        pf = r_Pft;
//        p_pg = r_Pga;
//        p = r_Pt;
//        q = r_Qt;
//        ep = r_Ept;
//        eq = r_Eqt;
//    }
//    Phase_Datatypedef->Rurms = Ht7036_Read_Count_Data(u);;
//    Phase_Datatypedef->Rirms = Ht7036_Read_Count_Data(i);				
//    Phase_Datatypedef->Rp = Ht7036_Read_Count_Data(q);
//    Phase_Datatypedef->Rq =Ht7036_Read_Count_Data(p);
//    Phase_Datatypedef->Freq = Ht7036_Read_Count_Data(freq);
//}
//void Ht7036_Read(void)
//{
//    uint32_t att;
//		static uint8_t i;


//	
//	

//    att = Ht7036_Spi_Read(0x2c); // 读状态寄存器

//    if ((Ht7036_Comcheck(att))) 
//    {
//			if (att & 0x0080) //上电复位或校表参数复位
//        {
////						printf("上电复位");
//						EA=0;
//						Ht7036_Check();
//						Ht7036_Config(Meter_type_3L4);
//						g_ht7036_adjust=1;
//						EA=1;
//        }			
//    }

//	
//    att = Ht7036_Spi_Read(r_INTFlag); // 读中断寄存器
//    if ((Ht7036_Comcheck(att)))
//    {
//				i=0;
//        if (att & 0x0001) //未校表
//        {
////						g_ht7036_adjust=1;
////						Ht7036_Config(Meter_type_3L4);
////										printf("未交表");
//					
//        }
//				else
//				{
//	
////				    adj = Ht7036_Spi_Read(0x3E); // 读状态寄存器
////						if (adj!=adj_data1)
////						{

////										Ht7036_Config(Meter_type_3L4);		
////						}		
////				    adj = Ht7036_Spi_Read(0x5E); // 读状态寄存器
////						if (adj!=adj_data2)
////						{
////										Ht7036_Config(Meter_type_3L4);		
////						}								
////				
//				
//				}
//        if (att & 0x0004)
//        {
//								/*过零中断*/
//        }				
//        if (att & 0x0002)
//        {
//            Ht7036_Put_Data(Phase_A, &ADataTypeDef);
//						if((ADataTypeDef.Rp==0)&&(ADataTypeDef.Rq==0))
//						ADataTypeDef.Rpf=0;
//						else
//						{
//							if(ADataTypeDef.Rq<0)
//							
//							ADataTypeDef.Rpf=-(fabs(ADataTypeDef.Rp))/sqrt(ADataTypeDef.Rp*ADataTypeDef.Rp+ADataTypeDef.Rq*ADataTypeDef.Rq);
//							else	
//							ADataTypeDef.Rpf=(fabs(ADataTypeDef.Rp))/sqrt(ADataTypeDef.Rp*ADataTypeDef.Rp+ADataTypeDef.Rq*ADataTypeDef.Rq);							
//						}	
//						if(ADataTypeDef.Rurms>20)
//						g_Hdun[12]	=		ADataTypeDef.Uthd*1000;	
//						else
//						{
//							ADataTypeDef.Uthd=0;
//						g_Hdun[12]	=		0;	
//						}
//						if(ADataTypeDef.Rirms>0.1)						
//						g_Hdin[12]  =   ADataTypeDef.Ithd*1000;	
//						else
//						{
//							ADataTypeDef.Ithd=0;
//						g_Hdin[12]  =   0;	
//						}							
//						g_ht7036_adjust=0;
//						
////					att = Ht7036_Spi_Read(0x3E); // 读状态寄存器
////					printf("1:%lu \r\n",att);
////										    att = Ht7036_Spi_Read(0x5E); // 读状态寄存器
////					printf("2:%lu \r\n",att);					
//				
//        }										

//    }
//		else
//		{
//				i++;
//				if(i>5)
//				{
////					printf("检测不到");
////					printf("%x",Ht7036_Spi_Read(r_DeviceID));
//					g_ht7036_adjust=1;
//						EA=0;
//						Ht7036_Check();
//						Ht7036_Config(Meter_type_3L4);
//						EA=1;
//						i=0;
//				}
//		}
//}

unsigned char Ht7036_Adjust_Data_Save(void)
{
    uint32_t att;
    uint8_t dat[40];
    uint8_t num,j,i;
		uint8_t err=0;
		FloatToBytesUnion a;
		if(g_cap_num.cap_num>2)
			j=3;
		else
			j=2;
  err|=FM31256_Write_Protect(0);//关闭写保护	
    for(num=1;num<j;num++)
		{
	  i=0;
    err|=Ht7036_Spi_Write(num,0xC6, 0x00005A);
	

    err|=Ht7036_Spi_Read_Check(num,w_UaRmsoffse,&att);

    dat[i++] = att >> 8;
    dat[i++] = att;
    err|=Ht7036_Spi_Read_Check(num,w_UbRmsoffse,&att);

    dat[i++] = att >> 8;
    dat[i++] = att;
    err|=Ht7036_Spi_Read_Check(num,w_UcRmsoffse,&att);

    dat[i++] = att >> 8;
    dat[i++] = att;
		
     err|=Ht7036_Spi_Read_Check(num,w_IaRmsoffse,&att);

    dat[i++] = att >> 8;
    dat[i++] = att;
     err|=Ht7036_Spi_Read_Check(num,w_IbRmsoffse,&att);

    dat[i++] = att >> 8;
    dat[i++] = att;
     err|=Ht7036_Spi_Read_Check(num,w_IcRmsoffse,&att);

    dat[i++] = att >> 8;
    dat[i++] = att;


    err|=Ht7036_Spi_Read_Check(num,w_UgainA,&att);

    dat[i++] = att >> 8;
    dat[i++] = att;
    err|=Ht7036_Spi_Read_Check(num,w_UgainB,&att);

    dat[i++] = att >> 8;
    dat[i++] = att;
    err|=Ht7036_Spi_Read_Check(num,w_UgainC,&att);

    dat[i++] = att >> 8;
    dat[i++] = att;


    err|= Ht7036_Spi_Read_Check(num,w_IgainA,&att);
    dat[i++] = att >> 8;
    dat[i++] = att;
    err|= Ht7036_Spi_Read_Check(num,w_IgainB,&att);
    dat[i++] = att >> 8;
    dat[i++] = att;
    err|= Ht7036_Spi_Read_Check(num,w_IgainC,&att);
    dat[i++] = att >> 8;
    dat[i++] = att;		

		err|= Ht7036_Spi_Read_Check(num,0x3e,&att);
		dat[i++]= att >> 16;
    dat[i++]= att>> 8;
    dat[i++]= att;

    err|= Ht7036_Spi_Read_Check(num,0x5e,&att);
		dat[i++]= att >> 16;
    dat[i++]= att>> 8;
    dat[i++]= att;


    dat[i++] = CRC8_Calc(dat, 30) >> 8;
    dat[i++]= CRC8_Calc(dat, 30);

    err|=Ht7036_Spi_Write_Check(num,0xC6, 0x000001);



    if(FM31256_Write_Calib(num,dat,31))
			err|=1;
}
	i=0;
  


    
		err |= Ht7036_Spi_Read_Check(4, Rms_U, &att);   // 电压有效值
    a.f_val=100.0/att;
   dat[i++]=a.bytes[0];
   dat[i++]=a.bytes[1];
   dat[i++]=a.bytes[2];
   dat[i++]=a.bytes[3];

		err |= Ht7036_Spi_Read_Check(4, Rms_I1, &att);  // 电流有效值
    a.f_val=5.0/att;
   dat[i++]=a.bytes[0];
   dat[i++]=a.bytes[1];
   dat[i++]=a.bytes[2];
   dat[i++]=a.bytes[3];



		err |= Ht7036_Spi_Read_Check(4, PowerP1, &att);  // 有功功率
		
    a.f_val=500.0/att;
   dat[i++]=a.bytes[0];
   dat[i++]=a.bytes[1];
   dat[i++]=a.bytes[2];
   dat[i++]=a.bytes[3];
	 
	err |= Ht7036_Spi_Read_Check(4, SUMChecksum_Add_7053, &att);  // 电流有效值
		dat[i++]= att >> 16;
    dat[i++]= att>> 8;
    dat[i++]= att;	
    if(FM31256_Write_Calib(4,dat,16))
			err|=1;
  dat[0]=system_date.year;
  dat[1]=system_date.month;
  dat[2]=system_date.day;		
	 err|=FM31256_FRAM_Write(FM31256_CALIB_VISON, dat  ,3);	
		
	  err|=FM31256_Write_Protect(1);//关闭写保护		
 return err;

}

//void Ht7036_Energy_Clear(void)
//{
//    Ht7036_Spi_Write(0xC9, 0x00005A);   // 打开校准数据写
//    Ht7036_Spi_Write(w_EMUCfg, 0xFc84); // 填写EMU单元配置寄存器  F804 关闭基波谐波测量 Fc04 开启基波谐波测量。
//    Ht7036_Spi_Write(0xC9, 0x000001);   // 关闭校准数据写
//    Ht7036_Read_Count_Data(r_Ept);
//    Ht7036_Read_Count_Data(r_Eqt);
//    Ht7036_Read_Count_Data(r_Ept);
//    Ht7036_Read_Count_Data(r_Eqt);
//    Ht7036_Spi_Write(0xC9, 0x00005A);   // 打开校准数据写
//    Ht7036_Spi_Write(w_EMUCfg, 0xFc04); // 填写EMU单元配置寄存器  F804 关闭基波谐波测量 Fc04 开启基波谐波测量。
//    Ht7036_Spi_Write(0xC9, 0x000001);   // 关闭校准数据写
//    ep1 = 0;
//    ep2 = 0;
//    eq1 = 0;
//    eq2 = 0;
//}
//void Ht7036_Type_Change(u8 type)
//{
//    if (type == Meter_type_3L3)
//    {
//        Ht7036_Spi_Write(0xC9, 0x00005A);    // 打开校准数据写
//        Ht7036_Spi_Write(w_EMUCfg, 0xFc04);  // 填写EMU单元配置寄存器  F804 关闭基波谐波测量 Fc04 开启基波谐波测量。
//        Ht7036_Spi_Write(w_ModeCfg, 0xF97E); // 填写模式配置寄存器0xB97E   ub来自ub通道， 0xF97E来自内部ua-uc
//        Ht7036_Spi_Write(w_EMCfg, 0x0001);   // 算法控制器 三相三线
//    }
//    if (type == Meter_type_3L4)
//    {
//        Ht7036_Spi_Write(0xC9, 0x00005A);    // 打开校准数据写
//        Ht7036_Spi_Write(w_EMUCfg, 0xFc04);  // 填写EMU单元配置寄存器  F804 关闭基波谐波测量 Fc04 开启基波谐波测量。
//        Ht7036_Spi_Write(w_ModeCfg, 0xB97E); // 填写模式配置寄存器0xB97E   ub来自ub通道， 0xF97E来自内部ua-uc
//        Ht7036_Spi_Write(w_EMCfg, 0x0000);   // 算法控制器 三相四线
//    }
//}



