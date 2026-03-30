#ifndef __USER_INTERATION_H
#define __USER_INTERATION_H
#include "system.h"
void menu_page_handler(KeySignalType keysignal);
void control_page_handler(KeySignalType keysignal);
void solve_graph_page_handler(KeySignalType keysignal);
void track_data_page_handler(KeySignalType keysignal);
void mpu6050_data_page_handler(KeySignalType keysignal);
void led_buzzer_page_handler(KeySignalType keysignal);
void nrf_page_handler(KeySignalType keysignal);
void joy_handler(void);
#endif
