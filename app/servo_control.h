/*
 * servo_control.h
 *
 *  Created on: 2024��3��4��
 *      Author: �����
 */

#ifndef APP_SERVO_CONTROL_H_
#define APP_SERVO_CONTROL_H_


#include "zf_common_headfile.h"


#define SERVO_MID 675

void servo_set(int angle);
void servo_init();
void encoder_init(void);
void encoder_get();
int PD_Camera(float expect_val,float err);


#endif /* APP_SERVO_CONTROL_H_ */
