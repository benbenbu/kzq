

#include "modbus.h"
#include "uart.h"
#include <intrins.h>
#include <string.h>
#include "data_save.h"
#include "data_deal.h"
/* ==================== 硬件适配宏（根据实际硬件调整） ==================== */


#define TIMER10MS_TICK      10          /* 10ms定时器基准 */
#define MB_FRAME_TIMEOUT    4           /* 接收超时计数（4*10ms=40ms） */

#define COM_LED_FLASH_TIME  10    // 指示灯亮的时长（10*10ms），可调整

/* ==================== Modbus功能码定义 ==================== */
#define FUNC_CODE_READ_DISCRETE  0x02    /* 读离散量（实时状态） */
#define FUNC_CODE_READ_HOLD_REG  0x03    /* 读保持寄存器（事件记录） */
#define FUNC_CODE_READ_INPUT_REG 0x04    /* 读输入寄存器（实时数据） */

/* ==================== 异常码定义 ==================== */
#define EXC_CODE_INVALID_FUNC    0x01    /* 无效功能码 */
#define EXC_CODE_INVALID_ADDR    0x02    /* 无效地址 */
#define EXC_CODE_INVALID_VALUE   0x03    /* 无效数值（数量超出范围） */

volatile bit com_led_flag = 0;             // 闪灯触发标志
volatile uint8_t com_led_timer = 0;       // 闪灯定时器（10ms级）
/**
 * @brief  组装Modbus响应帧并启动非阻塞发送（解决两次拷贝问题）
 * @param  buf：响应帧数据（不含CRC）
 * @param  len：响应帧长度（不含CRC）
 * @note   1. 仅一次拷贝（若buf为临时缓冲区），或零拷贝（直接填tx_buf）；
 *         2. 自动计算并追加CRC，无需手动处理；
 *         3. 发送过程不阻塞，CPU可处理其他任务。
 */
/**
 * @brief  零拷贝版Modbus响应发送（直接用tx_buf，无拷贝）
 * @param  len：tx_buf中已填充的响应长度（不含CRC）
 * @note   调用前需确保：1. tx_state=0；2. 已往tx_buf填好数据；3. 填充时关了中断
 */
void Send_Response(uint8_t len) {
    uint16_t crc;
    unsigned char SFRPAGE_SAVE = SFRPAGE;

    /* 合法性校验：发送中/缓冲区溢出/长度为0 */
    if(tx_state != 0 || len + 2 > TX_BUF_SIZE || len == 0) {
        return;
    }

    SFRPAGE = UART0_PAGE;
    EA = 0;  // 关中断，避免计算CRC时tx_buf被修改
    /* 计算CRC并追加到tx_buf末尾（Modbus RTU：低字节在前） */
    crc = Modbus_CRC16(tx_buf, len);

    tx_buf[len] = (crc >> 8) & 0xFF;  // CRC高字节
    tx_buf[len+1] = crc & 0xFF;    // CRC低字节		
    EA = 1;

    /* 启动非阻塞发送（总长度=原始长度+2字节CRC） */
    UART0_Start_Send(len + 2);
}
//void Send_Response(uint8_t *buf, uint8_t len) {
//    uint16_t crc;
//    uint8_t i;
//    /* 1. 校验缓冲区是否溢出（len+2 ≤ RESP_BUF_SIZE） */
//    if(len + 2 > RX_BUF_SIZE || buf == NULL || len == 0) {
//        return;
//    }

//    /* 2. 仅一次拷贝（如果buf不是tx_buf，否则可省略！）
//       若调用处直接往tx_buf填数据，可删掉这个循环，实现零拷贝 */
//    for( i = 0; i < len; i++) {
//        rx_buf[i] = buf[i];
//    }

//    /* 3. 计算CRC并追加到tx_buf末尾（Modbus RTU：低字节在前） */
//    crc = Modbus_CRC16(rx_buf, len);

