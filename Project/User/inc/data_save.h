#ifndef __DATA_SAVE_H__  // 头文件保护宏：避免重复包含
#define __DATA_SAVE_H__

/************************* 头文件包含（按依赖优先级排序） *************************/
#include "Type_def.h"       // 项目配置头文件（包含硬件引脚、系统参数等基础配置）

/************************* 版本信息（便于版本管理和维护） *************************/
#define DATA_SAVE_VER_MAJOR    1       /* 主版本号：1（核心架构不变） */
#define DATA_SAVE_VER_MINOR    1       /* 次版本号：1（新增返回值、优化结构体定义） */
#define DATA_SAVE_VER_REVISION 0       /* 修订号：0（无小范围修改） */

#define VER_MAIN       0x01    // 主版本号（v1.xx）
#define VER_SUB        0x17    // 次版本号（vx.23）
#define HW_VERSION     0x01    // 硬件版本号（V1.0）

#define PARA_VERSION     0x04    // 参数版本号，和烧的默认版本参数一致，默认版本参数04

/************************* 硬件相关宏定义（校表数据） *************************/
#define HT7036_CALIB_LEN    20        /* HT7036校表数据长度：20字节/通道 */
#define HT7053_CALIB_LEN    20        /* HT7053校表数据长度：20字节/通道 */

/************************* 存储地址空间分配说明（FM31256铁电存储） *************************
| 存储区域         | 地址范围      | 长度    | 用途                          | 备注                     |
|------------------|---------------|---------|-------------------------------|--------------------------|
| 校表数据区       | 0x0000~0x1FFF | 8KB     | 校表数据（主区+备份区）       | 写保护，独立于参数区     |
| 系统控制区       | 0x2000~0x2003 | 4字节   | 参数版本+事件日志索引+修改日志索引 | 固定地址，无冲突         |
| 主参数区         | 0x2004~0x2103 | 256字节 | 所有业务参数（含预留）        | 可扩展新增参数           |
| 备份参数区       | 0x2104~0x2203 | 256字节 | 主参数区完整备份              | 与主区偏移0x100字节      |
| 参数修改日志区   | 0x2204~0x2843 | 1600字节| 100条×16字节（参数修改记录）  | 循环覆盖，先进先出       |
| 系统事件记录区   | 0x2844~0x2CF3 | 1200字节| 100条×12字节（故障/操作事件） | 循环覆盖，先进先出       |
| 预留扩展区       | 0x2CF4~0x7FFF | ~18KB   | 未来扩展（新增参数/日志）     | 暂未使用                 |
************************************************************************************/

/************************* 基础宏定义（C89兼容，标准化注释） *************************/
/* 时间相关宏（日志时间戳用） */
#define BASE_YEAR          2000        /* 时间基准年：用于年份偏移计算（兼容旧版时间存储） */
#define YEAR_OFFSET        100         /* 年份偏移量：year>=100 → 21xx，year<100 → 20xx */

/* CRC8校验宏（工业标准） */
#define CRC8_POLY          0x31        /* CRC8多项式：x^8+x^5+x^4+1（0x31） */
#define CRC8_INIT          0xFF        /* CRC8初始值：0xFF（通用初始值） */

/* 日志相关宏（参数修改/系统事件） */
#define MODIFY_LOG_CNT     100         /* 参数修改日志最大条数：100条（循环覆盖） */
#define MODIFY_LOG_LEN     16          /* 单条参数修改日志长度：16字节 */
#define EVENT_LOG_CNT      100         /* 系统事件日志最大条数：100条（循环覆盖） */
#define EVENT_LOG_LEN      12          /* 单条系统事件日志长度：12字节（严格对齐） */

/* 日志操作类型宏（替代魔法数，提升可读性） */
#define OP_TYPE_ADD        0x00        /* 日志操作类型：参数初始化（首次添加） */
#define OP_TYPE_MODIFY     0x01        /* 日志操作类型：参数手动修改 */
#define OP_TYPE_RESET_DEF  0x02        /* 日志操作类型：参数恢复默认值 */
#define OP_TYPE_CLEAR_LOG  0x03        /* 日志操作类型：日志手动清空 */

/* 参数区宏（主备区管理） */
#define MAIN_PARAM_TOTAL   0x100       /* 主参数区总长度：256字节（含预留空间） */
#define BACKUP_PARAM_OFFSET 0x100      /* 主备参数区偏移量：256字节（地址差） */
#define PARA_CRC_LEN       1           /* 单个参数CRC8校验值长度：1字节（每个参数后紧跟CRC） */

/************************* 多通道电容参数扩展宏（4通道适配） *************************/
#define CAP_CHANNEL_CNT     4           /* 电容参数通道数：4路（1~4通道，代码中ch=0~3） */
/* 单通道参数总长度=数据长度+CRC长度（手动计算适配C89预处理） */
#define CAP_PARA_PER_CH_LEN     5       /* 单通道电容参数总长度：3字节数据 + 1字节CRC */
#define CAP_PROTECT_PARA_LEN    15      /* 单通道电容保护参数数据长度：15字节 */
#define CAP_PROTECT_PER_CH_LEN  16      /* 单通道电容保护参数总长度：15字节数据 + 1字节CRC */
#define CAP_RATIO_PARA_LEN      3       /* 单通道电容变比参数数据长度：3字节 */
#define CAP_RATIO_PER_CH_LEN    4       /* 单通道电容变比参数总长度：3字节数据 + 1字节CRC */

/************************* 存储地址定义（统一命名+精准注释） *************************/
/* 校表数据区地址（主备区分离） */
#define FM31256_CALIB_VISON     0x0000 /* 校表数据主区版本地址：存储校表数据版本号 */
#define FM31256_CALIB_START     0x0003 /* 校表数据主区起始地址：4通道校表数据起始 */
#define FM31256_CALIB_BACK_VISON 0x1000 /* 校表数据备区版本地址：备区校表数据版本号 */
#define FM31256_CALIB_BACKUP    0x1003 /* 校表数据备区起始地址：备区4通道校表数据起始 */
#define FM31256_CALIB_END       0x1FFF /* 校表数据区结束地址：8KB校表区边界 */

/* 系统控制区地址（0x2000~0x2003，共4字节） */
#define SYS_PARAM_START         0x2000  /* 系统参数总起始地址：控制区+参数区总起始 */
#define PARAM_VERSION_ADDR      0x2000  /* 参数版本地址：2字节（版本号+CRC8校验） */
#define EVENT_INDEX_ADDR        0x2002  /* 事件日志索引地址：1字节（下一条日志存储位置） */
#define LOG_INDEX_ADDR          0x2003  /* 修改日志索引地址：1字节（下一条日志存储位置） */

/* 主/备参数区地址（核心业务参数存储） */
#define MAIN_PARAM_START        0x2004  /* 主参数区起始地址：业务参数主区起始 */
#define MAIN_PARAM_END          (MAIN_PARAM_START + MAIN_PARAM_TOTAL - 1) /* 主参数区结束地址：0x2103 */
#define BACKUP_PARAM_START      (MAIN_PARAM_START + BACKUP_PARAM_OFFSET) /* 备份参数区起始地址：0x2104 */
#define BACKUP_PARAM_END        (BACKUP_PARAM_START + MAIN_PARAM_TOTAL - 1) /* 备份参数区结束地址：0x2203 */

