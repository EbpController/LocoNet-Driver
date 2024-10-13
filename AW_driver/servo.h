/* 
 * file: servo.h
 * author: J. van Hooydonk
 * comments: servo motor driver
 *
 * revision history:
 *  v1.0 Creation (16/08/2024)
 */

// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef SERVO_H
#define	SERVO_H

#include "config.h"

// definitions
#define TIMER3_2500us 5000U

// servo callback definition (as function pointer)
typedef void (*servoCallback_t)(uint8_t);

// routines
void servoInit(servoCallback_t);
void servoInitTmr3(void);
void servoInitCcp1(void);
void servoInitIsr(void);
void servoInitPortD(void);

void servoIsr(void);
void servoIsrTmr3(void);
void servoIsrCcp1(void);

// variables
servoCallback_t servoCallback;
uint16_t servoPortD[8];
uint8_t pinIndex;

#endif	/* SERVO_H */
