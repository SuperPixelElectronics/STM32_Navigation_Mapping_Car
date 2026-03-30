#ifndef __KEY_H
#define __KEY_H
#include "system.h"


#define KEY_UP_ADC_VAL 0x7be
#define KEY_DOWN_ADC_VAL 0xd2d
#define KEY_LEFT_ADC_VAL 0xbcd
#define KEY_RIGHT_ADC_VAL 0xa8b
#define KEY_CENTER_ADC_VAL 0xc80
#define KEY_NO_ADC_VAL 0xfc5
#define KEY_THRESHOLD 0x10

void key_init(void);
void key_read(void);
void key_process(void);
#endif