/* 日志区地址（参数修改/系统事件） */
#define MODIFY_LOG_START        (BACKUP_PARAM_END + 1) /* 参数修改日志起始地址：0x2204 */
#define MODIFY_LOG_TOTAL        (MODIFY_LOG_CNT * MODIFY_LOG_LEN) /* 修改日志总长度：1600字节 */
#define MODIFY_LOG_END          (MODIFY_LOG_START + MODIFY_LOG_TOTAL - 1) /* 修改日志结束地址：0x2843 */
#define EVENT_LOG_START         (MODIFY_LOG_END + 1) /* 系统事件日志起始地址：0x2844 */
#define EVENT_LOG_TOTAL         (EVENT_LOG_CNT * EVENT_LOG_LEN) /* 事件日志总长度：1200字节 */
#define EVENT_LOG_END           (EVENT_LOG_START + EVENT_LOG_TOTAL - 1) /* 事件日志结束地址：0x2CF3 */

/* 地址合法性检查（C89兼容，纯常量表达式） */
#if (EVENT_LOG_END) > 0x7FFFUL
#error "Storage overflow! Reduce log/event count or param reserved size."
#endif

/************************* 校表数据地址映射（4通道） *************************/
#define CALIB1_MAIN_DATA        FM31256_CALIB_START          /* 1路校表数据主区地址 */
#define CALIB1_BACKUP_DATA      FM31256_CALIB_BACKUP         /* 1路校表数据备区地址 */
#define CALIB2_MAIN_DATA        (CALIB1_MAIN_DATA + HT7036_CALIB_LEN) /* 2路校表数据主区地址 */
#define CALIB2_BACKUP_DATA      (CALIB1_BACKUP_DATA + HT7036_CALIB_LEN) /* 2路校表数据备区地址 */
#define CALIB3_MAIN_DATA        (CALIB2_MAIN_DATA + HT7053_CALIB_LEN) /* 3路校表数据主区地址 */
#define CALIB3_BACKUP_DATA      (CALIB2_BACKUP_DATA + HT7053_CALIB_LEN) /* 3路校表数据备区地址 */
#define CALIB4_MAIN_DATA        (CALIB3_MAIN_DATA + HT7036_CALIB_LEN) /* 4路校表数据主区地址 */
#define CALIB4_BACKUP_DATA      (CALIB3_BACKUP_DATA + HT7036_CALIB_LEN) /* 4路校表数据备区地址 */

/************************* 业务参数地址映射（主/备区，按存储顺序） *************************/
/* 1. 通讯参数（主备区） */
#define COM_PARA_MAIN_DATA      (MAIN_PARAM_START + 0x00UL)  /* 通讯参数主区数据地址：0x2004 */
#define COM_PARA_MAIN_CRC       (COM_PARA_MAIN_DATA + sizeof(Com_Para_Struct)) /* 通讯参数主区CRC地址 */
#define COM_PARA_BACKUP_DATA    (COM_PARA_MAIN_DATA + BACKUP_PARAM_OFFSET) /* 通讯参数备区数据地址：0x2104 */
#define COM_PARA_BACKUP_CRC     (COM_PARA_BACKUP_DATA + sizeof(Com_Para_Struct)) /* 通讯参数备区CRC地址 */

/* 2. 过压保护参数（主备区） */
#define OV_PROTECT_MAIN_DATA    (COM_PARA_MAIN_CRC + PARA_CRC_LEN) /* 过压保护参数主区数据地址 */
#define OV_PROTECT_MAIN_CRC     (OV_PROTECT_MAIN_DATA + sizeof(Protect_Para_Struct)) /* 过压保护参数主区CRC地址 */
#define OV_PROTECT_BACKUP_DATA  (OV_PROTECT_MAIN_DATA + BACKUP_PARAM_OFFSET) /* 过压保护参数备区数据地址 */
#define OV_PROTECT_BACKUP_CRC   (OV_PROTECT_BACKUP_DATA + sizeof(Protect_Para_Struct)) /* 过压保护参数备区CRC地址 */

/* 3. 欠压保护参数（主备区） */
#define UV_PROTECT_MAIN_DATA    (OV_PROTECT_MAIN_CRC + PARA_CRC_LEN) /* 欠压保护参数主区数据地址 */
#define UV_PROTECT_MAIN_CRC     (UV_PROTECT_MAIN_DATA + sizeof(Protect_Para_Struct)) /* 欠压保护参数主区CRC地址 */
#define UV_PROTECT_BACKUP_DATA  (UV_PROTECT_MAIN_DATA + BACKUP_PARAM_OFFSET) /* 欠压保护参数备区数据地址 */
#define UV_PROTECT_BACKUP_CRC   (UV_PROTECT_BACKUP_DATA + sizeof(Protect_Para_Struct)) /* 欠压保护参数备区CRC地址 */

/* 4. 控制参数（主备区） */
#define CONTROL_PARA_MAIN_DATA  (UV_PROTECT_MAIN_CRC + PARA_CRC_LEN) /* 控制参数主区数据地址 */
#define CONTROL_PARA_MAIN_CRC   (CONTROL_PARA_MAIN_DATA + sizeof(Control_Para_Struct)) /* 控制参数主区CRC地址 */
#define CONTROL_PARA_BACKUP_DATA (CONTROL_PARA_MAIN_DATA + BACKUP_PARAM_OFFSET) /* 控制参数备区数据地址 */
#define CONTROL_PARA_BACKUP_CRC (CONTROL_PARA_BACKUP_DATA + sizeof(Control_Para_Struct)) /* 控制参数备区CRC地址 */

/* 5. 电容参数（多通道，主备区） */

#define CAP_PARA_MAIN_DATA_CH(ch)  (CONTROL_PARA_MAIN_CRC +PARA_CRC_LEN+ (ch) * CAP_PARA_PER_CH_LEN) /* 第ch通道电容参数主区数据地址 */
#define CAP_PARA_MAIN_CRC_CH(ch)   (CAP_PARA_MAIN_DATA_CH(ch) + sizeof(Cap_Para_Struct)) /* 第ch通道电容参数主区CRC地址 */
#define CAP_PARA_BACKUP_DATA_CH(ch) (CAP_PARA_MAIN_DATA_CH(ch) + BACKUP_PARAM_OFFSET) /* 第ch通道电容参数备区数据地址 */
#define CAP_PARA_BACKUP_CRC_CH(ch)  (CAP_PARA_BACKUP_DATA_CH(ch) + sizeof(Cap_Para_Struct)) /* 第ch通道电容参数备区CRC地址 */

/* 6. 变比参数（主备区） */
#define RATIO_PARA_MAIN_DATA    (CAP_PARA_MAIN_CRC_CH(CAP_CHANNEL_CNT-1) + PARA_CRC_LEN) /* 变比参数主区数据地址 */
#define RATIO_PARA_MAIN_CRC     (RATIO_PARA_MAIN_DATA + sizeof(Ratio_Para_Struct)) /* 变比参数主区CRC地址 */
#define RATIO_PARA_BACKUP_DATA  (RATIO_PARA_MAIN_DATA + BACKUP_PARAM_OFFSET) /* 变比参数备区数据地址 */
#define RATIO_PARA_BACKUP_CRC   (RATIO_PARA_BACKUP_DATA + sizeof(Ratio_Para_Struct)) /* 变比参数备区CRC地址 */

