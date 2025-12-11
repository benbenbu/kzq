
#include "fm31256.h"






// 软件SPI时钟延时
void spi_delay() {
    _nop_(); _nop_(); _nop_(); _nop_(); // 延时几个机器周期
}

// 软件SPI发送一个字节
void spi_send_byte(uint8_t send_data) {
    uint8_t i;
    for(i = 0; i < 8; i++) {
        SPI_SCK = 0;
        if(send_data & 0x80) {
            SPI_MOSI = 1;
        } else {
            SPI_MOSI = 0;
        }
        send_data <<= 1;
        spi_delay();
        SPI_SCK = 1;
        spi_delay();
    }
    SPI_SCK = 0; // 保持时钟低电平
}

// 软件SPI接收一个字节
uint8_t spi_receive_byte() {
    uint8_t i;
    uint8_t send_data = 0;
    
    for(i = 0; i < 8; i++) {
        SPI_SCK = 0;
        spi_delay();
        SPI_SCK = 1;
        send_data <<= 1;
        if(SPI_MISO) {
            send_data |= 0x01;
        }
        spi_delay();
    }
    SPI_SCK = 0; // 保持时钟低电平
    return send_data;
}

// 软件SPI发送接收字节（全双工）
uint8_t spi_transfer_byte(uint8_t send_data) {
    uint8_t i;
    uint8_t receive = 0;
    
    for(i = 0; i < 8; i++) {
        SPI_SCK = 0;
        if(send_data & 0x80) {
            SPI_MOSI = 1;
        } else {
            SPI_MOSI = 0;
        }
        send_data <<= 1;
        spi_delay();
        SPI_SCK = 1;
        receive <<= 1;
        if(SPI_MISO) {
            receive |= 0x01;
        }
        spi_delay();
    }
    SPI_SCK = 0; // 保持时钟低电平
    return receive;
}


