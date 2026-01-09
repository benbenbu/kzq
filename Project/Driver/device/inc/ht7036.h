
#ifndef _HT7036_H_
#define _HT7036_H_

#include "config.h"
/* 引脚 */
sbit SPI_RST =P3^0;
sbit SPI_CS1 =P3^1;
sbit SPI_CS2 =P3^2;
sbit SPI_CS3 =P3^3;//7053
sbit SPI_CS4 =P3^4;//7053
sbit SPI_IRQ1 =P4^0;
sbit SPI_IRQ2 =P4^1;
sbit SPI_IRQ3 =P4^2;
sbit SPI_IRQ4 =P4^3;



/* 宏定义 */
#define CHIP_ID_36 0x7122E0UL
#define CHIP_ID_53 0x7053B0UL

#define ADJUST_CURRENT 5
#define ADJUST_VOLTAGE 100
#define ADJUST_POWER 288
#define ADJUST_PHASE 144





/****************************ATT7053计量数据寄存器***********************************/
#define ATT7053_CHIP_ID 0x7053B0
#define WRITE_OPEN_1 0xBC // 写保护打开 写40H-45H
#define WRITE_OPEN_2 0xA6 // 写保护打开 写50H-7CH
#define WRITE_CLOSE 0x00  // 写保护关闭
#define Rms_I1 0x06
#define Rms_I2 0x07
#define Rms_U 0x08
#define BackupData_53 0x16           // 通讯数据备份寄存器
#define COMChecksum_7053 0x17     // 通讯校验和寄存器
#define SUMChecksum_Add_7053 0x18 // 校表参数校验和寄存器
#define EMUSR 0x19                // EMU 状态寄存器
#define SYSSTA 0x1A               // 系统状态寄存器
#define ChipID 0x1B               // ChipID，默认值为 7053B0
#define DeviceID 0x1C             // DeviceID，默认值为 705321

#define PowerP1  0x0A
#define PowerQ1  0X0B
#define Power_S  0X0C
#define Energy_P  0X0D
#define Energy_Q  0X0E

/************************ATT7053校表寄存器********************************************/
#define EMUIE 0X30       // EMU 中断使能寄存器
#define EMUIF 0X31       // EMU 中断标志寄存器
#define WPREG 0X32       // 写保护寄存器
#define SRSTREG 0X33     // 软件复位寄存器
#define EMUCFG 0X40      // EMU 配置寄存器
#define FreqCFG 0x41     // 时钟/更新频率配置寄存器
#define ModuleEn 0x42    // EMU 模块使能寄存器
#define ANAEN 0x43       // 模拟模块使能寄存器
#define ADCCON 0x59      // ADC 通道增益选择
#define I2Gain 0x5b      // 电流通道 2 增益补偿
#define I1Off 0x5c       // 电流通道 1 的偏置校正
#define I2Off 0x5d       // 电流通道 2 的偏置校正
#define UOff 0x5e        // 电压通道的偏置校正
#define RMSStart 0x60    // 有效值启动值设置寄存器
#define Hfconst  0x61
#define I1RMSOFFSET 0x69 // 通道 1 有效值补偿寄存器，为 8bit 无符号数
#define I2RMSOFFSET 0x6a // 通道 2 有效值补偿寄存器，为 8bit 无符号数
#define URMSOFFSET 0x6b  // 电压通道（通道 3）有效值补偿寄存器，为 8bit无符号数

#define GP1 0X50       // 通道1有功功率校正
#define GQ1 0X51       // 通道1无功功率校正
#define GS1 0X52       // 通道1视在功率校正
#define GPHS1 0X6D       // 通道1相位校正





/* HT7036特殊寄存器 */

#define Adjust_Zero   0xC3           // 清校表数据
#define Adjust_Read   0xC6           // 读数据选择  0x00005A读校表 其他读计量
#define Adjust_Write   0xC9           // 校表写  0x00005A使能
#define Adjust_Enable  0x00005AUL    
#define Adjust_Disable  0x000001UL  
/****************************HT7036
计量数据寄存器***********************************/
#define r_DeviceID_36 0x5D           // HT7036 Device ID  复位值0x7122A0