/* 7. 统计参数（主备区） */
#define STAT_PARA_MAIN_DATA     (RATIO_PARA_MAIN_CRC + PARA_CRC_LEN) /* 统计参数主区数据地址 */
#define STAT_PARA_MAIN_CRC      (STAT_PARA_MAIN_DATA + sizeof(Stat_Para_Struct)) /* 统计参数主区CRC地址 */
#define STAT_PARA_BACKUP_DATA   (STAT_PARA_MAIN_DATA + BACKUP_PARAM_OFFSET) /* 统计参数备区数据地址 */
#define STAT_PARA_BACKUP_CRC    (STAT_PARA_BACKUP_DATA + sizeof(Stat_Para_Struct)) /* 统计参数备区CRC地址 */

/* 8. 口令参数（主备区） */
#define PASS_PARA_MAIN_DATA     (STAT_PARA_MAIN_CRC + PARA_CRC_LEN) /* 口令参数主区数据地址 */
#define PASS_PARA_MAIN_CRC      (PASS_PARA_MAIN_DATA + sizeof(Pass_Para_Struct)) /* 口令参数主区CRC地址 */
#define PASS_PARA_BACKUP_DATA   (PASS_PARA_MAIN_DATA + BACKUP_PARAM_OFFSET) /* 口令参数备区数据地址 */
#define PASS_PARA_BACKUP_CRC    (PASS_PARA_BACKUP_DATA + sizeof(Pass_Para_Struct)) /* 口令参数备区CRC地址 */

/* 9. 手自动参数（主备区） */
#define AUTO_MANUAL_PARA_MAIN_DATA (PASS_PARA_MAIN_CRC + PARA_CRC_LEN) /* 手自动参数主区数据地址 */
#define AUTO_MANUAL_PARA_MAIN_CRC  (AUTO_MANUAL_PARA_MAIN_DATA + sizeof(AutoManual_Para_Struct)) /* 手自动参数主区CRC地址 */
#define AUTO_MANUAL_PARA_BACKUP_DATA (AUTO_MANUAL_PARA_MAIN_DATA + BACKUP_PARAM_OFFSET) /* 手自动参数备区数据地址 */
#define AUTO_MANUAL_PARA_BACKUP_CRC  (AUTO_MANUAL_PARA_BACKUP_DATA + sizeof(AutoManual_Para_Struct)) /* 手自动参数备区CRC地址 */

/* 10. 对比度参数（主备区） */
#define CONTRA_PARA_MAIN_DATA   (AUTO_MANUAL_PARA_MAIN_CRC + PARA_CRC_LEN) /* 对比度参数主区数据地址 */
#define CONTRA_PARA_MAIN_CRC    (CONTRA_PARA_MAIN_DATA + sizeof(Contra_Para_Struct)) /* 对比度参数主区CRC地址 */
#define CONTRA_PARA_BACKUP_DATA (CONTRA_PARA_MAIN_DATA + BACKUP_PARAM_OFFSET) /* 对比度参数备区数据地址 */
#define CONTRA_PARA_BACKUP_CRC  (CONTRA_PARA_BACKUP_DATA + sizeof(Contra_Para_Struct)) /* 对比度参数备区CRC地址 */

/* 11. 报警参数（主备区） */
#define ALARM_PARA_MAIN_DATA    (CONTRA_PARA_MAIN_CRC + PARA_CRC_LEN) /* 报警参数主区数据地址 */
#define ALARM_PARA_MAIN_CRC     (ALARM_PARA_MAIN_DATA + sizeof(Alarm_Para_Struct)) /* 报警参数主区CRC地址 */
#define ALARM_PARA_BACKUP_DATA  (ALARM_PARA_MAIN_DATA + BACKUP_PARAM_OFFSET) /* 报警参数备区数据地址 */
#define ALARM_PARA_BACKUP_CRC   (ALARM_PARA_BACKUP_DATA + sizeof(Alarm_Para_Struct)) /* 报警参数备区CRC地址 */

/* 12. 电容保护参数（多通道，主备区） */
#define CAP_PROTECT_MAIN_DATA_CH(ch)  (ALARM_PARA_MAIN_CRC + PARA_CRC_LEN + (ch) * CAP_PROTECT_PER_CH_LEN) /* 第ch通道电容保护参数主区数据地址 */
#define CAP_PROTECT_MAIN_CRC_CH(ch)   (CAP_PROTECT_MAIN_DATA_CH(ch) + CAP_PROTECT_PARA_LEN) /* 第ch通道电容保护参数主区CRC地址 */
#define CAP_PROTECT_BACKUP_DATA_CH(ch) (CAP_PROTECT_MAIN_DATA_CH(ch) + BACKUP_PARAM_OFFSET) /* 第ch通道电容保护参数备区数据地址 */
#define CAP_PROTECT_BACKUP_CRC_CH(ch)  (CAP_PROTECT_BACKUP_DATA_CH(ch) + CAP_PROTECT_PARA_LEN) /* 第ch通道电容保护参数备区CRC地址 */

/* 13. 电容变比参数（多通道，主备区） */
#define CAP_RATIO_MAIN_DATA_CH(ch)    (CAP_PROTECT_MAIN_CRC_CH(CAP_CHANNEL_CNT-1) + PARA_CRC_LEN + (ch) * CAP_RATIO_PER_CH_LEN) /* 第ch通道电容变比参数主区数据地址 */
#define CAP_RATIO_MAIN_CRC_CH(ch)     (CAP_RATIO_MAIN_DATA_CH(ch) + CAP_RATIO_PARA_LEN) /* 第ch通道电容变比参数主区CRC地址 */
#define CAP_RATIO_BACKUP_DATA_CH(ch)  (CAP_RATIO_MAIN_DATA_CH(ch) + BACKUP_PARAM_OFFSET) /* 第ch通道电容变比参数备区数据地址 */
#define CAP_RATIO_BACKUP_CRC_CH(ch)   (CAP_RATIO_BACKUP_DATA_CH(ch) + CAP_RATIO_PARA_LEN) /* 第ch通道电容变比参数备区CRC地址 */

/* 14. 保护路数参数（多通道，主备区） */

#define CAP_NUM_MAIN_DATA    (CAP_RATIO_MAIN_CRC_CH(CAP_CHANNEL_CNT-1) + PARA_CRC_LEN) /* 保护路数主区数据地址 */
#define CAP_NUM__MAIN_CRC     (CAP_NUM_MAIN_DATA + sizeof(Cap_Num_Struct)) /* 保护路数主区CRC地址 */
#define CAP_NUM__BACKUP_DATA  (CAP_NUM_MAIN_DATA + BACKUP_PARAM_OFFSET) /* 保护路数备区数据地址 */
#define CAP_NUM__BACKUP_CRC   (CAP_NUM__BACKUP_DATA + sizeof(Cap_Num_Struct)) /* 保护路数备区CRC地址 */


