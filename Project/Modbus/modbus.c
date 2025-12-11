

#include "modbus.h"



// Modbus相关定义
uint8_t modbus_slave_addr = 0x01;    // 从机地址（可修改）


// 波特率变量（可动态修改）
uint32_t uart_baudrate = 9600;

// Modbus接收缓冲区
uint8_t modbus_rx_buffer[MODBUS_RX_BUF_SIZE];
volatile uint8_t modbus_rx_index = 0;
volatile uint8_t modbus_rx_complete = 0;
volatile uint16_t modbus_timeout_counter = 0;
volatile uint8_t modbus_timer_active = 0;

// Modbus寄存器数组（可修改）
uint16_t modbus_registers[100] = {0}; // 100个保持寄存器





// CRC16计算
uint16_t Modbus_CRC16(uint8_t *pucBuff, uint8_t unNum) {
    uint16_t CRC_Value = 0;
    uint8_t  i         = 0;
    uint8_t  j         = 0;

    CRC_Value = 0xffff;
    for(i=0;i<unNum;i++)  //
    {
        CRC_Value ^= *(pucBuff+i);
        for(j=0;j<8;j++)
        {
            if(CRC_Value & 0x00001)
                CRC_Value = (CRC_Value >> 1) ^ 0xA001;
            else
                CRC_Value = (CRC_Value >> 1);
        }
    }
    CRC_Value = ((CRC_Value >> 8) +  (CRC_Value << 8)); //交换高低字节

    return CRC_Value;
}

// Modbus接收超时检查
void Modbus_CheckTimeout() {
    if(modbus_timer_active && modbus_timeout_counter >= MODBUS_TIMEOUT_MS) {
        // 超时，处理当前接收的数据包
        if(modbus_rx_index > 0) {
            modbus_rx_complete = 1; // 标记接收完成
        }
        
        // 重置超时计数器
        modbus_timeout_counter = 0;
        modbus_timer_active = 0;
    }
}

// 启动Modbus接收超时计时器
void Modbus_StartTimeout() {
    modbus_timeout_counter = 0;
    modbus_timer_active = 1;
}

// 停止Modbus接收超时计时器
void Modbus_StopTimeout() {
    modbus_timer_active = 0;
    modbus_timeout_counter = 0;
}



//// 定时器2中断服务程序（用于Modbus超时计数）
//void Timer2_ISR() interrupt 5 {
//    TF2H = 0; // 清除定时器2溢出标志
//    
//    // 更新Modbus超时计数器
//    if(modbus_timer_active) {
//        modbus_timeout_counter++;
//        
//        // 检查是否超时
//        if(modbus_timeout_counter >= MODBUS_TIMEOUT_MS) {
//            // 超时，标记接收完成
//            if(modbus_rx_index > 0) {
//                modbus_rx_complete = 1;
//            }
//            
//            // 重置计时器
//            modbus_timer_active = 0;
//        }
//    }
//}

// 处理Modbus请求
void Modbus_ProcessRequest() {
    if(modbus_rx_complete && modbus_rx_index >= 4) { // 最小Modbus帧长度
        uint8_t slave_addr = modbus_rx_buffer[0];
        
        // 检查从机地址
        if(slave_addr == modbus_slave_addr) {
            uint8_t function_code = modbus_rx_buffer[1];
            uint16_t crc_received = ((uint16_t)modbus_rx_buffer[modbus_rx_index-1] << 8) | 
                                   modbus_rx_buffer[modbus_rx_index-2];
            
            // 验证CRC
            uint16_t crc_calculated = Modbus_CRC16(modbus_rx_buffer, modbus_rx_index-2);
            
            if(crc_received == crc_calculated) {
                // CRC正确，处理功能码
                switch(function_code) {
                    case 0x02: // 读离散输入
                        Modbus_HandleReadDiscreteInputs();
                        break;
                    case 0x03: // 读保持寄存器
                        Modbus_HandleReadHoldingRegisters();
                        break;
                    case 0x04: // 读输入寄存器
                        Modbus_HandleReadInputRegisters();
                        break;
                    default:
                        // 不支持的功能码，发送错误响应
                        Modbus_SendError(function_code | 0x80, 0x01); // 非法功能
                        break;
                }
            } else {
                // CRC错误，发送错误响应
                Modbus_SendError(function_code | 0x80, 0x02); // CRC错误
            }
        }
        
        // 重置接收状态
        modbus_rx_index = 0;
        modbus_rx_complete = 0;
        Modbus_StopTimeout();
    }
}

