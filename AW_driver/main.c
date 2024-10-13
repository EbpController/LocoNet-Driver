/* 
 * file: main.c
 * author: J. van Hooydonk
 * comments: main program
 *
 * revision history:
 *  v1.0 creation (16/08/2024)
 */

#include "config.h"
#include "ln.h"
#include "aw.h"

// declarations routines and variables
void lnRxMessageHandler(lnQueue_t*);
void awHandler(AWCON_t*, uint8_t);
void initPinIO(void);
uint8_t getDipSwitchAddress(void);

lnQueue_t lnTxMsg;

/**
 * main (start of program)
 */
void main(void)
{
    // init IO pins
    initPinIO();
    // init the LN driver and give the function pointer for the callback
    lnInit(&lnRxMessageHandler);
    // init the aw driver
    awInit(&awHandler);    
    // init a temporary LN message queue for transmitting a LN message
    initQueue(&lnTxMsg);

    // main loop
    while (true)        
    {
        // in fact, there is nothing to do here
        // so make just a blinking led (with a period of 1 sec.)
        // to show that the device is running
        LATEbits.LATE0 = true;      // led 'data on (active high)
        __delay_ms(20);
        LATEbits.LATE0 = false;     // led 'data off (active high)
        __delay_ms(980);
    }    
    return;
}

/**
 * this is the callback function for the LN receiver
 * @param lnRxMsg: the lN message queue
 */
void lnRxMessageHandler(lnQueue_t* lnRxMsg)
{
    while (!isQueueEmpty(lnRxMsg))
    {
        // analyse the received LN message from queue
        switch (lnRxMsg->values[lnRxMsg->head])
        {
            case 0xb0:
            {
                // switch function request
                uint8_t index;
                uint8_t address;

                index = lnRxMsg->values[lnRxMsg->head + 1] & 0x07;
                address = (lnRxMsg->values[lnRxMsg->head + 1] & 0x78) >> 3;
                address += (lnRxMsg->values[lnRxMsg->head + 2] & 0x0f) << 4;

                if (address == getDipSwitchAddress())
                {
                    if ((lnRxMsg->values[lnRxMsg->head + 2] & 0x20) == 0x20)
                    {
                        setCAWL(&aw[index], true);
                        setCAWR(&aw[index], false);
                    }
                    else
                    {
                        setCAWL(&aw[index], false);
                        setCAWR(&aw[index], true);
                    }
                }
                break;
            }
            case 0x82:
            {
                // global power OFF request
                for (uint8_t index = 0; index < 8; index++)
                {
                        setCAWL(&aw[index], false);
                        setCAWR(&aw[index], false);
                }
                break;               
            }
            case 0x83:
            {
                // global power ON request
                for (uint8_t index = 0; index < 8; index++)
                {
                        setCAWL(&aw[index], aw[index].CAWL_mem);
                        setCAWR(&aw[index], aw[index].CAWR_mem);
                }
                break;
            }
        }
        // clear the received LN message from queue
        deQueue(lnRxMsg);
    }
}

/**
 * this is the callback function for the AW (when the KAW status is changed)
 * @param aw: the AW parameters
 * @param index: the index of AW in the AW list
 */
void awHandler(AWCON_t* aw, uint8_t index)
{
    // create a 'turnout sensor state report'
    // reference https://wiki.rocrail.net/doku.php?id=loconet:ln-pe-en
    //           https://wiki.rocrail.net/doku.php?id=loconet:lnpe-parms-en
    // OPCODE = 0xB1 (OPC_SW_REP) 
    // SN1 = turnout sensor address
    //       0, A6, A5, A4, A3, A2, A1, A0
    //       (A0 - A3 = index of AW)
    //       (A4 - A6 = DIP switches 1 - 3)
    // SN2 = alternately turnout sensor address and status
    //       0, 0, C, T, A10, A9, A8, A7
    //       (A7 - A10 = DIP switches 4 - 7)
    //       (C = KAWL, T = KAWR)

    // get DIP switch address
    uint16_t address = getDipSwitchAddress();
    
    // make arguments SN1, SN2
    uint8_t SN1 = ((uint8_t)((address << 3) & 0x00f8) + index) & 0x7f;
    uint8_t SN2 = (uint8_t)(address >> 4) & 0x0f;
    if (aw->KAWR) { SN2 |= 0x10; }
    if (aw->KAWL) { SN2 |= 0x20; }
    
    // enqueue message
    enQueue(&lnTxMsg, 0xB1);
    enQueue(&lnTxMsg, SN1);
    enQueue(&lnTxMsg, SN2);
    // transmit the LN message
    lnTxMessageHandler(&lnTxMsg);    
}

/**
 * initialistaion of the IO pins (to read the DIP switch address) *
 */
void initPinIO()
{
    // setup digital inputs to read the DIP switches
    // PORTA = A3 A2 -- --  -- -- A1 A0
    // PORTC = -- -- A9 A8  A7 A6 A5 A4
    
    // we only need to read 8 DIP switches (A0 - A7)
    // this makes the adress A3 - A10 for the complete LN address selection
    // A0 - A2 will be the index of the AW (= 8 turnouts)
    TRISA |= 0xc3;          // disable output (= input) on pin A0 - A1, A6 - A7
    TRISC |= 0x0f;          // disable output (= input) on pin C0 - C3
    
    ANSELA &= 0x3c;         // enable TTL input buffer on pin A0 - A1, A6 - A7
    ANSELC &= 0xf0;         // enable TTL input buffer on pin C0 - C3
    
    WPUA |= 0xc3;           // enable pull-up on pin A0 - A1, A6 - A7
    WPUC |= 0x0f;           // enabel pull-up on pin C0 - C3

    // setup PORTE, bit 0, 1 as digital output (as indication leds)
    TRISEbits.TRISE0 = false;
    TRISEbits.TRISE1 = false;
}

/**
 * get the state of the DIP switches (0 - 9)
 * @return the address (or value of the DIP switches)
 */
uint8_t getDipSwitchAddress()
{
    // return the address
    // address = 0 0 0 0  0 0 A9 A8  A7 A6 A5 A4  A3 A2 A1 A0

    // we only need to read 8 DIP switches (A0 - A7)
    // this makes the adress A3 - A10 for the complete LN address selection
    // A0 - A2 will be the index of the AW (= 8 turnouts)
    uint8_t address;

    address = PORTA & 0x03;             // A1 - A0 on port A, pin 0 - 1
    address += (PORTA >> 4) & 0x0c;     // A3 - A2 on PORT A, pin 6 - 7
    address += (PORTC << 4) & 0xf0;     // A9 - A4 on PORT C, pin 0 - 3

    return address;
}