/* 主参数区溢出检查（C89兼容，纯常量表达式） */
//#define CAP_RATIO_LAST_CRC      (CAP_RATIO_MAIN_CRC_CH(CAP_CHANNEL_CNT-1) + PARA_CRC_LEN)
//#if CAP_RATIO_LAST_CRC > MAIN_PARAM_END
//#error "Main parameter area overflow! Reduce CAP_CHANNEL_CNT or shorten struct length."
//#endif

/************************* 日志地址计算宏（快速定位日志位置） *************************/
#define MODIFY_LOG_ADDR(n)      (MODIFY_LOG_START + (n)*MODIFY_LOG_LEN) /* 第n条参数修改日志地址（n=0~99） */
#define EVENT_LOG_ADDR(n)       (EVENT_LOG_START + (n)*EVENT_LOG_LEN)  /* 第n条系统事件日志地址（n=0~99） */

/************************* 默认参数定义（标准化，贴合实际业务） *************************/
#define COM_PARA_DEFAULT        {0x01, 9600, 0x00}  /* 通讯参数默认值：地址01、波特率9600、无校验 */
#define OV_PROTECT_DEFAULT      {0x00, 1250, 200}   /* 过压保护默认值：禁用、阈值12.50kV、延时200ms */
#define UV_PROTECT_DEFAULT      {0x00, 650, 200}    /* 欠压保护默认值：禁用、阈值6.50kV、延时200ms */
#define CONTROL_PARA_DEFAULT    {115, 95, 0x00, 100, 92, 50, 0x02, 300, 300, 100, 0} /* 控制参数默认值：电压上下限11.5/9.5kV，cos上下限1.00/0.92，限投50次/日，组合投切，系数3.00，投切延时300s，间隔100s */
#define CAP_PARA_DEFAULT        {0x01, 1, 0x01}     /* 电容参数默认值：使能、容量1kvar、投入状态 */
#define RATIO_PARA_DEFAULT      {100, 1000, 5}      /* 变比参数默认值：PT100、CT1000/5A */
#define STAT_PARA_DEFAULT       {0, 0, 0, 0}        /* 统计参数默认值：日/月投切次数全0 */
#define PASS_PARA_DEFAULT       {0}          /* 口令默认值：6位数字000000 */
#define AUTO_MANUAL_PARA_DEFAULT {0x00}             /* 手自动默认值：自动模式 */
#define CONTRA_PARA_DEFAULT     {60}                /* 对比度默认值：60（0~255） */
#define ALARM_PARA_DEFAULT      {0}                 /* 报警参数默认值：0（无报警） */
#define CAP_PROTECT_PARA_DEFAULT {0, 1000, 500, 0, 900, 100, 0, 100, 200} /* 电容保护默认值：过压/速压/零压均禁用，过压阈值10.00kV、延时5.00s，速压阈值9.00kV、延时1.00s，零压阈值1.00kV、延时2.00s */
#define CAP_RATIO_PARA_DEFAULT  {1000, 5}           /* 电容变比默认值：CT1000/5A */
#define CAP_NUM_DEFAULT  {4,4}          							 /* 电容路数默认值：4*/



/************************* 核心枚举类型定义（精准分类，易扩展） *************************/
/**
 * @brief 参数读写操作返回值枚举（精准定位失败原因）
 * @note  0表示成功，非0表示失败，错误码按“从易到难排查”排序
 */
typedef enum {
    PARA_OP_SUCCESS = 0,        /* 操作成功：参数读写/日志操作正常完成 */
    PARA_OP_ERR_IIC,        /* 硬件读取失败：iic通信错误 */	
    PARA_OP_ERR_NULL_PTR,       /* 空指针错误：输入/输出缓冲区为NULL */
    PARA_OP_ERR_CRC_MAIN,       /* 主区CRC校验失败：已从备区恢复数据 */
    PARA_OP_ERR_CRC_BACKUP,     /* 备份区CRC校验失败：主备区均失效 */
    PARA_OP_ERR_ADDR_OVERFLOW,  /* 存储地址溢出：参数区越界（宏定义错误） */
    PARA_OP_ERR_HW_WRITE,       /* 硬件写入失败：31256通信错误 */
    PARA_OP_ERR_PARAM_ID,       /* 参数ID无效：日志/通用接口传入非法参数ID */
    EVENT_OK,                   /* 读取事件成功 */	
    EVENT_ERR,                  /* 无事件 */		
	  CALIB_READ_BUF_ERR,
	  CALIB_READ_CHAN_ERR,
    PARA_OP_ERR_MAX             /* 返回值最大值：用于边界检查，无实际意义 */
} Para_Op_Result_E;

/**
 * @brief 参数ID枚举（用于参数索引/日志记录，一一对应业务参数）
 */
typedef enum {
    PARAM_ID_COM,                  /* 参数ID：通讯参数 */
    PARAM_ID_OV_PROT,              /* 参数ID：过压保护参数 */
    PARAM_ID_UV_PROT,              /* 参数ID：欠压保护参数 */
    PARAM_ID_CONTROL,              /* 参数ID：控制参数 */
    PARAM_ID_CAP_CH1,              /* 参数ID：1路电容参数 */
    PARAM_ID_CAP_CH2,              /* 参数ID：2路电容参数 */
    PARAM_ID_CAP_CH3,              /* 参数ID：3路电容参数 */
    PARAM_ID_CAP_CH4,              /* 参数ID：4路电容参数 */
    PARAM_ID_RATIO,                /* 参数ID：变比参数 */
    PARAM_ID_STAT,                 /* 参数ID：统计参数 */
    PARAM_ID_PASS,                 /* 参数ID：口令参数 */
    PARAM_ID_AUTO_MANUAL,          /* 参数ID：手自动参数 */
    PARAM_ID_CONTRA,               /* 参数ID：对比度参数 */
    PARAM_ID_ALARM,                /* 参数ID：报警参数 */
    PARAM_ID_CAP_PROTECT_CH1,      /* 参数ID：1路电容保护参数 */
    PARAM_ID_CAP_PROTECT_CH2,      /* 参数ID：2路电容保护参数 */
    PARAM_ID_CAP_PROTECT_CH3,      /* 参数ID：3路电容保护参数 */
    PARAM_ID_CAP_PROTECT_CH4,      /* 参数ID：4路电容保护参数 */
    PARAM_ID_CAP_RATIO_CH1,        /* 参数ID：1路电容变比参数 */
    PARAM_ID_CAP_RATIO_CH2,        /* 参数ID：2路电容变比参数 */
    PARAM_ID_CAP_RATIO_CH3,        /* 参数ID：3路电容变比参数 */
    PARAM_ID_CAP_RATIO_CH4,        /* 参数ID：4路电容变比参数 */
    PARAM_ID_MAX                   /* 参数ID最大值：用于边界检查 */
} Param_ID_E;

/**
 * @brief 系统事件类型枚举（故障/操作事件分类，覆盖所有业务场景）
 * @note  事件码按“故障类型+操作类型”分组，便于日志解析
 */