// CRC16校验计算
uint16_t calculate_crc16(const uint8_t *send_data, uint32_t length) {
    uint16_t crc = 0xFFFF;
    uint32_t i;
    uint8_t j;   
    for(i = 0; i < length; i++) {
        crc ^= send_data[i];

        for(j = 0; j < 8; j++) {
            if(crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001; // CRC16-IBM多项式
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

// BCD码转换为二进制
uint8_t bcd_to_bin(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

// 二进制转换为BCD码
uint8_t bin_to_bcd(uint8_t bin) {
    return ((bin / 10) << 4) + (bin % 10);
}

// 获取当前时间（从存储器读取）
bool get_current_time(time_struct *time) {
    // 这里假设时间存储在FRAM的最后几个字节，或通过RTC芯片获取
    // 为了简化，这里直接返回当前时间（实际应用中应从RTC获取）
    time->second = 0;
    time->minute = 0;
    time->hour = 0;
    time->day = 1;
    time->month = 1;
    time->year = 23; // 2023年
    return 1;
}

// 设置当前时间（保存到存储器）
void set_current_time(const time_struct *time) {
    // 实际应用中应保存到RTC或特定存储位置
}

// 更新时间（递增一秒）
void update_time() {
    // 实际应用中由定时器中断处理
}

// FM31256初始化
bool fm31256_init() {
	   uint8_t status=0;
    // 设置控制引脚初始状态
    FM31256_CS = 1;   // CS拉高
    FM31256_HOLD = 1; // HOLD拉高
    FM31256_WP = 0;   // WP拉低（允许写入）
    
    // 初始化SPI接口
    SPI_MOSI = 0;
    SPI_SCK = 0;
    
    // 短暂延时让设备稳定
    Delay_ms(10);
    
    // 检查设备是否响应
    status = fm31256_read_status();
    if(status != 0xFF) {
        return 1;
    } else {
        return 0;
    }
}

// 读取状态寄存器（带重试机制）
uint8_t fm31256_read_status_with_retry() {
    uint8_t status = 0;
    uint8_t retries = 0;
    
    while(retries < FM31256_RETRY_COUNT) {
        FM31256_CS = 0;           // CS拉低
        spi_send_byte(FM31256_CMD_RDSR); // 发送读状态寄存器命令
        status = spi_receive_byte();     // 读取状态值
        FM31256_CS = 1;           // CS拉高
        
        // 简单验证：如果读取到的状态值在合理范围内，认为读取成功
        if(status <= 0x03) { // WIP和WEL位组合不会超过0x03
            break;
        }
        
        retries++;
        Delay_us(10); // 短暂延时后重试
    }
    
    return status;
}

// 读取状态寄存器
uint8_t fm31256_read_status() {
    return fm31256_read_status_with_retry();
}

// 写入状态寄存器（带验证）
void fm31256_write_status_verified(uint8_t status) {
    uint8_t retries = 0;
    bool success = 0;
    uint8_t readback=0;   
    while(retries < FM31256_RETRY_COUNT && !success) {
        fm31256_write_enable();           // 使能写操作
        
        FM31256_CS = 0;           // CS拉低
        spi_send_byte(FM31256_CMD_WRSR); // 发送写状态寄存器命令
        spi_send_byte(status);           // 写入状态值
        FM31256_CS = 1;           // CS拉高
        
        fm31256_wait_write_complete();    // 等待写操作完成
        
        // 验证写入是否成功
        readback = fm31256_read_status();
        if(readback == status) {
            success = 1;
        } else {
            retries++;
            Delay_us(100); // 等待后重试
        }
    }
}

// 使能写操作（带验证）
void fm31256_write_enable_verified() {
    uint8_t retries = 0;
    bool success = 0;
    uint8_t status=0;    
    while(retries < FM31256_RETRY_COUNT && !success) {
        FM31256_CS = 0;           // CS拉低
        spi_send_byte(FM31256_CMD_WREN); // 发送写使能命令
        FM31256_CS = 1;           // CS拉高
        
        // 验证WEL位是否被设置
        status = fm31256_read_status();
        if(status & FM31256_STATUS_WEL) {
            success = 1;
        } else {
            retries++;
            Delay_us(100); // 等待后重试
        }
    }
}

// 使能写操作
void fm31256_write_enable() {
    fm31256_write_enable_verified();
}

// 禁止写操作（带验证）
void fm31256_write_disable_verified() {
    uint8_t retries = 0;
    bool success = false;
    uint8_t status=0;   
    while(retries < FM31256_RETRY_COUNT && !success) {
        FM31256_CS = 0;           // CS拉低
        spi_send_byte(FM31256_CMD_WRDI); // 发送写禁止命令
        FM31256_CS = 1;           // CS拉高
        
        // 验证WEL位是否被清除
        status = fm31256_read_status();
        if(!(status & FM31256_STATUS_WEL)) {
            success = 1;
        } else {
            retries++;
            Delay_us(100); // 等待后重试
        }
    }
}

// 禁止写操作
void fm31256_write_disable() {
    fm31256_write_disable_verified();
}

// 等待写操作完成（带超时）
void fm31256_wait_write_complete() {
    uint16_t timeout = 10000; // 10ms超时
    while((fm31256_read_status() & FM31256_STATUS_WIP) && timeout > 0) {
        Delay_us(1);
        timeout--;
    }
}

// 读取数据（带重试和校验）
bool fm31256_read_data_with_retry(uint32_t address, uint8_t *buffer, uint32_t length) {
	
    uint8_t retries = 0;
    bool success = 0;	
    uint32_t i;
    bool match = 1;	
    if(address + length > FM31256_CAPACITY) {
        return 0;
    }
    

    
    while(retries < FM31256_RETRY_COUNT && !success) {
        FM31256_CS = 0;           // CS拉低
        
        spi_send_byte(FM31256_CMD_READ);        // 发送读命令
        spi_send_byte((address >> 16) & 0xFF);  // 发送地址高位
        spi_send_byte((address >> 8) & 0xFF);   // 发送地址中位
        spi_send_byte(address & 0xFF);          // 发送地址低位
        

        for(i = 0; i < length; i++) {
            buffer[i] = spi_receive_byte();     // 读取数据
        }
        
        FM31256_CS = 1;           // CS拉高
        
        // 进行二次读取验证（读取前几个字节）
        if(length > 0) {
            uint8_t verify_buffer[4];
            uint32_t verify_len = (length < 4) ? length : 4;
            
            FM31256_CS = 0;           // CS拉低
            spi_send_byte(FM31256_CMD_READ);        // 发送读命令
            spi_send_byte((address >> 16) & 0xFF);  // 发送地址高位
            spi_send_byte((address >> 8) & 0xFF);   // 发送地址中位
            spi_send_byte(address & 0xFF);          // 发送地址低位
            
            for(i = 0; i < verify_len; i++) {
                verify_buffer[i] = spi_receive_byte(); // 读取验证数据
            }
            
            FM31256_CS = 1;           // CS拉高
            
            // 比较原始读取和验证读取的数据
;
            for(i = 0; i < verify_len; i++) {
                if(buffer[i] != verify_buffer[i]) {
                    match = 0;
                    break;
                }
            }
            
            if(match) {
                success = 1;
            } else {
                retries++;
                Delay_us(100); // 短暂延时后重试
            }
        } else {
            success = 1;
        }
    }
    
    if(!success) {
        return 0;
    }
    
    return 1;
}

// 写入数据（带重试、校验和验证）
bool fm31256_write_data_verified(uint32_t address, const uint8_t *buffer, uint32_t length) {
	
    uint8_t retries = 0;
    bool success = 0;	
    uint8_t verify_buffer[16];  // 根据需要调整大小	
    uint32_t verify_len ;	
    bool read_success;	
    if(address + length > FM31256_CAPACITY) {
        return 0;
    }
    

    
    while(retries < FM31256_RETRY_COUNT && !success) {
        // 计算原始数据校验
        uint16_t original_crc = calculate_crc16(buffer, length);
        
        uint32_t i;
        for(i = 0; i < length; i++) {
            fm31256_write_enable();           // 使能写操作
            
            FM31256_CS = 0;           // CS拉低
            
            spi_send_byte(FM31256_CMD_WRITE);       // 发送写命令
            spi_send_byte(((address + i) >> 16) & 0xFF); // 发送地址高位
            spi_send_byte(((address + i) >> 8) & 0xFF);  // 发送地址中位
            spi_send_byte((address + i) & 0xFF);    // 发送地址低位
            spi_send_byte(buffer[i]);               // 写入数据
            
            FM31256_CS = 1;           // CS拉高
            
            fm31256_wait_write_complete();          // 等待写操作完成
        }
        
        // 使用固定大小的验证缓冲区，避免使用malloc

         verify_len = (length < 16) ? length : 16;
        
        read_success = fm31256_read_data_with_retry(address, verify_buffer, verify_len);
        if(read_success) {
            bool match = true;
            for(i = 0; i < verify_len; i++) {
                if(buffer[i] != verify_buffer[i]) {
                    match = false;
                    break;
                }
            }
            
            if(match) {
                // 额外的校验：再次读取前几个字节确认
                uint8_t extra_verify[4];
                uint32_t extra_len = (length < 4) ? length : 4;
                bool extra_success = fm31256_read_data_with_retry(address, extra_verify, extra_len);
                
                if(extra_success) {
                    bool extra_match = true;
                    for(i = 0; i < extra_len; i++) {
                        if(buffer[i] != extra_verify[i]) {
                            extra_match = false;
                            break;
                        }
                    }
                    
                    if(extra_match) {
                        success = true;
                    }
                }
            }
        }
        
        retries++;
        
        if(!success) {
            Delay_us(100); // 短暂延时后重试
        }
    }
    
    if(!success) {
        return false;
    }
    
    return true;
}

// 存储器健康检查
bool fm31256_health_check() {
	
    uint8_t test_data = 0x55;
    uint8_t read_data = 0;
    uint32_t test_addr = 0x00;
    bool write_result=0;
    bool read_result=0XFF;		
    // 读取状态寄存器
    uint8_t status = fm31256_read_status_with_retry();
    
    // 检查WIP位
    if(status & FM31256_STATUS_WIP) {
        // 写操作进行中，等待完成
        fm31256_wait_write_complete();
    }
    
    // 简单读写测试

    
    write_result = fm31256_write_data_verified(test_addr, &test_data, 1);
    if(!write_result) {
        return false;
    }
    
    read_result = fm31256_read_data_with_retry(test_addr, &read_data, 1);
    if(!read_result) {
        return false;
    }
    
    if(test_data == read_data) {
        return true; // 健康检查通过
    } else {
        return false; // 健康检查失败
    }
}

// 验证数据块完整性
bool verify_data_integrity(const uint8_t *send_data, uint8_t data_size) {
	
	 uint16_t calc_checksum;
	 uint16_t stored_checksum;
	 uint16_t magic;
    if(data_size < 4) return false; // 数据太小无法验证
    
    // 检查魔数头
     magic = ((uint16_t)send_data[1] << 8) | send_data[0];
    if(magic != FM31256_MAGIC_HEADER) {
        return false;
    }
    
    // 计算校验和并验证
     calc_checksum = calculate_crc16(send_data, data_size - 2); // 不包含校验和本身
     stored_checksum = ((uint16_t)send_data[data_size-1] << 8) | send_data[data_size-2];
    
    return calc_checksum == stored_checksum;
}

// 读取系统参数
bool read_system_param(system_param_t *param) {
    uint8_t buffer[SYS_PARAM_SIZE];	
    if(param == NULL) return false;
    

    if(!fm31256_read_data_with_retry(SYS_PARAM_ADDR, buffer, SYS_PARAM_SIZE)) {
        return false;
    }
    
    // 验证数据完整性
    if(!verify_data_integrity(buffer, SYS_PARAM_SIZE)) {
        return false;
    }
    
    // 复制数据到结构体
    memcpy(param, buffer, sizeof(system_param_t));
    return true;
}

// 写入系统参数
bool write_system_param(const system_param_t *param) {
	  system_param_t temp_param;
    if(param == NULL) return false;
    
    // 计算校验和
     temp_param = *param;
    temp_param.checksum = calculate_crc16((uint8_t*)&temp_param, 
                                         sizeof(system_param_t) - sizeof(uint16_t));
    
    return fm31256_write_data_verified(SYS_PARAM_ADDR, 
                                      (uint8_t*)&temp_param, 
                                      sizeof(system_param_t));
}

// 读取校表参数
bool read_calibration_param(calibration_param_t *param) {
    uint8_t buffer[CAL_PARAM_SIZE];	
    if(param == NULL) return false;
    

    if(!fm31256_read_data_with_retry(CAL_PARAM_ADDR, buffer, CAL_PARAM_SIZE)) {
        return false;
    }
    
    // 验证数据完整性
    if(!verify_data_integrity(buffer, CAL_PARAM_SIZE)) {
        return false;
    }
    
    // 复制数据到结构体
    memcpy(param, buffer, sizeof(calibration_param_t));
    return true;
}

// 写入校表参数（仅在出厂时调用）
bool write_calibration_param(const calibration_param_t *param) {
	calibration_param_t temp_param ;
    if(param == NULL) return false;
    
    // 计算校验和
     temp_param = *param;
    temp_param.checksum = calculate_crc16((uint8_t*)&temp_param, 
                                         sizeof(calibration_param_t) - sizeof(uint16_t));
    
    return fm31256_write_data_verified(CAL_PARAM_ADDR, 
                                      (uint8_t*)&temp_param, 
                                      sizeof(calibration_param_t));
}

// 添加事件记录
bool add_event_record(const event_record_t *event) {
	    uint16_t record_idx = 0;
    bool found_empty = false;
    uint32_t addr;
        event_record_t temp_event;	
	event_record_t write_event;
    if(event == NULL) return false;
    
    // 查找第一个无效的记录位置

    
    for(record_idx = 0; record_idx < MAX_EVENT_RECORDS; record_idx++) {

        addr = EVENT_RECORD_ADDR + (record_idx * EVENT_RECORD_SIZE);
        
        if(!fm31256_read_data_with_retry(addr, (uint8_t*)&temp_event, EVENT_RECORD_SIZE)) {
            continue; // 读取失败，继续查找
        }
        
        // 检查记录是否有效
        if(temp_event.valid_flag != 0xFF) {
            found_empty = true;
            break;
        }
    }
    
    if(!found_empty) {
        // 如果所有记录都已满，覆盖最旧的记录（循环使用）
        record_idx = 0;
    }
    
    // 准备要写入的事件记录
     write_event = *event;
    write_event.magic = FM31256_MAGIC_HEADER;
    write_event.checksum = calculate_crc16((uint8_t*)&write_event, 
                                         sizeof(event_record_t) - 3); // 不包含校验和和有效标志
    write_event.valid_flag = 0xFF; // 标记为有效
    
    addr = EVENT_RECORD_ADDR + (record_idx * EVENT_RECORD_SIZE);
    return fm31256_write_data_verified(addr, (uint8_t*)&write_event, EVENT_RECORD_SIZE);
}

// 读取事件记录
bool read_event_record(uint16_t index, event_record_t *event) {
    uint32_t addr;	
    if(event == NULL || index >= MAX_EVENT_RECORDS) return false;
    
    addr = EVENT_RECORD_ADDR + (index * EVENT_RECORD_SIZE);
    if(!fm31256_read_data_with_retry(addr, (uint8_t*)event, EVENT_RECORD_SIZE)) {
        return false;
    }
    
    // 验证数据完整性
    if(!verify_data_integrity((uint8_t*)event, EVENT_RECORD_SIZE)) {
        return false;
    }
    
    return true;
}

// 获取有效事件记录数量
uint16_t get_event_record_count() {
    uint16_t count = 0;
    uint16_t i;
    
    for(i = 0; i < MAX_EVENT_RECORDS; i++) {
        event_record_t temp_event;
        uint32_t addr = EVENT_RECORD_ADDR + (i * EVENT_RECORD_SIZE);
        
        if(fm31256_read_data_with_retry(addr, (uint8_t*)&temp_event, EVENT_RECORD_SIZE)) {
            if(temp_event.valid_flag == 0xFF && 
               verify_data_integrity((uint8_t*)&temp_event, EVENT_RECORD_SIZE)) {
                count++;
            }
        }
    }
    
    return count;
}

// 初始化时间（如果时间无效则设置为默认时间）
void init_time() {
    time_struct current_time;
    
    // 尝试读取当前时间
    if(get_current_time(&current_time)) {
        // 检查时间是否有效
        if(current_time.year > 50) { // 假设有效年份在2000-2050之间
            return; // 时间有效，直接返回
        }
    }
    
    // 设置默认时间（2023年1月1日 00:00:00）
    current_time.second = 0;
    current_time.minute = 0;
    current_time.hour = 0;
    current_time.day = 1;
    current_time.month = 1;
    current_time.year = 23; // 2023年
    
    set_current_time(&current_time);
}

// 初始化存储器布局
void init_storage_layout() {
    // 初始化系统参数区域（如果需要）
    system_param_t sys_param;
    calibration_param_t cal_param;	
    if(!read_system_param(&sys_param)) {
        // 如果读取失败，初始化默认参数
        memset(&sys_param, 0, sizeof(sys_param));
        sys_param.param_version = 1;
        sys_param.device_id = 0x1234;
        sys_param.calibration_date = 0x230101; // 2023年1月1日
        sys_param.temperature_offset = 0.0f;
        sys_param.voltage_ref = 3.3f;
        sys_param.alarm_threshold = 80;
        write_system_param(&sys_param);
    }
    
    // 初始化校表参数区域（如果需要）

    if(!read_calibration_param(&cal_param)) {
        // 如果读取失败，初始化默认校表参数
        memset(&cal_param, 0, sizeof(cal_param));
        cal_param.cal_version = 1;
        cal_param.gain_factor = 1.0f;
        cal_param.offset_value = 0.0f;
        cal_param.cal_date = 0x230101; // 2023年1月1日
        write_calibration_param(&cal_param);
    }
}

// 系统参数存储测试函数
void sys_param_test() {
	
	    system_param_t test_param;
	    calibration_param_t cal_param;
	    event_record_t event;
    // 初始化存储器
    if(!fm31256_init()) {
        return;
    }
    
    // 初始化时间
    init_time();
    
    // 执行健康检查
    if(!fm31256_health_check()) {
        return;
    }
    
    // 测试系统参数读写

    test_param.param_version = 1;
    test_param.device_id = 0x55AA;
    test_param.calibration_date = 0x230515; // 2023年5月15日
    test_param.temperature_offset = 2.5f;
    test_param.voltage_ref = 3.3f;
    test_param.alarm_threshold = 85;
    
    if(write_system_param(&test_param)) {
        system_param_t read_param;
        if(read_system_param(&read_param)) {
            // 参数读写测试成功
        }
    }
    
    // 测试校表参数读写

    cal_param.cal_version = 1;
    cal_param.gain_factor = 1.05f;
    cal_param.offset_value = 0.1f;
    cal_param.cal_date = 0x230515;
    
    if(write_calibration_param(&cal_param)) {
        calibration_param_t read_cal;
        if(read_calibration_param(&read_cal)) {
            // 校表参数读写测试成功
        }
    }
    
    // 测试事件记录

    event.event_type = 1; // 故障事件
    event.event_code = 0x01;
    get_current_time(&event.timestamp);
    
    if(add_event_record(&event)) {
        // 事件记录添加成功
    }
}

// 定时器0中断服务程序（用于时间更新）
//void timer0_isr() interrupt 1 {
//    static uint16_t tick_count = 0;
//    
//    // 重新加载定时器值（假设12MHz晶振，12T模式，定时50ms）
//    TH0 = 0x3C;
//    TL0 = 0xB0;
//    
//    tick_count++;
//    
//    // 每秒更新一次时间（20个50ms = 1秒）
//    if(tick_count >= 20) {
//        tick_count = 0;
//        update_time();
//    }
//}

