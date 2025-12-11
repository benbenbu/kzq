#ifndef _FM31256_H
#define _FM31256_H

#include "config.h"


// 硬件接口定义 - 根据实际硬件连接修改
sbit FM31256_CS    = P2^0;    // 片选信号
sbit FM31256_HOLD  = P2^1;    // 保持信号
sbit FM31256_WP    = P2^2;    // 写保护信号

// SPI接口定义
sbit SPI_MOSI      = P1^0;    // 主机输出，从机输入
sbit SPI_MISO      = P1^1;    // 主机输入，从机输出
sbit SPI_SCK       = P1^2;    // 时钟信号

// FM31256命令定义
#define FM31256_CMD_READ        0x03    // 读操作命令
#define FM31256_CMD_WRITE       0x02    // 写操作命令
#define FM31256_CMD_RDSR        0x05    // 读状态寄存器
#define FM31256_CMD_WRSR        0x01    // 写状态寄存器
#define FM31256_CMD_WREN        0x06    // 写使能
#define FM31256_CMD_WRDI        0x04    // 写禁止

// 状态寄存器位定义
#define FM31256_STATUS_WIP      0x01    // 写操作进行中
#define FM31256_STATUS_WEL      0x02    // 写使能锁存

// 存储器容量定义
#define FM31256_CAPACITY        (32UL * 1024)    // 32KB

// 抗干扰相关常量
#define FM31256_RETRY_COUNT     3       // 重试次数
#define FM31256_CRC_POLY        0xED    // CRC多项式
#define FM31256_MAGIC_HEADER    0x5A5A  // 数据块魔数头

// 时间结构体定义
typedef struct {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint8_t year;  // 相对于2000年的年份
} time_struct;

// 事件记录结构体（12字节）
typedef struct {
    uint16_t magic;      // 魔数头，用于验证数据完整性
    uint8_t event_type;  // 事件类型：0-正常事件，1-故障事件
    uint8_t event_code;  // 事件代码
    time_struct timestamp; // 事件时间戳
    uint16_t checksum;   // 数据校验和
    uint8_t reserved;    // 预留字节
    uint8_t valid_flag;  // 有效标志：0xFF-有效，0x00-无效
} event_record_t;

// 系统参数结构体
typedef struct {
    uint16_t param_version;    // 参数版本号
    uint16_t device_id;        // 设备ID
    uint16_t calibration_date; // 校准日期（年月日压缩格式）
    float temperature_offset;  // 温度补偿值
    float voltage_ref;         // 参考电压
    uint8_t alarm_threshold;   // 报警阈值
    uint8_t reserved[7];       // 预留空间
    uint16_t checksum;         // 参数校验和
} system_param_t;

// 校表参数结构体
typedef struct {
    uint16_t cal_version;      // 校表版本号
    float gain_factor;         // 增益系数
    float offset_value;        // 偏移值
    uint16_t cal_date;         // 校准日期
    uint16_t checksum;         // 校表参数校验和
    uint8_t reserved[8];       // 预留空间
} calibration_param_t;

// 存储区域分配（从存储器开始分配）
#define SYS_PARAM_ADDR          0x000000UL  // 系统参数区域：0x000000-0x00001F (32字节)
#define CAL_PARAM_ADDR          0x000020UL  // 校表参数区域：0x000020-0x00003F (32字节)
#define EVENT_RECORD_ADDR       0x000040UL  // 事件记录区域：0x000040-0x0004FF (1200字节，100条记录）

// 计算各区域大小
#define SYS_PARAM_SIZE          sizeof(system_param_t)
#define CAL_PARAM_SIZE          sizeof(calibration_param_t)
#define EVENT_RECORD_SIZE       sizeof(event_record_t)
#define MAX_EVENT_RECORDS       100         // 最大事件记录数量

// 函数声明

/**
 * @brief 软件SPI时钟延时
 */
void spi_delay(void);

/**
 * @brief 软件SPI发送一个字节
 * @param data 要发送的数据
 */
void spi_send_byte(uint8_t send_data);

/**
 * @brief 软件SPI接收一个字节
 * @return 接收到的数据
 */
uint8_t spi_receive_byte(void);

/**
 * @brief 软件SPI发送接收字节（全双工）
 * @param data 要发送的数据
 * @return 接收到的数据
 */
uint8_t spi_transfer_byte(uint8_t send_data);


/**
 * @brief CRC16校验计算
 * @param data 数据指针
 * @param length 数据长度
 * @return CRC16校验值
 */
