#ifndef _DATA_DEAL_H_
#define _DATA_DEAL_H_

#include "Type_def.h"



uint8_t CRC8_Calc(const uint8_t *sdata, uint16_t len);
uint16_t CRC16_Calc(u8 *pucBuff, u8 unNum);

void data_disp(uint16_t dat, uint8_t unit, uint8_t f, uint8_t type, char* buf);
void meter_data(uint32_t meter_data, uint8_t type, uint16_t *data_deal, uint8_t *unit, uint8_t *f);
void cap_data_disp(char num,char type,uint32_t calc_val, char unit, char *out_str);
#endif