typedef enum {
    EVENT_TYPE_NONE           = 0x0000U,  /* 事件类型：无事件（默认值） */
    /* 过流/速断故障（1~8） */
    EVENT_TYPE_OVER_CURRENT1  = 0x0001U,  /* 事件类型：1路过流故障 */
    EVENT_TYPE_OVER_CURRENT2  = 0x0002U,  /* 事件类型：2路过流故障 */
    EVENT_TYPE_OVER_CURRENT3  = 0x0003U,  /* 事件类型：3路过流故障 */
    EVENT_TYPE_OVER_CURRENT4  = 0x0004U,  /* 事件类型：4路过流故障 */
    EVENT_TYPE_OVER_CURRENT11 = 0x0005U,  /* 事件类型：1路速断故障 */
    EVENT_TYPE_OVER_CURRENT22 = 0x0006U,  /* 事件类型：2路速断故障 */
    EVENT_TYPE_OVER_CURRENT33 = 0x0007U,  /* 事件类型：3路速断故障 */
    EVENT_TYPE_OVER_CURRENT44 = 0x0008U,  /* 事件类型：4路速断故障 */
    /* 零序故障（9~12） */
    EVENT_TYPE_OVER_VOL_ZERO1 = 0x0009U,  /* 事件类型：1路零序过压故障 */
    EVENT_TYPE_OVER_VOL_ZERO2 = 0x000AU,  /* 事件类型：2路零序过压故障 */
    EVENT_TYPE_OVER_VOL_ZERO3 = 0x000BU,  /* 事件类型：3路零序过压故障 */
    EVENT_TYPE_OVER_VOL_ZERO4 = 0x000CU,  /* 事件类型：4路零序过压故障 */
    /* 系统电压故障（13~14） */
    EVENT_TYPE_OVER_VOL       = 0x000DU,  /* 事件类型：系统过压故障 */
    EVENT_TYPE_UNDER_VOL      = 0x000EU,  /* 事件类型：系统欠压故障 */
    /* 拒投故障（15~18） */
    EVENT_TYPE_IN_STOP1       = 0x000FU,  /* 事件类型：1路电容拒投故障 */
    EVENT_TYPE_IN_STOP2       = 0x0010U,  /* 事件类型：2路电容拒投故障 */
    EVENT_TYPE_IN_STOP3       = 0x0011U,  /* 事件类型：3路电容拒投故障 */
    EVENT_TYPE_IN_STOP4       = 0x0012U,  /* 事件类型：4路电容拒投故障 */
    /* 拒切故障（19~22） */
    EVENT_TYPE_QUIT_STOP1     = 0x0013U,  /* 事件类型：1路电容拒切故障 */
    EVENT_TYPE_QUIT_STOP2     = 0x0014U,  /* 事件类型：2路电容拒切故障 */
    EVENT_TYPE_QUIT_STOP3     = 0x0015U,  /* 事件类型：3路电容拒切故障 */
    EVENT_TYPE_QUIT_STOP4     = 0x0016U,  /* 事件类型：4路电容拒切故障 */
    /* 外部故障（23~26） */
    EVENT_TYPE_ERR1           = 0x0017U,  /* 事件类型：1路外部故障 */
    EVENT_TYPE_ERR2           = 0x0018U,  /* 事件类型：2路外部故障 */
    EVENT_TYPE_ERR3           = 0x0019U,  /* 事件类型：3路外部故障 */
    EVENT_TYPE_ERR4           = 0x001AU,  /* 事件类型：4路外部故障 */
    /* 电源故障（27） */
    EVENT_TYPE_POWER_OFF      = 0x001BU,  /* 事件类型：前段总闸断电故障 */
    /* 正常操作（28~39） */
    EVENT_TYPE_IN1            = 0x001CU,  /* 事件类型：1路电容投入操作 */
    EVENT_TYPE_IN2            = 0x001DU,  /* 事件类型：2路电容投入操作 */
    EVENT_TYPE_IN3            = 0x001EU,  /* 事件类型：3路电容投入操作 */
    EVENT_TYPE_IN4            = 0x001FU,  /* 事件类型：4路电容投入操作 */
    EVENT_TYPE_QUIT1          = 0x0020U,  /* 事件类型：1路电容切除操作 */
    EVENT_TYPE_QUIT2          = 0x0021U,  /* 事件类型：2路电容切除操作 */
    EVENT_TYPE_QUIT3          = 0x0022U,  /* 事件类型：3路电容切除操作 */
    EVENT_TYPE_QUIT4          = 0x0023U   /* 事件类型：4路电容切除操作 */
} Event_Type_E;

/************************* 核心结构体定义 *************************/


/**
 * @brief 参数版本结构体（存储于0x2000~0x2001）
 */
typedef struct {
    uint8_t param_version;         /* 参数版本号：V3=0x03（区分不同参数结构版本） */
    uint8_t crc8;                  /* 版本校验位：param_version的CRC8值 */
} Param_Version_Struct;

/**
 * @brief 通讯参数结构体
 * @note  总长度：5字节（address(1)+bps(2)+check(1)）
 */
typedef struct {
    uint8_t address;               /* 通讯地址：0x00~0xFF（Modbus从站地址） */
    uint16_t bps;                  /* 波特率：支持1200/2400/4800/9600 */
    uint8_t  check;                /* 校验方式：0=无校验，1=奇校验，2=偶校验 */
} Com_Para_Struct;

/**
 * @brief 保护参数结构体（过压/欠压通用）
 * @note  总长度：5字节（onf(1)+value(2)+time(2)）
 */
typedef struct {
    uint8_t  onf;                  /* 保护使能：0=禁用，1=启用 */
    uint16_t value;                /* 保护阈值：0.01kV单位（如1250=12.50kV） */
    uint16_t time;                 /* 保护延时：单位ms（如200=200ms） */
} Protect_Para_Struct;

/**
 * @brief 控制参数结构体（电容投切核心参数）
 * @note  总长度：15字节（按实际字段累加）
 */
typedef struct {
    uint16_t vol_up;               /* 电压上限：1位小数，单位kV，10倍存储（如115=11.5kV） */
    uint16_t vol_down;             /* 电压下限：1位小数，单位kV，10倍存储（如95=9.5kV） */
    uint8_t  cos_up_f;             /* cos上限符号：0=正，1=负（预留） */
    uint8_t  cos_up;               /* cos上限：两位小数，×100存储（如100=1.00） */
    uint8_t  cos_down;             /* cos下限：两位小数，×100存储（如92=0.92） */
    uint16_t times;                /* 限投次数：每日最大投切次数（如50=50次/日） */
    uint8_t  type;                 /* 投切方案：1=差容，2=组合，3=滤波，4=等容 */
    uint8_t  factor;               /* 投切系数：两位小数，×100存储（如300=3.00） */
    uint16_t delay_time_on;        /* 投入延时时间：单位s（如300=300s） */
    uint16_t delay_time_off;       /* 切除延时时间：单位s（如300=300s） */
    uint8_t  time_interval;        /* 投切间隔时间：单位s（如100=100s） */
} Control_Para_Struct;

/**
 * @brief 电容参数结构体（单通道）
 * @note  总长度：3字节（onf(1)+value(2)+state(1) → 修正：实际3字节）
 */
