/* 
 * file: main.c
 * author: J. van Hooydonk
 * comments: main program
 *
 * revision history:
 *  v1.0 creation (28/06/2024)
 */

#include "config.h"
#include "ln.h"

void lnRxMessageHandler(lnQueue_t*);

/**
 * main (start of program)
 */
void main(void) {
    // init the LN driver and give the function pointer for the callback
    lnInit(&lnRxMessageHandler);
    
    // init a temporary LN message queue for transmitting a LN message
    lnQueue_t lnTxMsg;
    initQueue(&lnTxMsg);

    // define some outputs to test the driver
    TRISBbits.TRISB0 = 0; // B0 as output
    TRISBbits.TRISB4 = 0; // B4 as output
    while (1)        
    {
        // set output pin B0 high
        LATBbits.LATB0 = true;
        // transmit a LN message
        enQueue(&lnTxMsg, 0xB2);
        enQueue(&lnTxMsg, 0x00);
        enQueue(&lnTxMsg, 0x10);
        lnTxMessageHandler(&lnTxMsg);
        // delay
        __delay_ms(1000);
    
       // set output pin B0 low
        LATBbits.LATB0 = false;
        // transmit a LN message
        enQueue(&lnTxMsg, 0xB2);
        enQueue(&lnTxMsg, 0x00);
        enQueue(&lnTxMsg, 0x00);
        lnTxMessageHandler(&lnTxMsg);
         // delay       
        __delay_ms(1000);
    }
    return;
}

/**
 * this is the callback function for the LN receiver
 * @param lnRxMsg: the lN message queue
 */
void lnRxMessageHandler(lnQueue_t* lnRxMsg)
{
    // analyse LN message from queue
    switch (lnRxMsg->values[lnRxMsg->head])
    {
        case 0x82:
            // led 'Power' OFF
            LATBbits.LATB4 = false;
            break;
        case 0x83:
            // led 'Power' ON
            LATBbits.LATB4 = true;
            break;
    }

    // clear queue
    while (!isQueueEmpty(lnRxMsg))
    {
        deQueue(lnRxMsg);
    }
}