// 处理读离散输入请求
void Modbus_HandleReadDiscreteInputs() {
	  uint16_t i;
		uint16_t start_addr	;
	  uint16_t num_inputs;
    uint8_t response[255];
    uint8_t byte_count;	
	  uint16_t crc;
     start_addr = ((uint16_t)modbus_rx_buffer[2] << 8) | modbus_rx_buffer[3];
     num_inputs = ((uint16_t)modbus_rx_buffer[4] << 8) | modbus_rx_buffer[5];
    
    if(num_inputs > 0 && num_inputs <= 2000) { // Modbus限制
        // 计算需要的字节数
         byte_count = (num_inputs + 7) / 8;
        
        // 构造响应

        response[0] = modbus_slave_addr;
        response[1] = 0x02; // 功能码
        response[2] = byte_count; // 字节数
        
        // 填充离散输入数据（示例：全部为0）

        for(i = 0; i < byte_count; i++) {
            response[3 + i] = 0x00; // 示例数据，实际应根据输入状态设置
        }
        
        // 计算CRC
        crc = Modbus_CRC16(response, 3 + byte_count);
        response[3 + byte_count] = crc & 0xFF;
        response[4 + byte_count] = (crc >> 8) & 0xFF;
        
        // 发送响应
        UART0_SendArray(response, 5 + byte_count);
    } else {
        // 发送错误响应
        Modbus_SendError(0x82, 0x03); // 非法数据值
    }
}

// 处理读保持寄存器请求
void Modbus_HandleReadHoldingRegisters() {
	  uint8_t i;
	  uint16_t start_addr;
	  uint16_t num_regs;
	  uint16_t crc;
     start_addr = ((uint16_t)modbus_rx_buffer[2] << 8) | modbus_rx_buffer[3];
     num_regs = ((uint16_t)modbus_rx_buffer[4] << 8) | modbus_rx_buffer[5];
    
    if(num_regs > 0 && num_regs <= 125 && start_addr + num_regs <= 100) { // 检查边界
        // 构造响应
        uint8_t response[256];
        response[0] = modbus_slave_addr;
        response[1] = 0x03; // 功能码
        response[2] = num_regs * 2; // 字节数
        
        // 填充寄存器数据

        for(i = 0; i < num_regs; i++) {
            response[3 + i*2] = (modbus_registers[start_addr + i] >> 8) & 0xFF;
            response[4 + i*2] = modbus_registers[start_addr + i] & 0xFF;
        }
        
        // 计算CRC
         crc = Modbus_CRC16(response, 3 + num_regs*2);
        response[3 + num_regs*2] = crc & 0xFF;
        response[4 + num_regs*2] = (crc >> 8) & 0xFF;
        
        // 发送响应
        UART0_SendArray(response, 5 + num_regs*2);
    } else {
        // 发送错误响应
        Modbus_SendError(0x83, 0x02); // 非法数据地址
    }
}

// 处理读输入寄存器请求
void Modbus_HandleReadInputRegisters() {
	   uint16_t crc;
	  uint16_t start_addr;
	uint16_t num_regs;
	         uint8_t i;
	          uint8_t response[256];
     start_addr = ((uint16_t)modbus_rx_buffer[2] << 8) | modbus_rx_buffer[3];
     num_regs = ((uint16_t)modbus_rx_buffer[4] << 8) | modbus_rx_buffer[5];
    
    if(num_regs > 0 && num_regs <= 125 && start_addr + num_regs <= 100) { // 检查边界
        // 构造响应

        response[0] = modbus_slave_addr;
        response[1] = 0x04; // 功能码
        response[2] = num_regs * 2; // 字节数
        
        // 填充寄存器数据（示例：返回与保持寄存器相同的值）

        for(i = 0; i < num_regs; i++) {
            response[3 + i*2] = (modbus_registers[start_addr + i] >> 8) & 0xFF;
            response[4 + i*2] = modbus_registers[start_addr + i] & 0xFF;
        }
        
        // 计算CRC
         crc = Modbus_CRC16(response, 3 + num_regs*2);
        response[3 + num_regs*2] = crc & 0xFF;
        response[4 + num_regs*2] = (crc >> 8) & 0xFF;
        
        // 发送响应
        UART0_SendArray(response, 5 + num_regs*2);
    } else {
        // 发送错误响应
        Modbus_SendError(0x84, 0x02); // 非法数据地址
    }
}

// 发送Modbus错误响应
void Modbus_SendError(uint8_t function_code, uint8_t exception_code) {
	
    uint8_t error_response[5];
	   uint16_t crc;
    error_response[0] = modbus_slave_addr;
    error_response[1] = function_code;
    error_response[2] = exception_code;
    
     crc = Modbus_CRC16(error_response, 3);
    error_response[3] = crc & 0xFF;
    error_response[4] = (crc >> 8) & 0xFF;
    
    UART0_SendArray(error_response, 5);
}