typedef struct {
    uint8_t   onf;                 /* 电容使能：0=无（禁用），1=有（启用） */
    uint16_t  value;               /* 电容值：四位数，单位kvar（如1=1kvar，100=100kvar） */
    uint8_t   state;               /* 投切状态：0=不投，1=投入 */
} Cap_Para_Struct;

/**
 * @brief 变比参数结构体（系统级PT/CT变比）
 * @note  总长度：5字节（pt_ratio(2)+ct_ratio(2)+ct_ratio1(1)）
 */
typedef struct {
    uint16_t  pt_ratio;            /* 电压互感器变比：三位数（如100=100:1） */
    uint16_t  ct_ratio;            /* 电流互感器变比：1000格式（如1000=1000:5） */
    uint8_t   ct_ratio1;           /* 电流互感器变比：5A格式（如5=1000:5） */
} Ratio_Para_Struct;

/**
 * @brief 统计参数结构体（投切次数统计）
 * @note  总长度：16字节（4个uint32_t）
 */
typedef struct {
    uint32_t  day_times_on;        /* 日投次数：当日电容投入总次数 */
    uint32_t  day_times_off;       /* 日切次数：当日电容切除总次数 */
    uint32_t  month_times_on;      /* 月投次数：当月电容投入总次数 */
    uint32_t  month_times_off;     /* 月切次数：当月电容切除总次数 */
} Stat_Para_Struct;

/**
 * @brief 口令参数结构体
 * @note  总长度：6字节（6位字符）
 */
typedef struct  {
    char pass_code[6];             /* 口令值：6位数字/字符（如"000000"） */
} Pass_Para_Struct;

/**
 * @brief 手自动参数结构体
 * @note  总长度：1字节
 */
typedef struct  {
    uint8_t mode;                  /* 运行模式：0x00=自动，0x01=手动 */
} AutoManual_Para_Struct;

/**
 * @brief 对比度参数结构体（屏幕显示）
 * @note  总长度：1字节
 */
typedef struct  {
    uint8_t value;                 /* 对比度值：0~255（对应0x00~0xFF，默认60） */
} Contra_Para_Struct;

/**
 * @brief 报警参数结构体
 * @note  总长度：1字节
 */
typedef struct  {
    char value;                 /* 报警值：0~61（对应不同报警类型，0=无报警） */
} Alarm_Para_Struct;

/**
 * @brief 保护路数结构体
 * @note  总长度：1字节
 */
typedef struct  {
    uint8_t cap_num;                 /* 电容路数1,2,3,4 */
    uint8_t pro_num;                 /* 保护路数1,2,3,4 */	
} Cap_Num_Struct;


/**
 * @brief 电容保护参数结构体（单通道，过压/速压/零压）
 * @note  总长度：15字节（按字段累加）
 */
typedef struct {
    uint8_t  over_onf;             /* 过压保护使能：0=退出，1=投入 */
    uint16_t over_value;           /* 过压阈值：两位小数，单位kV（如1000=10.00kV） */
    uint16_t over_time;            /* 过压延时：两位小数，单位s（如500=5.00s） */
    uint8_t  quick_onf;            /* 速压保护使能：0=退出，1=投入 */
    uint16_t quick_value;          /* 速压阈值：两位小数，单位kV（如900=9.00kV） */
    uint16_t quick_time;           /* 速压延时：两位小数，单位s（如100=1.00s） */
    uint8_t  zero_onf;             /* 零压保护使能：0=退出，1=投入 */
    uint16_t zero_value;           /* 零压阈值：两位小数，单位kV（如100=1.00kV） */
    uint16_t zero_time;            /* 零压延时：两位小数，单位s（如200=2.00s） */
} Cap_Protect_Para_Struct;

/**
 * @brief 电容变比参数结构体（单通道CT变比）
 * @note  总长度：3字节（ct_ratio(2)+ct_ratio1(1)）
 */
typedef struct {
    uint16_t  ct_ratio;            /* 电流互感器变比：1000格式（如1000=1000:5） */
    uint8_t   ct_ratio1;           /* 电流互感器变比：5A格式（如5=1000:5） */
} Cap_Ratio_Para_Struct;



/**
 * @brief 事件日志结构体（严格12字节，匹配EVENT_LOG_LEN）
 * @note  字段分布：event_type(2)+event_data(4)+year(1)+month(1)+day(1)+hour(1)+minute(1)+second(1)
 */
typedef struct  {
    uint16_t event_type;           /* 事件类型：Event_Type_E（2字节） */
    uint32_t event_data;           /* 事件关联数据：如过压值、电容通道号等（4字节） */
    /* 6字节时间戳（完整年月日时分秒，无偏移） */
    uint8_t year;                  /* 年份：完整年份（如2025） */
    uint8_t month;                 /* 月份：1~12 */
    uint8_t day;                   /* 日期：1~31 */
    uint8_t hour;                  /* 小时：0~23 */
    uint8_t minute;                /* 分钟：0~59 */
    uint8_t second;                /* 秒：0~59 */
} Event_Log_Struct;


/************************* 变量声明（所有读写函数均带返回值） *************************/

extern Pass_Para_Struct g_protect_password;     // 6位口令
extern Alarm_Para_Struct g_alarm_minutes;        // 报警时间（分钟）
extern Contra_Para_Struct g_lcd_contrast;         // 对比度 0~255
extern  uint8_t g_language;  //语言  0：中午 1：英文

extern Ratio_Para_Struct g_sys_pt_ct;  //系统pt、ct

extern Protect_Para_Struct g_vol_h,g_vol_l;//过压保护、欠压保护

extern  Cap_Protect_Para_Struct  g_cap_protect[4];
extern  Com_Para_Struct  g_com;//通讯参数
extern  Cap_Para_Struct  g_cap[4];//电容参数

extern  AutoManual_Para_Struct g_adjust_cap; //调试开关
extern  Cap_Num_Struct g_cap_num;//

extern   Control_Para_Struct g_control_para;//控制参数
extern   Stat_Para_Struct g_stat;//统计参数
extern Cap_Ratio_Para_Struct  g_cap_ratio[4];



/************************* 函数原型声明（所有读写函数均带返回值） *************************/
/**
 * @brief  读取校表数据（FM31256校表区）
 * @param  calib_channel：校表通道（1~4）
 * @param  read_data：读取数据缓冲区（输出，非NULL）
 * @param  data_len：读取数据长度（字节，建议HT7036_CALIB_LEN/HT7053_CALIB_LEN）
 * @param  saved_chksum3e：3E校验和（输出，可选NULL）
 * @param  saved_chksum5e：5E校验和（输出，可选NULL）
 * @return Para_Op_Result_E：操作结果（0=成功，非0=失败）
 * @note   自动校验校表数据完整性，失败返回对应错误码
 */
Para_Op_Result_E FM31256_Read_Calib(uint8_t calib_channel, uint8_t *read_data, uint16_t data_len,
                           uint32_t *saved_chksum3e, uint32_t *saved_chksum5e);