#define r_Pa 0x01                 // A相有功功率
#define r_Pb 0x02                 // B相有功功率
#define r_Pc 0x03                 // C相有功功率
#define r_Pt 0x04                 // 合相有功功率
#define r_Qa 0x05                 // A相无功功率
#define r_Qb 0x06                 // B相无功功率
#define r_Qc 0x07                 // C相无功功率
#define r_Qt 0x08                 // 合相无功功率
#define r_Sa 0x09                 // A相视在功率
#define r_Sb 0x0A                 // B相视在功率
#define r_Sc 0x0B                 // C相视在功率
#define r_St 0x0C                 // 合相视在功率
#define r_UaRms 0x0D              // A相电压有效值
#define r_UbRms 0x0E              // B相电压有效值
#define r_UcRms 0x0F              // C相电压有效值
#define r_IaRms 0x10              // A相电流有效值
#define r_IbRms 0x11              // B相电流有效值
#define r_IcRms 0x12              // C相电流有效值
#define r_ItRms 0x13              // 三相电流矢量和的有效值
#define r_Pfa 0x14                // A相功率因数
#define r_Pfb 0x15                // B相功率因数
#define r_Pfc 0x16                // C相功率因数
#define r_Pft 0x17                // 合相功率因数
#define r_Pga 0x18                // A相电流与电压相角
#define r_Pgb 0x19                // B相电流与电压相角
#define r_Pgc 0x1A                // C相电流与电压相角
#define r_INTFlag 0x1B            // 中断标志，读后清零
#define r_Freq 0x1C               // 线网频率
#define r_EFlag 0x1D              // 电能寄存器的工作状态，读后清零
#define r_Epa 0x1E                // A相有功电能（可配置为读后清零）
#define r_Epb 0x1F                // B相有功电能（可配置为读后清零）
#define r_Epc 0x20                // C相有功电能（可配置为读后清零）
#define r_Ept 0x21                // 合相有功电能（可配置为读后清零）
#define r_Eqa 0x22                // A相无功电能（可配置为读后清零）
#define r_Eqb 0x23                // B相无功电能（可配置为读后清零）
#define r_Eqc 0x24                // C相无功电能（可配置为读后清零）
#define r_Eqt 0x25                // 合相无功电能（可配置为读后清零）
#define r_YUaUb 0x26              // Ua与Ub的电压夹角
#define r_YUaUc 0x27              // Ua与Uc的电压夹角
#define r_YUbUc 0x28              // Ub与Uc的电压夹角
#define r_UtRms 0x2B              // 三相电压矢量和的有效值
#define R_Sflag 0x2C              // 存放断相、相序、SIG等标志状态
#define r_BckReg_36 0x2D             // 通讯数据备份寄存器
#define COMChecksum_Add_7036 0x2e // 通讯校验和寄存器1,2

#define r_Pflag 0x3D              // 有功/无功功率方向，正向为0，负向为1

#define Checksum_Register1 0x3e  //校表参数和寄存器  01-0x39

#define r_LineUaRrms 0x48         // 基波/谐波A相电压有效值
#define r_LineUbRrms 0x49         // 基波/谐波B相电压有效值
#define r_LineUcRrms 0x4A         // 基波/谐波C相电压有效值
#define r_LineIaRrms 0x4B         // 基波/谐波A相电流有效值
#define r_LineIbRrms 0x4C         // 基波/谐波B相电流有效值
#define r_LineIcRrms 0x4D         // 基波/谐波C相电流有效值
#define r_SAGFlag 0X4F            // SAG标志寄存器
#define r_PeakUa 0X50             // A相电压最大值
#define r_PeakUb 0X51             // B相电压最大值
#define r_PeakUc 0X52             // C相电压最大值
#define r_ChipID 0X5D             // 芯片版本指示寄存器  复位值0x7022E0
#define Checksum_Register2 0x5e  //校表参数和寄存器 0x60-0x71
#define r_PtrWavebuff 0X7E        // 缓冲数据指针，指示内部缓冲buffer已有数据长度
#define r_WaveBuff 0X7F           // 缓冲数据寄存器，内部自增益，重复读取直至读完缓冲数据长度