//    rx_buf[len] = (crc >> 8) & 0xFF;  // CRC高字节
//    rx_buf[len+1] = crc & 0xFF;    // CRC低字节
//    /* 4. 阻塞发送（tx_buf：原始数据+CRC，共len+2字节） */
//    UART0_SendBuf_Block(rx_buf, len + 2);
//}

/* ==================== 组装异常响应帧（C89格式） ==================== */
void Build_Exception_Resp(uint8_t func_code, uint8_t exc_code)
{

    
    tx_buf[0] = g_com.address;
    tx_buf[1] = func_code | 0x80; /* 功能码MSB置1 */
    tx_buf[2] = exc_code;
    
    Send_Response(3); /* 3字节（地址+异常功能码+异常码） */
}

/* ==================== 解析02功能码（读实时状态，C89+大端序） ==================== */
uint8_t Parse_Func02(uint8_t *req_buf )
{
    uint16_t start_addr;
    uint16_t reg_num;
    
    /* 校验请求格式：长度8字节 */
    if(rx_len != 8)
    {
        return 1;
    }
    
    /* 解析起始地址和寄存器数量（大端序拼接，接收字节流需手动拼） */
    start_addr = (req_buf[2] << 8) | req_buf[3];
    reg_num = (req_buf[4] << 8) | req_buf[5];
    
    /* 校验地址和数量（固定0000，000E） */
    if(start_addr != 0 || reg_num != 0x000E)
    {
        return 1;
    }
    
    /* 组装响应帧：01 02 02 数据2 数据1（大端序直接填充） */
    tx_buf[0] = g_com.address;
    tx_buf[1] = FUNC_CODE_READ_DISCRETE;
    tx_buf[2] = 0x02; /* 数据长度（2字节） */
    tx_buf[3] = g_com_state>>8; /* 数据2（高字节） */
    tx_buf[4] = (uint8_t)g_com_state; /* 数据1（低字节） */
    
    Send_Response(5); /* 5字节（不含CRC） */
    return 0;
}

/* ==================== 解析04功能码（读实时数据，C89+大端序） ==================== */
uint8_t Parse_Func04(uint8_t *req_buf)
{
    uint16_t start_addr;
    uint16_t reg_num;
    uint8_t chn;
	  uint32_t a,b,c;
    
    if(rx_len < 8)
    {
        return 1;
    }
    
    /* 解析起始地址和寄存器数量 */
    start_addr = (req_buf[2] << 8) | req_buf[3];
    reg_num = (req_buf[4] << 8) | req_buf[5];
    
    /* 分支1：读系统实时数据（01 04 0000 000A） */
    if(start_addr == 0 && reg_num == 0x000A)
    {
        /* 响应帧头：01 04 14 */
        tx_buf[0] = g_com.address;
        tx_buf[1] = FUNC_CODE_READ_INPUT_REG;
        tx_buf[2] = 0x14; /* 数据长度20字节 */
        
        /* 大端序直接拷贝（C51大端序，高字节在前） */
        memcpy(&tx_buf[3], &g_sys_data.q, 4);  /* 无功功率 */
        memcpy(&tx_buf[7], &g_sys_data.p, 4);    /* 有功功率 */
        memcpy(&tx_buf[11], &g_sys_data.u, 4);    /* 系统电压 */
        memcpy(&tx_buf[15], &g_sys_data.i, 4);    /* 系统电流 */
        memcpy(&tx_buf[19], &g_sys_data.c, 4);   /* 功率因数 */
        
        Send_Response( 23); /* 23字节（不含CRC） */
        return 0;
    }
    /* 分支2：读1~4路实时数据（0001/0002/0003/0004，0006） */
    else if((start_addr >= 1 && start_addr <= 4) && reg_num == 0x0006)
    {
        chn = start_addr - 1; /* 0=1路，1=2路，2=3路，3=4路 */
        
        /* 响应帧头：01 04 0C */
        tx_buf[0] = g_com.address;
        tx_buf[1] = FUNC_CODE_READ_INPUT_REG;
        tx_buf[2] = 0x0C; /* 数据长度12字节 */
			
			  a=g_cap_data[start_addr].ia/1000;
			  b=g_cap_data[start_addr].ic/1000;
        c=g_cap_data[start_addr].uo*10;
			
			
        /* 大端序直接拷贝 */
        memcpy(&tx_buf[3], &a, 4);   /* A相电流 */
        memcpy(&tx_buf[7], &b, 4);   /* C相电流 */
        memcpy(&tx_buf[11], &c, 4); /* 零序电压 */
        
        Send_Response( 15); /* 15字节（不含CRC） */
        return 0;
    }
    
    return 1;
}