/**
 * @brief  写入FM31256校表数据（主备区双写+CRC8校验）
 * @param  calib_channel：校表通道（1~4）
 * @param  write_data：待写入校表数据（输入，非NULL）
 * @return Para_Op_Result_E：操作结果（0=成功，非0=失败）
 * @note   1. 主区写入数据+CRC8；2. 写入后立即校验，失败自动重试1次；
 */
Para_Op_Result_E FM31256_Write_Calib(uint8_t calib_channel, const uint8_t *write_data, uint16_t data_len);
/**
 * @brief  系统参数初始化（上电必调）
 * @return Para_Op_Result_E：初始化结果（0=成功，非0=失败/部分恢复）
 * @note   流程：1.校验主区CRC → 2.主区失效恢复备区 → 3.备区失效加载默认值
 */
Para_Op_Result_E Sys_Param_Init(void);

// -------------------------- 通讯参数读写接口 --------------------------
/**
 * @brief  读取通讯参数
 * @param  para：通讯参数缓冲区（输出，非NULL）
 * @return Para_Op_Result_E：操作结果（0=成功，非0=失败）
 * @note   自动CRC校验，主备区失效时加载默认值并返回对应错误码
 */
Para_Op_Result_E Get_Com_Para(Com_Para_Struct *para);

/**
 * @brief  设置并保存通讯参数
 * @param  new_para：新通讯参数（输入，非NULL）
 * @return Para_Op_Result_E：操作结果（0=成功，非0=失败）
 * @note   自动同步主备区、计算CRC、记录修改日志；写入失败返回对应错误码
 */
Para_Op_Result_E Set_Com_Para(const Com_Para_Struct *new_para);

// -------------------------- 过压保护参数读写接口 --------------------------
/**
 * @brief  读取过压保护参数
 * @param  para：过压保护参数缓冲区（输出，非NULL）
 * @return Para_Op_Result_E：操作结果（0=成功，非0=失败）
 * @note   自动CRC校验，主备区失效时加载默认值并返回对应错误码
 */
Para_Op_Result_E Get_OV_Protect_Para(Protect_Para_Struct *para);

/**
 * @brief  设置并保存过压保护参数
 * @param  new_para：新过压保护参数（输入，非NULL）
 * @return Para_Op_Result_E：操作结果（0=成功，非0=失败）
 * @note   自动同步主备区、计算CRC、记录修改日志；写入失败返回对应错误码
 */
Para_Op_Result_E Set_OV_Protect_Para(const Protect_Para_Struct *new_para);

// -------------------------- 欠压保护参数读写接口 --------------------------
/**
 * @brief  读取欠压保护参数
 * @param  para：欠压保护参数缓冲区（输出，非NULL）
 * @return Para_Op_Result_E：操作结果（0=成功，非0=失败）
 * @note   自动CRC校验，主备区失效时加载默认值并返回对应错误码
 */
Para_Op_Result_E Get_UV_Protect_Para(Protect_Para_Struct *para);

/**
 * @brief  设置并保存欠压保护参数
 * @param  new_para：新欠压保护参数（输入，非NULL）
 * @return Para_Op_Result_E：操作结果（0=成功，非0=失败）
 * @note   自动同步主备区、计算CRC、记录修改日志；写入失败返回对应错误码
 */
Para_Op_Result_E Set_UV_Protect_Para(const Protect_Para_Struct *new_para);

// -------------------------- 控制参数读写接口 --------------------------
/**
 * @brief  读取控制参数
 * @param  para：控制参数缓冲区（输出，非NULL）
 * @return Para_Op_Result_E：操作结果（0=成功，非0=失败）
 * @note   自动CRC校验，主备区失效时加载默认值并返回对应错误码
 */
Para_Op_Result_E Get_Control_Para(Control_Para_Struct *para);

/**
 * @brief  设置并保存控制参数
 * @param  new_para：新控制参数（输入，非NULL）
 * @return Para_Op_Result_E：操作结果（0=成功，非0=失败）
 * @note   自动同步主备区、计算CRC、记录修改日志；写入失败返回对应错误码
 */
Para_Op_Result_E Set_Control_Para(const Control_Para_Struct *new_para);

// -------------------------- 电容参数（多通道）读写接口 --------------------------
/**
 * @brief  读取指定通道的电容参数
 * @param  ch：通道号（0=1路，1=2路...，<CAP_CHANNEL_CNT）
 * @param  para：电容参数缓冲区（输出，非NULL）
 * @return Para_Op_Result_E：操作结果（0=成功，非0=失败）
 * @note   自动CRC校验，主备区失效时加载默认值并返回对应错误码
 */
Para_Op_Result_E Get_Cap_Para_Ch(uint8_t ch, Cap_Para_Struct *para);

/**
 * @brief  设置并保存指定通道的电容参数
 * @param  ch：通道号（0=1路，1=2路...，<CAP_CHANNEL_CNT）
 * @param  new_para：新电容参数（输入，非NULL）
 * @return Para_Op_Result_E：操作结果（0=成功，非0=失败）
 * @note   自动同步主备区、计算CRC、记录修改日志；写入失败返回对应错误码
 */
Para_Op_Result_E Set_Cap_Para_Ch(uint8_t ch, const Cap_Para_Struct *new_para);

// -------------------------- 变比参数读写接口 --------------------------
/**
 * @brief  读取变比参数
 * @param  para：变比参数缓冲区（输出，非NULL）
 * @return Para_Op_Result_E：操作结果（0=成功，非0=失败）
 * @note   自动CRC校验，主备区失效时加载默认值并返回对应错误码
 */
Para_Op_Result_E Get_Ratio_Para(Ratio_Para_Struct *para);

/**
 * @brief  设置并保存变比参数
 * @param  new_para：新变比参数（输入，非NULL）
 * @return Para_Op_Result_E：操作结果（0=成功，非0=失败）
 * @note   自动同步主备区、计算CRC、记录修改日志；写入失败返回对应错误码
 */
Para_Op_Result_E Set_Ratio_Para(const Ratio_Para_Struct *new_para);

// -------------------------- 统计参数读写接口 --------------------------
/**
 * @brief  读取统计参数
 * @param  para：统计参数缓冲区（输出，非NULL）
 * @return Para_Op_Result_E：操作结果（0=成功，非0=失败）
 * @note   自动CRC校验，主备区失效时加载默认值并返回对应错误码
 */
Para_Op_Result_E Get_Stat_Para(Stat_Para_Struct *para);

/**
 * @brief  设置并保存统计参数
 * @param  new_para：新统计参数（输入，非NULL）
 * @return Para_Op_Result_E：操作结果（0=成功，非0=失败）
 * @note   自动同步主备区、计算CRC、记录修改日志；写入失败返回对应错误码
 */
Para_Op_Result_E Set_Stat_Para(const Stat_Para_Struct *new_para);

// -------------------------- 口令参数读写接口 --------------------------
/**
 * @brief  读取口令参数
 * @param  para：口令参数缓冲区（输出，非NULL）
 * @return Para_Op_Result_E：操作结果（0=成功，非0=失败）
 * @note   自动CRC校验，主备区失效时加载默认值并返回对应错误码
 */
Para_Op_Result_E Get_Pass_Para(Pass_Para_Struct *para);