uint16_t calculate_crc16(const uint8_t *send_data, uint32_t length);

/**
 * @brief BCD码转换为二进制
 * @param bcd BCD码
 * @return 二进制值
 */
uint8_t bcd_to_bin(uint8_t bcd);

/**
 * @brief 二进制转换为BCD码
 * @param bin 二进制值
 * @return BCD码
 */
uint8_t bin_to_bcd(uint8_t bin);

/**
 * @brief 获取当前时间（从存储器读取）
 * @param time 时间结构体指针
 * @return 操作结果，true成功，false失败
 */
bool get_current_time(time_struct *time);

/**
 * @brief 设置当前时间（保存到存储器）
 * @param time 时间结构体指针
 */
void set_current_time(const time_struct *time);

/**
 * @brief 更新时间（递增一秒）
 */
void update_time(void);

/**
 * @brief FM31256初始化
 * @return 初始化结果，true成功，false失败
 */
bool fm31256_init(void);

/**
 * @brief 读取状态寄存器（带重试机制）
 * @return 状态寄存器值
 */
uint8_t fm31256_read_status_with_retry(void);

/**
 * @brief 读取状态寄存器
 * @return 状态寄存器值
 */
uint8_t fm31256_read_status(void);

/**
 * @brief 写入状态寄存器（带验证）
 * @param status 要写入的状态值
 */
void fm31256_write_status_verified(uint8_t status);

/**
 * @brief 使能写操作（带验证）
 */
void fm31256_write_enable_verified(void);

/**
 * @brief 使能写操作
 */
void fm31256_write_enable(void);

/**
 * @brief 禁止写操作（带验证）
 */
void fm31256_write_disable_verified(void);

/**
 * @brief 禁止写操作
 */
void fm31256_write_disable(void);

/**
 * @brief 等待写操作完成（带超时）
 */
void fm31256_wait_write_complete(void);

/**
 * @brief 读取数据（带重试和校验）
 * @param address 读取地址
 * @param buffer 数据缓冲区
 * @param length 读取长度
 * @return 操作结果，true成功，false失败
 */
bool fm31256_read_data_with_retry(uint32_t address, uint8_t *buffer, uint32_t length);

/**
 * @brief 写入数据（带重试、校验和验证）
 * @param address 写入地址
 * @param buffer 数据缓冲区
 * @param length 写入长度
 * @return 操作结果，true成功，false失败
 */
bool fm31256_write_data_verified(uint32_t address, const uint8_t *buffer, uint32_t length);

/**
 * @brief 存储器健康检查
 * @return 检查结果，true健康，false异常
 */
bool fm31256_health_check(void);

/**
 * @brief 验证数据块完整性
 * @param data 数据指针
 * @param size 数据大小
 * @return 验证结果
 */
bool verify_data_integrity(const uint8_t *send_data, uint8_t data_size);

/**
 * @brief 读取系统参数
 * @param param 系统参数结构体指针
 * @return 操作结果
 */
bool read_system_param(system_param_t *param);

/**
 * @brief 写入系统参数
 * @param param 系统参数结构体指针
 * @return 操作结果
 */
bool write_system_param(const system_param_t *param);

/**
 * @brief 读取校表参数
 * @param param 校表参数结构体指针
 * @return 操作结果
 */
bool read_calibration_param(calibration_param_t *param);

/**
 * @brief 写入校表参数（仅在出厂时调用）
 * @param param 校表参数结构体指针
 * @return 操作结果
 */
bool write_calibration_param(const calibration_param_t *param);

/**
 * @brief 添加事件记录
 * @param event 事件记录结构体指针
 * @return 操作结果
 */
bool add_event_record(const event_record_t *event);

/**
 * @brief 读取事件记录
 * @param index 事件记录索引
 * @param event 事件记录结构体指针
 * @return 操作结果
 */
bool read_event_record(uint16_t index, event_record_t *event);

/**
 * @brief 获取有效事件记录数量
 * @return 有效事件记录数量
 */
uint16_t get_event_record_count(void);

/**
 * @brief 初始化时间（如果时间无效则设置为默认时间）
 */
void init_time(void);

/**
 * @brief 初始化存储器布局
 */
void init_storage_layout(void);

/**
 * @brief 系统参数存储测试函数
 */
void sys_param_test(void);

/**
 * @brief 初始化定时器0（用于时间更新）
 */
void init_timer0(void);

#endif // _FM31256_SYS_PARAM_H



