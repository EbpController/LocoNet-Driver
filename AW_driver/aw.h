/* 
 * file: aw.h
 * author: J. van Hooydonk
 * comments: AW driver
 *
 * revision history:
 *  v1.0 Creation (16/08/2024)
 */

// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef AW_H
#define	AW_H

#include "config.h"
#include "servo.h"

// definitions
// sweeptime time in ms from min/max to max/min position
#define SWEEPTIME 1500U
// the puls duration of the servo must be between 1000µs and 2000µs (SG90)
#define SERVO_MIN 750U              // max. value = 500 (= -90°)
#define SERVO_MAX 2000U             // max. value = 2250 (= +90°)
// the period for the servo is 20ms
// so, for a certain sweeptime, the number of steps to add or subtrack is
// equal to the sweeptime divided by the period (SWEEPTIME / 20)
// the value (= GRADIENT) to add or subtract is than calculated as follow
#define GRADIENT (uint8_t)((SERVO_MAX - SERVO_MIN) / (SWEEPTIME / 20))

// AW status register
typedef struct
    {
        bool CAWL;
        bool CAWR;
        bool CAWL_mem;
        bool CAWR_mem;
        bool KAWL;
        bool KAWR;
    } AWCON_t;
AWCON_t AWCON;

// AW callback definition (as function pointer)
typedef void (*awCallback_t)(AWCON_t*, uint8_t);

// routines
void awInit(awCallback_t);
void awInitPortBC(void);
void awUpdate(uint8_t);
void awUpdateServo(AWCON_t*, uint16_t*, uint8_t);
void setCAWL(AWCON_t*, bool);
void setCAWR(AWCON_t*, bool);
void setKAWL(AWCON_t*, bool, uint8_t);
void setKAWR(AWCON_t*, bool, uint8_t);
bool getSwitchKAWL(uint8_t);
bool getSwitchKAWR(uint8_t);

// variables
awCallback_t awCallback;
AWCON_t aw[8];

#endif	/* AW_H */