/************************HT7036校表寄存器********************************************/
#define w_ModeCfg 0x01     // 模式相关控制
#define w_PGACtrl 0x02     // ADC 增益选择
#define w_EMUCfg 0x03      // EMU 模块配置 寄存器
#define w_PgainA 0x04      // A相有功功率增益
#define w_PgainB 0x05      // B相有功功率增益
#define w_PgainC 0x06      // C相有功功率增益
#define w_QgainA 0x07      // A相无功功率增益
#define w_QgainB 0x08      // B相无功功率增益
#define w_QgainC 0x09      // C相无功功率增益
#define w_SgainA 0x0A      // A相无功功率增益
#define w_SgainB 0x0B      // B相无功功率增益
#define w_SgainC 0x0C      // C相无功功率增益
#define w_PhSregApq0 0x0D  // A相相位校正0
#define w_PhSregBpq0 0x0E  // B相相位校正0
#define w_PhSregCpq0 0x0F  // C相相位校正0
#define w_PhSregApq1 0x10  // A相相位校正1
#define w_PhSregBpq1 0x11  // B相相位校正1
#define w_PhSregCpq1 0x12  // C相相位校正1
#define w_PoffsetA 0X13    // A相有功功率offset校正
#define w_PoffsetB 0X14    // B相有功功率offset校正
#define w_PoffsetC 0X15    // C相有功功率offset校正
#define w_QPhscal 0X16     // 无功相位校正
#define w_UgainA 0X17      // A相电压增益
#define w_UgainB 0X18      // B相电压增益
#define w_UgainC 0X19      // C相电压增益
#define w_IgainA 0X1A      // A相电流增益
#define w_IgainB 0X1B      // B相电流增益
#define w_IgainC 0X1C      // C相电流增益
#define w_Istarup 0X1D     // 起动电流阈值设置
#define w_Hfconst 0X1E     // 高频脉冲输出设置
#define w_FailVoltage 0X1F // 失压阈值设置（三相四线模式）(复位0x0600)失压阈值设置（三相四线模式）(复位值0x1200)
#define w_QoffsetA 0x21    // A相无功功率offset校正
#define w_QoffsetB 0x22    // B相无功功率offset校正
#define w_QoffsetC 0x23    // C相无功功率offset校正
#define w_UaRmsoffse 0X24  // A相电压有效值offset校正
#define w_UbRmsoffse 0X25  // B相电压有效值offset校正
#define w_UcRmsoffse 0X26  // C相电压有效值offset校正
#define w_IaRmsoffse 0X27  // A相电流有效值offset校正
#define w_IbRmsoffse 0X28  // B相电流有效值offset校正
#define w_IcRmsoffse 0X29  // C相电流有效值offset校正
#define w_EMUIE 0x30       // 中断使能
#define w_ModuleCFG 0x31   // 模拟模块使能寄存器
#define w_Pstartup 0x36    // 起动功率阈值设置
#define w_Iregion0 0x37    // 相位补偿区域设置寄存器
#define w_Cyclength 0x38   // SAG数据长度设置寄存器
#define w_SAGLvl 0x39      // SAG检测阈值设置寄存器
#define w_Iregion1 0x60    // 相位补偿区域设置寄存器1
#define w_PhSregApq2 0x61  // A相相位校正2
#define w_PhSregBpq2 0x62  // B相相位校正2
#define w_PhSregCpq2 0x63  // C相相位校正2
#define w_PoffsetAL 0X64   // A相有功功率offset校正低字节
#define w_PoffsetBL 0X65   // B相有功功率offset校正低字节
#define w_PoffsetCL 0X66   // C相有功功率offset校正低字节
#define w_QoffsetAL 0X67   // A相无功功率offset校正低字节
#define w_QoffsetBL 0X68   // B相无功功率offset校正低字节
#define w_QoffsetCL 0X69   // C相无功功率offset校正低字节
#define w_ItRmsoffset 0X6A // 电流矢量和offset校正寄存器
#define w_TPSoffset 0X6B   // TPS初值校正寄存器
#define w_TPSgain 0X6C     // TPS斜率校正寄存器
#define w_TCcoffA 0X6D     // Vrefgain的二次系数
#define w_TCcoffB 0X6E     // Vrefgain的一次系数
#define w_TCcoffC 0X6F     // Vrefgain的常数项
#define w_EMCfg 0X70       // 新增算法控制寄存器
#define w_OILVL 0X71       // 过流阈值设置寄存器