/**
 * @brief  设置并保存口令参数
 * @param  new_para：新口令参数（输入，非NULL）
 * @return Para_Op_Result_E：操作结果（0=成功，非0=失败）
 * @note   自动同步主备区、计算CRC、记录修改日志；写入失败返回对应错误码
 */
Para_Op_Result_E Set_Pass_Para(const Pass_Para_Struct *new_para);

// -------------------------- 手自动参数读写接口 --------------------------
/**
 * @brief  读取手自动控制参数
 * @param  para：手自动参数缓冲区（输出，非NULL）
 * @return Para_Op_Result_E：操作结果（0=成功，非0=失败）
 * @note   自动CRC校验，主备区失效时加载默认值并返回对应错误码
 */
Para_Op_Result_E Get_AutoManual_Para(AutoManual_Para_Struct *para);

/**
 * @brief  设置并保存手自动控制参数
 * @param  new_para：新手自动参数（输入，非NULL）
 * @return Para_Op_Result_E：操作结果（0=成功，非0=失败）
 * @note   自动同步主备区、计算CRC、记录修改日志；写入失败返回对应错误码
 */
Para_Op_Result_E Set_AutoManual_Para(const AutoManual_Para_Struct *new_para);

// -------------------------- 对比度参数读写接口 --------------------------
/**
 * @brief  读取屏幕对比度参数
 * @param  para：对比度参数缓冲区（输出，非NULL）
 * @return Para_Op_Result_E：操作结果（0=成功，非0=失败）
 * @note   自动CRC校验，主备区失效时加载默认值并返回对应错误码
 */
Para_Op_Result_E Get_Contra_Para(Contra_Para_Struct *para);

/**
 * @brief  设置并保存屏幕对比度参数
 * @param  new_para：新对比度参数（输入，非NULL）
 * @return Para_Op_Result_E：操作结果（0=成功，非0=失败）
 * @note   自动同步主备区、计算CRC、记录修改日志；写入失败返回对应错误码
 */
Para_Op_Result_E Set_Contra_Para(const Contra_Para_Struct *new_para);

// -------------------------- 报警参数读写接口 --------------------------
/**
 * @brief  读取报警参数
 * @param  para：报警参数缓冲区（输出，非NULL）
 * @return Para_Op_Result_E：操作结果（0=成功，非0=失败）
 * @note   自动CRC校验，主备区失效时加载默认值并返回对应错误码
 */
Para_Op_Result_E Get_Alarm_Para(Alarm_Para_Struct *para);

/**
 * @brief  设置并保存报警参数
 * @param  new_para：新报警参数（输入，非NULL）
 * @return Para_Op_Result_E：操作结果（0=成功，非0=失败）
 * @note   自动同步主备区、计算CRC、记录修改日志；写入失败返回对应错误码
 */
Para_Op_Result_E Set_Alarm_Para(const Alarm_Para_Struct *new_para);

// -------------------------- 电容保护参数（多通道）读写接口 --------------------------
/**
 * @brief  读取指定通道的电容保护参数
 * @param  ch：通道号（0=1路，1=2路...，<CAP_CHANNEL_CNT）
 * @param  para：电容保护参数缓冲区（输出，非NULL）
 * @return Para_Op_Result_E：操作结果（0=成功，非0=失败）
 * @note   自动CRC校验，主备区失效时加载默认值并返回对应错误码
 */
Para_Op_Result_E Get_Cap_Protect_Para_Ch(uint8_t ch, Cap_Protect_Para_Struct *para);

/**
 * @brief  设置并保存指定通道的电容保护参数
 * @param  ch：通道号（0=1路，1=2路...，<CAP_CHANNEL_CNT）
 * @param  new_para：新电容保护参数（输入，非NULL）
 * @return Para_Op_Result_E：操作结果（0=成功，非0=失败）
 * @note   自动同步主备区、计算CRC、记录修改日志；写入失败返回对应错误码
 */
Para_Op_Result_E Set_Cap_Protect_Para_Ch(uint8_t ch, const Cap_Protect_Para_Struct *new_para);

// -------------------------- 电容变比参数（多通道）读写接口 --------------------------
/**
 * @brief  读取指定通道的电容变比参数
 * @param  ch：通道号（0=1路，1=2路...，<CAP_CHANNEL_CNT）
 * @param  para：电容变比参数缓冲区（输出，非NULL）
 * @return Para_Op_Result_E：操作结果（0=成功，非0=失败）
 * @note   自动CRC校验，主备区失效时加载默认值并返回对应错误码
 */
Para_Op_Result_E Get_Cap_Ratio_Para_Ch(uint8_t ch, Cap_Ratio_Para_Struct *para);

/**
 * @brief  设置并保存指定通道的电容变比参数
 * @param  ch：通道号（0=1路，1=2路...，<CAP_CHANNEL_CNT）
 * @param  new_para：新电容变比参数（输入，非NULL）
 * @return Para_Op_Result_E：操作结果（0=成功，非0=失败）
 * @note   自动同步主备区、计算CRC、记录修改日志；写入失败返回对应错误码
 */
Para_Op_Result_E Set_Cap_Ratio_Para_Ch(uint8_t ch, const Cap_Ratio_Para_Struct *new_para);



/**
 * @brief  读取保护路数参数
 * @param  para：存储报保护路数的结构体指针（输出参数）
 * @return 无
 * @note   内部调用Read_Param_With_Backup，支持主备区切换和失效恢复（仅内存默认值）
 */
Para_Op_Result_E Get_Cap_Num(Cap_Num_Struct *para);


/**
 * @brief  设置保护路数
 * @param  new_para：新参数结构体指针（输入参数）
 * @return 无
 * @note   主程序按需调用，不主动触发则不覆盖已有数据
 */
Para_Op_Result_E Set_Cap_Num(const Cap_Num_Struct *new_para);



// -------------------------- 事件日志操作接口 --------------------------
/**
 * @brief  写入系统事件日志
 * @param  event_type：事件类型（Event_Type_E）
 * @param  event_data：事件关联数据（如过压值、通道号等）
 * @return Para_Op_Result_E：操作结果（0=成功，非0=失败）
 * @note   自动填充时间戳、循环覆盖旧日志、更新事件索引
 */
Para_Op_Result_E Write_Event_Log(Event_Type_E event_type, uint32_t event_data);

/**
 * @brief  读取最新的系统事件日志
 * @param  event_log：事件日志缓冲区（输出，非NULL）
 * @return Para_Op_Result_E：操作结果（0=成功，1=无日志，其他=失败）
 */
Para_Op_Result_E Read_Latest_Event_Log(Event_Log_Struct *event_log);

/**
 * @brief  读取指定索引的系统事件日志
 * @param  index：日志索引（0~EVENT_LOG_CNT-1）
 * @param  event_log：事件日志缓冲区（输出，非NULL）
 * @return Para_Op_Result_E：操作结果（0=成功，非0=失败）
 */
Para_Op_Result_E Read_Event_Log(uint8_t index, Event_Log_Struct *event_log);


/**
 * @brief  获取当前事件日志索引
 * @return uint8_t：当前事件索引（下一条日志写入位置，0~99）
 * @note   无失败场景，直接返回索引值
 */
uint8_t Get_Event_Index(void);








#endif  // __DATA_SAVE_H__