/* ==================== 解析03功能码（适配新Event_Log_Struct） ==================== */
uint8_t Parse_Func03(uint8_t *req_buf) {
    /* C89：所有局部变量放在函数开头 */
    uint16_t start_addr;
    uint16_t reg_num;
    uint8_t chn;
    uint8_t i;
    uint8_t offset;
    Event_Log_Struct temp_event_20[20];  /* 最新20条事件 */
    Event_Log_Struct temp_chn_event;     /* 单路最新故障 */

    /* 校验请求长度 */
    if (rx_len < 8) {
        return 1;
    }

    /* 解析起始地址和寄存器数量 */
    start_addr = (req_buf[2] << 8) | req_buf[3];
    reg_num = (req_buf[4] << 8) | req_buf[5];

    /* 分支1：读20条事件记录（01 03 0000 0078） */
    if (start_addr == 0 && reg_num == 0x0078) {
        /* 响应帧头：01 03 F0（F0=240字节数据） */
        tx_buf[0] = g_com.address;
        tx_buf[1] = FUNC_CODE_READ_HOLD_REG;
        tx_buf[2] = 0xF0;
        offset = 3;

        /* 读取最新20条事件（适配新结构体） */
        (void)Read_Latest_20_Event_Logs(temp_event_20);

        /* 按协议填充20条事件（核心调整时间字段） */
        for (i = 0; i < 20; i++) {
            /* 1. 故障类型（2字节：高字节在前） */
            tx_buf[offset] = (temp_event_20[i].event_type >> 8) & 0xFF;  /* 高字节 */
            tx_buf[offset + 1] = temp_event_20[i].event_type & 0xFF;     /* 低字节 */
            offset += 2;

            /* 2. 故障数据（4字节：最高字节→最低字节） */
            tx_buf[offset] = (temp_event_20[i].event_data >> 24) & 0xFF; /* 最高字节 */
            tx_buf[offset + 1] = (temp_event_20[i].event_data >> 16) & 0xFF;
            tx_buf[offset + 2] = (temp_event_20[i].event_data >> 8) & 0xFF;
            tx_buf[offset + 3] = temp_event_20[i].event_data & 0xFF;     /* 最低字节 */
            offset += 4;

            /* 3. 故障时间（6字节：year→month→day→hour→minute→second，按顺序填充） */
            tx_buf[offset] = temp_event_20[i].year;    /* 年（完整年份，如2025） */
            tx_buf[offset + 1] = temp_event_20[i].month; /* 月（1~12） */
            tx_buf[offset + 2] = temp_event_20[i].day;   /* 日（1~31） */
            tx_buf[offset + 3] = temp_event_20[i].hour;  /* 时（0~23） */
            tx_buf[offset + 4] = temp_event_20[i].minute;/* 分（0~59） */
            tx_buf[offset + 5] = temp_event_20[i].second;/* 秒（0~59） */
            offset += 6;
        }

        /* 发送响应（243字节：不含CRC） */
        Send_Response( 243);
        return 0;
    }

    /* 分支2：读1~4路最新故障（0001/0002/0003/0004，0006） */
    else if ((start_addr >= 1 && start_addr <= 4) && reg_num == 0x0006) {
        chn = start_addr - 1;  /* 0=1路，1=2路... */

        /* 响应帧头：01 03 0C（12字节数据） */
        tx_buf[0] = g_com.address;
        tx_buf[1] = FUNC_CODE_READ_HOLD_REG;
        tx_buf[2] = 0x0C;
        offset = 3;

        /* 读取对应路最新故障（适配新结构体） */
        (void)Read_Chn_Latest_Event_Log(start_addr,&temp_chn_event);

        /* 填充单路故障数据（核心调整时间字段） */
        /* 1. 故障类型（2字节） */
        tx_buf[offset] = (temp_chn_event.event_type >> 8) & 0xFF;
        tx_buf[offset + 1] = temp_chn_event.event_type & 0xFF;
        offset += 2;

        /* 2. 故障数据（4字节） */
        tx_buf[offset] = (temp_chn_event.event_data >> 24) & 0xFF;
        tx_buf[offset + 1] = (temp_chn_event.event_data >> 16) & 0xFF;
        tx_buf[offset + 2] = (temp_chn_event.event_data >> 8) & 0xFF;
        tx_buf[offset + 3] = temp_chn_event.event_data & 0xFF;
        offset += 4;

        /* 3. 故障时间（6字节：按year→month→day→hour→minute→second填充） */
        tx_buf[offset] = temp_chn_event.year;
        tx_buf[offset + 1] = temp_chn_event.month;
        tx_buf[offset + 2] = temp_chn_event.day;
        tx_buf[offset + 3] = temp_chn_event.hour;
        tx_buf[offset + 4] = temp_chn_event.minute;
        tx_buf[offset + 5] = temp_chn_event.second;

        /* 发送响应（15字节：不含CRC） */
        Send_Response(15);
        return 0;
    }

    /* 地址/数量不匹配，返回异常 */
    return 1;
}
/* ==================== 帧解析主函数（非阻塞，C89格式） ==================== */
void Frame_Parse_Main(void)
{
    uint8_t func_code;
    uint16_t req_crc;
    uint16_t calc_crc;
    uint8_t parse_ret;
    uint8_t req_buf[RX_BUF_SIZE];
    uint8_t i;

   if(tx_state != 0) 
    {
        return;
    }
	
    if(rx_state != 2) /* 无完整帧，直接返回 */
    {
        return;
    }
    
    /* 1. 拷贝接收数据到本地缓冲区（避免volatile影响） */
    for(i = 0; i < rx_len; i++)
    {
        req_buf[i] = rx_buf[i];
    }
    
    /* 2. 校验从机地址 */
    if(req_buf[0] != g_com.address)
    {
        /* 重置接收状态 */
        rx_state = 0;
        rx_len = 0;
        return;
    }
    
	  LED_COM=0;    // 通讯灯亮
    com_led_flag = 1;            // 标记闪灯触发
    com_led_timer = COM_LED_FLASH_TIME; // 重置闪灯时长（100ms）	
		
    /* 3. 校验CRC（请求帧最后2字节是CRC） */
    req_crc = ((uint16_t)req_buf[rx_len-2] << 8) | req_buf[rx_len-1];
    calc_crc = Modbus_CRC16(req_buf, rx_len-2);
    if(req_crc != calc_crc)
    {
        rx_state = 0;
        rx_len = 0;
        return;
    }
    
    /* 4. 解析功能码 */
    func_code = req_buf[1];
    parse_ret = 1;
    
    switch(func_code)
    {
        case FUNC_CODE_READ_DISCRETE:
            parse_ret = Parse_Func02(req_buf);
            break;
        case FUNC_CODE_READ_INPUT_REG:
            parse_ret = Parse_Func04(req_buf);
            break;
        case FUNC_CODE_READ_HOLD_REG:
            parse_ret = Parse_Func03(req_buf);
            break;
        default:
            /* 无效功能码，返回异常 */
            Build_Exception_Resp(func_code, EXC_CODE_INVALID_FUNC);
            parse_ret = 0; /* 异常已处理 */
            break;
    }
    
    /* 5. 解析失败→返回异常 */
    if(parse_ret != 0)
    {
        Build_Exception_Resp(func_code, EXC_CODE_INVALID_ADDR);
    }
    
    /* 6. 重置接收状态 */
    rx_state = 0;
    rx_len = 0;
}
