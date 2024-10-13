/*
 * file: servo.c
 * author: J. van Hooydonk
 * comments: servo motor driver
 *
 * revision history:
 *  v1.0 Creation (16/08/2024)
*/

#include "servo.h"

// <editor-fold defaultstate="collapsed" desc="initialisation">

/**
 * servo motor driver initialisation
 * @param fptr: the function pointer to the (callback) servo handler
*/
void servoInit(servoCallback_t fptr)
{
    // init servo callback function (function pointer)
    servoCallback = fptr;

    // initialisation of the servo variables
    for (uint8_t i = 0; i < 8; i++)
    {
        servoPortD[i] = 1500U;
    }
    pinIndex = 0;
    
    // init of the other elements (timer, comparator, IST, port)
    servoInitTmr3();
    servoInitCcp1();
    servoInitIsr();
    servoInitPortD();
}

/**
 * servo motor driver initialisation of the timer 3
 */
void servoInitTmr3(void)
{
    // timer 3 must give a high interrupt every 2500µs so that 8 servos
    // will give a 20ms frame rate
    TMR3CLK = 0x01;             // clock source to Fosc / 4
    T3CON = 0b00110000;         // T3CKPS = 0b11 (1:8 prescaler)
                                // SYNC = 0 (ignored)
                                // RD16 = 0 (timer 3 in 8 bit operation)
                                // TMR1ON = 0 (timer 3 is disabled)
    WRITETIMER3(~TIMER3_2500us);// set delay in timer 1
}

/**
 * servo motor driver initialisation of the comparator (CCP1)
 */
void servoInitCcp1(void)
{
    // initialisation comparator (CCP1)
    CCPTMRSbits.C1TSEL = 2;     // CCP1 is based of timer 3
    CCP1CONbits.MODE = 8;       // set output mode
    CCP1CONbits.EN = true;      // enable comparator (CCP1)    
    CCPR1 = ~(TIMER3_2500us - (servoPortD[pinIndex] * 2));
}

/**
 * servo motor driver initialisation of the interrupt service routine
 */
void servoInitIsr(void)
{
    // set global interrupt parameters
    INTCONbits.IPEN = true;     // enable priority levels on iterrupt
    INTCONbits.GIEH = true;     // enable all high priority interrupts
    INTCONbits.GIEL = true;     // enable all low priority interrupts
    // set comparator (CCP1) interrrupt parameters
    IPR6bits.CCP1IP = true;     // comparator (CCP1) interrupt high priority
    PIE6bits.CCP1IE = true;     // enable comparator (CCP1) overflow interrupt
    // set timer 3 interrrupt parameters
    IPR4bits.TMR3IP = true;     // timer 3 interrupt high priority
    PIE4bits.TMR3IE = true;     // enable timer 3 overflow interrupt
    T3CONbits.ON = true;        // enable timer 3
}

/**
 * servo motor driver initialisation of the output port D (= 8 servos)
 */
void servoInitPortD(void)
{
    // port D
    TRISD = 0x00;               // configure all pins of port D as output
    LATD = 0x00;                // set them to 0    
}

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="ISR">

// <editor-fold defaultstate="collapsed" desc="ISR high priority">

// there are two possible high interrupt triggers, coming from
// the timer 3 overrun flag and/or coming from the comparator

/**
 * high priority interrupt service routine
 */
void __interrupt(high_priority) servoIsr(void)
{
    if (PIR4bits.TMR3IF)
    {
        // timer 3 interrupt
        // clear the interrupt flag and handle the request
        PIR4bits.TMR3IF = false;
        servoIsrTmr3();
    }
    if (PIR6bits.CCP1IF)
    {
        // comparator (CCP1) interrupt
        // clear the interrupt flag and handle the request
        PIR6bits.CCP1IF = false;
        servoIsrCcp1();
    }
}

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="ISR timer 3">

/**
 * interrupt routine for timer 3
 */
void servoIsrTmr3(void)
{
    // increment pin index
    pinIndex++;
    if (pinIndex == 8)
    {
        pinIndex = 0;
    }
    // get servo values (in the callback function)
    (*servoCallback)(pinIndex);
    // set comparator (CCP1)
    CCPR1 = ~(TIMER3_2500us - (servoPortD[pinIndex] * 2));
    // toggle output port D pin[pinIndex]
    LATD = (uint8_t)(0x01 << pinIndex);
    // reload timer 3
    // the timer must be started after all previous setting are done
    WRITETIMER3(~TIMER3_2500us);// set delay in timer 3
}

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="ISR comparator (CCP1 and CCP2)">

/**
 * interrupt routine for comparator (CCP1)
 */
void servoIsrCcp1(void)
{
    // set (all) pin(s) of port D to 0
    LATD = 0x00;
}

// </editor-fold>

// </editor-fold>