#define Meter_Ec 3200 // 电表常数
#define Vu 0.08      // 电压通道采样电压
#define Vi 0.025      // 电流通道采样电压
#define Un 220        // 额定电压
#define In 5          // 额定电流
#define Meter_G 1.163 // ATT7022E常数
// #define Meter_HFConst (int)((2.592 * pow(10, 10) * Meter_G * Meter_G * Vu * Vi) / (In * Un * Meter_Ec))
// #define Meter_K (2.592 * pow(10, 10) / (Meter_HFConst * Meter_Ec * pow(2, 23)))
#define Meter_HFConst (44)
#define Meter_K (0.07404275)*2




// 7036 INTFlag寄存器标志位
#define HT7036_FLAG_UNCALIB    0x01  // 未校表
#define HT7036_FLAG_VALID_UPD  0x02  // 有效值更新
#define HT7036_FLAG_VA_ZERO    0x04  // A相电压过零
#define HT7036_FLAG_VB_ZERO    0x08  // B相电压过零
#define HT7036_FLAG_VC_ZERO    0x10  // C相电压过零
#define HT7036_FLAG_SAG        0x40  // 欠压SAG

// 7053 EMUIF寄存器标志位
#define HT7053_FLAG_SPI_ERR    0x01  // SPI通讯错误
#define HT7053_FLAG_VOLT_ZERO  0x02  // 电压过零
#define HT7053_FLAG_VALID_UPD  0x80  // 有效值更新
#define HT7053_FLAG_CURR_ZERO  0x400 // 电流过零




/* 变量 */
typedef struct
{
    uint8_t I_Amp_Factor;        // 电流放大倍数
    uint8_t V_Amp_Factor;        // 电压放大倍数
    uint8_t P_Gain_compensation; // A相有功功率补偿系数
    uint8_t Ph_compensation;     // A相相位补偿补偿系数
} meter_gain;

typedef enum
{
    OK =0,
	  READ_ERR,
	  WRITE_ERR,
    PARA_ERR,  //参数错误
    CHECKSUM_ERR, //校验错误
	  BCKREG_ERR,  //备份数据错误
	
} Meter_Fault_Type;

// 芯片类型枚举
typedef enum {
    CHIP_TYPE_7036 = 0,
    CHIP_TYPE_7053
} ChipType;

// 单芯片监控数据
typedef struct {
    uint8_t chip_id;          // 芯片编号（1~4）
    ChipType type;            // 芯片类型（7036/7053）
    uint8_t err_flag;         // 错误标志（0-正常，1-spi通讯错误 2-未校表）
    uint8_t data_ready;	      //采集到数据标志
    uint32_t check1;	      //校表和1
    uint32_t check2;	      //校表和2	
	  uint32_t read_state_err;  //读写状态错误次数
	  uint32_t read_data_err;  //读写数据错误次数	
    // 计量数据存储（根据实际需求扩展）
    uint32_t u_a;             // A相电压有效值（V）
    uint32_t u_b;             // B相电压有效值（V）
    uint32_t u_c;             // C相电压有效值（V）
    uint32_t i_a;             // A相电流有效值（A）
    uint32_t i_b;             // B相电流有效值（A）
    uint32_t i_c;             // C相电流有效值（A）
    uint32_t rp;             // 有功	
    uint32_t rq;  // 无功功率（mVar）
	
} MeterChipStatus;


extern MeterChipStatus  g_meter_chip[4];




uint8_t Ht7036_init(void);



char Ht7036_Config(u8 num);
void Ht7036_Read(void);
void Ht7036_Adj_Zero(void);
char Ht7036_Zero_Check(void);
void Ht7036_Adj_Gain(void);
char Ht7036_Gain_Check(void);
void Ht7036_Adj_Power(void);
char Ht7036_Power_Check(void);
void Ht7036_Adj_Phase(void);
char Ht7036_Phase_Check(void);
unsigned char  Ht7036_Adjust_Data_Save(void);
void Ht7036_Type_Change(u8 type) ;
void Ht7036_Energy_Clear(void);
void Ht_7036_Rest(void);
char Ht7036_Pf_Check(void);

#endif
