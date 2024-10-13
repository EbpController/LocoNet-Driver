/*
 * file: aw.c
 * author: J. van Hooydonk
 * comments: AW driver
 *
 * revision history:
 *  v1.0 Creation (16/08/2024)
*/

#include "aw.h"

// <editor-fold defaultstate="collapsed" desc="initialisation">

/**
 * AW driver initialisation
 * @param fptr: the function pointer to the (callback) AW handler
*/
void awInit(awCallback_t fptr)
{
    // init servo callback function (function pointer)
    awCallback = fptr;

    // init of the AW ports B and C (= KAWL/KAWR switches)
    awInitPortBC();

    // initialisation of the servo variables
    servoInit(&awUpdate);    
}

/**
 * AW initialisation of the ports B and C (= KAWL/KAWR switches)
 */
void awInitPortBC()
{
    // setup digital inputs to read the KAWL and KAWR switches

    // scheme:
    //
    //               * PORTB pin x (0 to 7) *
    //               |                      |
    //                 /                      /
    //  switch KAWL - /        switch KAWR - /
    //               |                      |
    //          PORTC pin 4            PORTC pin 5
    //         (= KAWL line)         (= KAWR line)
    //
    
    // set all pins of PORTB to inputs with pull-up
    TRISB = 0xff;           // disable output (= input) on pin B0 - B7
    ANSELB = 0x00;          // enable TTL input buffer on pin B0 - B7
    WPUB = 0xff;            // enable pull-up on pin B0 - B7

    // setup PORTC pin 4 and 5 as outputs (controlling KAWL/KAWR switches)
    // and make output high (active low)
    // C4 will be (power) line to control the KAWL switches
    // C5 will be (power) line to control the KAWR switches
    TRISCbits.TRISC4 = false;
    TRISCbits.TRISC5 = false;
    LATCbits.LATC4 = true;
    LATCbits.LATC5 = true;
}

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="routines">

/**
 * this is the callback function for the AW update
 * @param index: the index of AW in the AW list
 */
void awUpdate(uint8_t index)
{
    // update servo on port D
    awUpdateServo(&aw[index], &servoPortD[index], index);
}

/**
 * 
 * @param aw: pointer to the AW parameters
 * @param servo: pointer to the servo
 * @param index: the index of AW in the AW list
 */
void awUpdateServo(AWCON_t *aw, uint16_t *servo, uint8_t index)
{
    // increment pulse width with gradient depending on state of CAW
    if (aw->CAWL == aw->CAWR)
    {
        // if CAWL = CAWR clear KAWs and set the servo position in the middle 
        setKAWL(aw, false, index);
        setKAWR(aw, false, index);
        if (*servo > ((SERVO_MAX + SERVO_MIN) / 2) + GRADIENT)
        {
            *servo -= GRADIENT;
        }
        else if (*servo < ((SERVO_MAX + SERVO_MIN) / 2) - GRADIENT)
        {
            *servo += GRADIENT;
        }
    }
    else
    {
        // if CAWL then clear KAWR and set servo left
        if (aw->CAWL)
        {
            setKAWR(aw, index, false);
            if (getSwitchKAWL(index))
            {
                setKAWL(aw, true, index);
            }
            else
            {
                if (*servo > (SERVO_MAX - GRADIENT))
                {
                    *servo = SERVO_MAX;
                    setKAWL(aw, true, index);
                }
                else
                {
                    *servo += GRADIENT;
                    setKAWL(aw, false, index);
                }                
            }
        }
        // if CAWR then clear KAWL and set servo right
        if (aw->CAWR)
        {
            setKAWL(aw, index, false);
            if (getSwitchKAWR(index))
            {
                setKAWR(aw, true, index);
            }
            else
            {
                if (*servo < SERVO_MIN + GRADIENT)
                {
                    *servo = SERVO_MIN;
                    setKAWR(aw, true, index);
                }
                else
                {
                    *servo -= GRADIENT;
                    setKAWR(aw, false, index);
                }
            }
        }
    }
}

/**
 * set the property CAWL
 * @param aw: pointer to the AW parameters
 * @param value: true or false (= state of CAWL)
 */
void setCAWL(AWCON_t* aw, bool value)
{
    // set CAWL and keep old CAW value in memory
    if (aw->CAWL == true && aw->CAWR == false)
    {
        aw->CAWL_mem = true;
        aw->CAWR_mem = false;
    }
    aw->CAWL = value;
}

/**
 * set the property CAWR
 * @param aw: pointer to the AW parameters
 * @param value: true or false (= state of CAWR)
 */
void setCAWR(AWCON_t* aw, bool value)
{
    // set CAWR and keep old CAW value in memory
    if (aw->CAWR == true && aw->CAWL == false)
    {
        aw->CAWR_mem = true;
        aw->CAWL_mem = false;
    }
    aw->CAWR = value;
}

/**
 * set the property KAWL
 * @param aw: pointer to the AW parameters
 * @param value: true or false (= state of KAWL)
 * @param index: the index of AW in the AW list
 */
void setKAWL(AWCON_t* aw, bool value, uint8_t index)
{
    // set KAWL
    if (aw->KAWL != value)
    {
        aw->KAWL = value;
        // handle the changed KAW state (in the callback function)
        (*awCallback)(aw, index);
    }
}

/**
 * set the property KAWR
 * @param aw: pointer to the AW parameters
 * @param value: true or false (= state of KAWR)
 * @param index: the index of AW in the AW list
 */
void setKAWR(AWCON_t* aw, bool value, uint8_t index)
{
    // set KAWR
    if (aw->KAWR != value)
    {
        aw->KAWR = value;
        // handle the changed KAW state (in the callback function)
        (*awCallback)(aw, index);
    }
}

/**
 * get the state of the switch of KAWL
 * @param index: the index of AW in the AW list
 * @return the state of the switch
 */
bool getSwitchKAWL(uint8_t index)
{
    bool value = false;
    
    // enable KAWL line (active low))
    LATCbits.LATC4 = false;
    // get KAWL switch state
    if ((PORTB & (1 << index)) == 0) { value = true; }    
    // disable KAWL line (active low)
    LATCbits.LATC4 = true;    
    // return the state of the KAWL switch
    return value;
}

/**
 * get the state of the switch of KAWR
 * @param index: the index of AW in the AW list
 * @return the state of the switch
 */
bool getSwitchKAWR(uint8_t index)
{
    bool value = false;
    
    // enable KAWR line (active low))
    LATCbits.LATC5 = false;
    // get KAWR switch state
    if ((PORTB & (1 << index)) == 0) { value = true; }    
    // disable KAWR line (active low)
    LATCbits.LATC5 = true;    
    // return the state of the KAWL switch
    return value;
}

// </editor-fold>
