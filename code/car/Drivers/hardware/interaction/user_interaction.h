#ifndef __USER_INTERATION_H
#define __USER_INTERATION_H
#include "system.h"
void menu_page_handler(KeySignalType keysignal);
void encoder_data_page_handler(KeySignalType keysignal);
void encoder_graph_page_handler(KeySignalType keysignal);
void mpu6050_data_page_handler(KeySignalType keysignal);
void mpu6050_graph_page_handler(KeySignalType keysignal);
void combined_data_page_handler(KeySignalType keysignal);
void combined_graph_page_handler(KeySignalType keysignal);
void motor_control_page_handler(KeySignalType keysignal);
void nrf_page_handler(KeySignalType keysignal);
void led_buzzer_sys_page_handler(KeySignalType keysignal);
void track_data_page_handler(KeySignalType keysignal);
void track_graph_page_handler(KeySignalType keysignal);
void map_data_page_handler(KeySignalType keysignal);
void map_graph_page_handler(KeySignalType keysignal);
void map_pure_page_handler(KeySignalType keysignal);
void solve_data_page_handler(KeySignalType keysignal);
void solve_graph_page_handler(KeySignalType keysignal);
#endif
