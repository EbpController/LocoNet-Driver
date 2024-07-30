/*
 * file: ln.c
 * author: J. van Hooydonk
 * comments: LocoNet driver, following the project of G. Giebens https://github.com/GeertGiebens
 *
 * revision history:
 *  v0.1 Creation (14/01/2024)
 *  v1.0 Merge PIC18F2525/2620/4525/4620 and PIC18F24/25/26/27/45/46/47Q10 microcontrollers (20/07/2024)
*/

#include <pic18f46q10.h>
#include <pic18.h>

#include "ln.h"

// <editor-fold defaultstate="collapsed" desc="initialisation">

/**
 * LN initialisation
 * @param fptr: the function pointer to the (callback) LN RX message handler
*/
void lnInit(lnRxMsgCallback_t fptr)
{
    // init LN RX message callback function (function pointer)
    lnRxMsgCallback = fptr;
    
    // declaration and initialisation of the RX and TX queue
    // essentially the queue is just a pointer to the instance of the struct
    initQueue(&lnTxQueue);
    initQueue(&lnTxTempQueue);
    initQueue(&lnRxQueue);
    initQueue(&lnRxTempQueue);
    
    // init of the other elements (clock, comparator, EUSART, timer, ISR, leds)
    lnInitOscillator();
    lnInitCmp1();
    lnInitEusart1();
    lnInitTmr1();
    lnInitIsr();
    lnInitLeds();
}

/**
 * initialisation of the internal HS oscillator to the maximum speed of 32MHz
 */
void lnInitOscillator(void)
{
    // this is only neccesary for non PIC18FxxQ10 microcontrollers
    #ifdef _18FXXQ10_FAMILY_
        NOP();
    #else
        // set oscillator to 8MHz
        OSCTUNE = 0x00;
        OSCCON = 0x70;

        // wait till oscillator is stable
        while (!OSCCONbits.IOFS)
        {
            NOP();
        }
        // then enable the speed up PLL circuit to get a frequncy of 4 x 8 MHz
        OSCTUNEbits.PLLEN = true;
    #endif
}

/**
 * LN initialisation of the comparator input circuit
 */
void lnInitCmp1(void)
{
    // set pins for CMP 1
    ANSELAbits.ANSELA3 = true;      // PORT A, pin 0 = (analog) input, CMP 1 IN+
    TRISAbits.TRISA3 = true;
    TRISAbits.TRISA4 = false;       // PORT A, pin 4 = output, CMP 1 OUT
    #ifdef _18FXXQ10_FAMILY_
        // use the fixed voltage reference to feed the Vin-
        // refer to PIC18FxxQ10 datasheet 'FVR (fixed voltage reference)'
        FVRCON = 0x0c;              // CDAFVR buffer gain is 4x (4.096V))
        FVRCONbits.FVREN = true;    // enable FVR
        // wait till FVR is ready to use
        while (!FVRCONbits.FVRRDY)
        {
            NOP();
        }
        // refer to PIC18FxxQ10 datasheet 'pin allocation tables' and 'CMP module'
        CM1NCH = 0x06;              // CMP 1 Vin- = FVR
        CM1PCH = 0x01;              // CMP 1 Vin+ = RA3 (CxIN1+)
        // refer to PIC18FxxQ10 datasheet 'PPS module' and 'CMP module'
        RA4PPS = 0x0d;              // CMP 1 Vout = RA4 (CxOUT))
        // refer to PIC18FxxQ10 datasheet 'slew rate control'
        SLRCONAbits.SLRA4 = true;   // set pin to limited slew rate
        
        CM1CON0bits.EN = true;      // enable CMP 1
    #else
        ANSELAbits.ANSELA0 = true;  // PORT A, pin 0 = (analog) input, CMP 1 IN-
        TRISAbits.TRISA0 = true;
        TRISAbits.RA4 = false;      // PORT A, pin 4 = output, CMP 1 OUT
        CMCON = 0b00000001;         // one indipendent comparator with output
                                    // CMP 1 Vin- = RA0 (CxIN0-)
                                    // CMP 1 Vin+ = RA3 (CxIN1+)
                                    // CMP 1 Vout = RA4 (CxOUT)
    #endif
}

/**
 * LN initialisation of the EUSART to baudrate of 16.666
 */
void lnInitEusart1(void)
{
    // set pins for EUSART 1 RX and TX
    TRISCbits.TRISC6 = false;   // PORT C, pin 6 = LN TX
    ANSELCbits.ANSELC6 = false;
    TRISCbits.TRISC7 = true;    // PORT C, pin 7 = LN RX
    ANSELCbits.ANSELC7 = false;
    #ifdef _18FXXQ10_FAMILY_
        // refer to PIC18FxxQ10 datasheet 'PPS module' and 'CMP module'
        RC6PPS = 0x09;              // EUSART 1 TX = RC6
        RX1PPS = 0x17;              // EUSART 1 RX = RC7

        // configure EUSART 1
        BAUD1CONbits.SCKP = true;   // invert TX output signal
        BAUD1CONbits.BRG16 = false; // 16-bit baudrate generator
        TX1STAbits.SYNC = false;    // asynchronous mode
        TX1STAbits.BRGH = false;    // low speed    
        RC1STAbits.CREN = false;    // first clear bit CREN to clear the OERR bit
        RC1STAbits.CREN = true;     // enable receiver
        _ = RC1REG;                 // read the receive register to clear his
                                    // content and to clear the FERR bit

        setBrg1();                  // set and enable the BRG

        RC1STAbits.SPEN = true;     // enable serial port
    #else
        // configure EUSART
        BAUDCONbits.TXCKP = true;   // invert TX output signal
        BAUDCONbits.BRG16 = true;   // 16-bit baudrate generator
        TXSTAbits.SYNC = false;     // asynchronous mode
        TXSTAbits.BRGH = false;     // low speed
        TXSTAbits.TXEN = true;      // enable transmitter
        RCSTAbits.CREN = false;     // clear bit CREN to clear the OERR bit
        RCSTAbits.CREN = true;      // enable receiver
        _ = RCREG;                  // read the receive register to clear his
                                    // content and to clear the FERR bit

        setBrg1();                  // set and enable the BRG

        RCSTAbits.SPEN = true;      // enable serial port
    #endif
}

/**
 * LN initialisation of the timer 1
 */
void lnInitTmr1(void)
{
    #ifdef _18FXXQ10_FAMILY_
        TMR1H = 0x00;               // reset timer 1
        TMR1L = 0x00;
        TMR1CLK = 0x01;             // clock source to Fosc / 4
        T1CON = 0b00110000;         // T1CKPS = 0b11 (1:8 prescaler)
                                    // T1OSCEN = 0 (oscillator is disabled)
                                    // SYNC = 0 (ignored)
                                    // RD16 = 0 (timer 1 in 8 bit operation)
                                    // TMR1ON = 0 (timer 1 is disabled)
    #else
        TMR1H = 0x00;               // reset timer1
        TMR1L = 0x00;
        T1CON = 0b00110000;         // RD16 = 0 (timer1 in 8 bit operation)
                                    // T1RUN = 0 (driven by another source)
                                    // T1CKPS = 0b11 (1:8 prescaler)
                                    // T1OSCEN = 0 (oscillator is disabled)
                                    // T1SYNC = 0 (ignored)
                                    // TMR1CS = 0 (source: internal clock = FOSC/4)
                                    // TMR1ON = 0 (timer1 is disabled)
    #endif
}

/**
 * LN initialisation of the interrupt service routine
 */
void lnInitIsr(void)
{
    #ifdef _18FXXQ10_FAMILY_
        IPR3bits.RC1IP = false;     // EUSART 1 RXD interrupt low priority
        IPR4bits.TMR1IP = false;    // timer 1 interrupt low priority
        INTCONbits.IPEN = true;     // enable priority levels on iterrupt
        INTCONbits.GIEH = true;     // enable all high priority interrupts
        INTCONbits.GIEL = true;     // enable all low priority interrupts
        PIE3bits.RC1IE = true;      // enable EUSART 1 RXD interrupt
        PIE4bits.TMR1IE = true;     // enable timer 1 overflow interrupt

        T1CONbits.TMR1ON = true;    // enable timer 1
    #else
        IPR1bits.TMR1IP = false;    // timer1 interrupt low priority
        IPR1bits.RCIP = false;      // rxd interrupt low priority
        RCONbits.IPEN = true;       // enable priority levels on iterrupt
        INTCONbits.GIEH = true;     // enable all high priority interrupts
        INTCONbits.GIEL = true;     // enable all low priority interrupts
        PIE1bits.RCIE = true;       // enable rxd interrupt
        PIE1bits.TMR1IE = true;     // enable timer 1 overflow interrupt

        T1CONbits.TMR1ON = true;    // enable timer 1
    #endif
    lastRandomValue = 1234u;        // set a random value != 0
    startCmpDelay();                // start LN driver with CMP delay
}

/**
 * LN initialisation of the leds
 */
void lnInitLeds(void)
{
    TRISAbits.TRISA5 = false;       // A5 as output
    LATAbits.LATA5 = true;          // led 'data on LN' off (active low)
    #if LN_RX_TX_LED
        TRISEbits.TRISE0 = false;   // E0 as output
        LATEbits.LATE0 = true;      // led 'data on LN RX' off (active low)
        TRISEbits.TRISE1 = false;   // E1 as output
        LATEbits.LATE1 = true;      // led 'data on LN TX' off (active low)
    #endif
}

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="ISR">

// <editor-fold defaultstate="collapsed" desc="ISR low priority">

// there are two possible low interrupt triggers, coming from
// the EUSART data receiver and/or coming from the timer 1 overrun flag

/**
 * low priority interrupt service routine
 */
void __interrupt(low_priority) lnIsr(void)
{
    #ifdef _18FXXQ10_FAMILY_
        if (PIR4bits.TMR1IF)
        {
            // timer 1 interrupt
            // clear the interrupt flag and handle the request
            PIR4bits.TMR1IF = false;
            lnIsrTmr1();
        }
        else if (PIR3bits.RC1IF)
        {
            // EUSART RC interupt
            if (RC1STAbits.FERR)
            {
                // EUSART framing error (linebreak detected)
                // read RCREG to clear the interrupt flag and FERR bit
                _ = RC1REG;
                // retreive (recover) the last transmitted LN message
                recoverLnMessage(&lnTxTempQueue);
                // this framing error detection takes about 600탎
                // (10bits x 60탎) and a linebreak duration is specified at
                // 900탎, so add 300탎 after this detection time to complete
                // a full linebreak
                startLinebreak(LINEBREAK_SHORT);
            }
            else
            {
                // EUSART data received
                // handle the received data byte
                lnIsrRc();
            }
        }
    #else
        if (PIR1bits.TMR1IF)
        {
            // timer 1 interrupt
            // clear the interrupt flag and handle the request
            PIR1bits.TMR1IF = false;
            lnIsrTmr1();
        }
        else if (PIE1bits.RCIE)
        {
            // EUSART RC interupt
            if (RCSTAbits.FERR)
            {
                // EUSART framing error (linebreak detected)
                // read RCREG to clear the FERR bit
                _ = RCREG;
                // retreive (recover) the last transmitted LN message
                recoverLnMessage(&lnTxTempQueue);
                // this framing error detection takes about 600탎
                // (10bits x 60탎) and a linebreak duration is specified at
                // 900탎, so add 300탎 after this detection time to complete
                // a full linebreak
                startLinebreak(LINEBREAK_SHORT);
            }
            else
            {
                // EUSART data received
                // handle the received data byte
                lnIsrRc();
            }
        }
    #endif
}

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="ISR timer 1">

/**
 * interrupt routine for timer 1
 */
void lnIsrTmr1(void)
{
    switch (LNCON.TMR1_MODE)
    {
        case 0:
            // LN driver is in idle mode
            if (isLnFree())
            {
                // LN is free
                if (!isQueueEmpty(&lnTxTempQueue))
                {
                    // if LN TX temporary queue is not empty restart
                    // the tramsmission (there is still something to be sent)
                    // this may occur when the last TX message was transmitted
                    // with errors (eg. after linebreak, conflict RX-TX, ...)
                    // start sync BRG before transmitting the first data byte
                    startSyncBrg1();
                }
                else if (!isQueueEmpty(&lnTxQueue))
                {
                    // if LN TX queue has a LN message 
                    startLnTxMessage();
                }
                else
                {
                    // LN is free but nothing has to be transmitted
                    // restart timer 1 with idle delay
                    startIdleDelay();
                }
            }
            else
            {
                // LN is not free, so start timer 1 with CMP delay
                startCmpDelay();
            }
            break;
        case 1:
            // after the CMP delay
            if (isLnFree())
            {
                // if LN line is free start timer 1 with idle delay
                startIdleDelay();
            }
            else
            {
                // if LN line is not free restart timer 1 with CMP delay
                startCmpDelay();
            }
            break;
        case 2:
            // after the linebreak (delay) start CMP delay
            #ifdef _18FXXQ10_FAMILY_
                RC1STAbits.SPEN = true; // (re-)enable the receiver
            #else
                RCSTAbits.SPEN = true;  // (re-)enable the receiver
            #endif
            PORTCbits.RC6 = false;      // and restore output pin
            startCmpDelay();            // start the timer 1 with CMP delay
            break;
        case 3:
            // after the synchronisation of the BRG start sending the LN message
            LNCON.TMR1_MODE = 0;
            txHandler();
            break;
        default:
            break;
    }
}

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="ISR RX">

/**
 * interrupt routine for EUSART RX
 */
void lnIsrRc(void)
{
    // get the received value
    #ifdef _18FXXQ10_FAMILY_
        uint8_t lnRxData = RC1REG;
    #else
        uint8_t lnRxData = RCREG;
    #endif

    if (!isQueueEmpty(&lnTxTempQueue))
    {
        // device is in TX mode
        // check if received byte = transmitted byte
        if (lnRxData == lnTxTempQueue.values[lnTxTempQueue.head])
        {
            // if last value is correct transmitted then dequeue
            deQueue(&lnTxTempQueue);
            if (!isQueueEmpty(&lnTxTempQueue))
            {
                // send next data of LN message untill queue is empty
                txHandler();
            }
            else
            {
                // restart CMP delay
                startCmpDelay();
                #if LN_RX_TX_LED
                    // led 'data on LN TX' off (active low)
                    LATEbits.LATE1 = false;
                #endif
            }
        }
        else
        {
            // if LN RX data is not equal to LN TX data send linebreak
            startLinebreak(LINEBREAK_LONG);
        }
    }
    else
    {
        // device is in RX mode (receive LN message)
        rxHandler(lnRxData);
        // restart CMP delay
        startCmpDelay();
    }
}

// </editor-fold>

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="RX routines">

/**
 * EUSART RX handler
 * @param lnRxData: the received databyte
 */
void rxHandler(uint8_t lnRxData)
{
    // start testing if msb = 1 (this is the startbyte of the LN message)
    if ((lnRxData & 0x80) == 0x80)
    {
        clearQueue(&lnRxTempQueue);
        enQueue(&lnRxTempQueue, lnRxData);
    }
    else
    {
        enQueue(&lnRxTempQueue, lnRxData);

        // determine length of LN message
        uint8_t lnMessageLength;
        lnMessageLength = (lnRxTempQueue.values[lnRxTempQueue.head] & 0x60);
        lnMessageLength = (lnMessageLength >> 4) + 2;
        if (lnMessageLength > 6)
        {
            lnMessageLength = lnRxTempQueue.values[lnRxTempQueue.head + 1];
        }

        // has LN message reached the end the test checksum
        if (lnMessageLength == lnRxTempQueue.numEntries)
        {
            if (isChecksumCorrect(&lnRxTempQueue))
            {
                // if checksum is correct then copy LN RX temp queue to
                // LN RX queue
                while (!isQueueEmpty(&lnRxTempQueue))
                {
                    enQueue(&lnRxQueue, lnRxTempQueue.values[lnRxTempQueue.head]);
                    deQueue(&lnRxTempQueue);
                }
                #if LN_RX_TX_LED
                    // led 'data on LN RX' on (active low)
                    LATEbits.LATE0 = false;
                #endif
                // handle LN RX message (in the callback function)
                (*lnRxMsgCallback)(&lnRxQueue);
            }
        }
    }     
}

/**
 * calculate the checksum
 * @param lnQueue: name of the queue (pass the address of the queue)
 * @return true: if checksum is correct 
 */
bool isChecksumCorrect(lnQueue_t* lnQueue)
{
    uint8_t checksum = 0;    
    for (uint8_t i = 0; i < lnQueue->numEntries; i++)
    {
        checksum ^= lnQueue->values[(lnQueue->head + i) % lnQueue->size];
    }    
    return (checksum == 0xff);
}

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="TX routines">

/**
 * start routine for transmitting a LN message
 * @param the message to transmit
 */
void lnTxMessageHandler(lnQueue_t* lnTxMsg)
{
    // disable the interrupts while copying the LN message
    di();

    // copy the LN message into the LN TX queue
    // and add the calculated checksum
    uint8_t checksum = 0x00;
    
    while (!isQueueEmpty(lnTxMsg))
    {
        checksum ^= lnTxMsg->values[lnTxMsg->head];
        enQueue(&lnTxQueue, lnTxMsg->values[lnTxMsg->head]);
        deQueue(lnTxMsg);
    }
    enQueue(&lnTxQueue, (checksum ^ 0xff));

    // reenable the interrupts
    ei();
}

/**
 * begin of routine for transmitting a LN message
 */
void startLnTxMessage(void)
{
    // this routine is driven by (timer) interrupt, so don't call it directly
    // first, copy next LN message from LN TX queue into LN TX temporary queue
    do
    {
        enQueue(&lnTxTempQueue, lnTxQueue.values[lnTxQueue.head]);
        deQueue(&lnTxQueue);
    }
    while (!isQueueEmpty(&lnTxQueue) &&
            ((lnTxQueue.values[lnTxQueue.head] & 0x80) != 0x80));
    // sync BRG before transmitting the first data byte
    startSyncBrg1();            
}

/**
 * routine that handles the transmission of the message
 */
void txHandler(void)
{
    if (isLnFree())
    {
        // the last transmited value (TX1REG) must be stored (in lnTxData)
        // this is necessary to check if the data is transmitted correctly
        // (see routine rxHandler)
        #ifdef _18FXXQ10_FAMILY_
            TX1REG = lnTxTempQueue.values[lnTxTempQueue.head];
        #else
            TXREG = lnTxTempQueue.values[lnTxTempQueue.head];
        #endif
    }
    else
    {
        // if line is not free start the linebreak
        startLinebreak(LINEBREAK_LONG);
    }
}

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="LN routines">

/**
 * check if the LocoNet is free (to use)
 * @return true: if the line is free, false: if the line is occupied
 */
bool isLnFree(void)
{
    // check if:
    //  RC7 = 1 (PORT C, bit 7 = high)
    //  RCIDL = 1 (receiver is idle = no data reception in progress)
    #ifdef _18FXXQ10_FAMILY_
        return (PORTCbits.RC7 && BAUD1CONbits.RCIDL);
    #else
        return (PORTCbits.RC7 && BAUDCONbits.RCIDL);
    #endif
}
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="timer 1 routines">

/**
 * start of the idle delay (1000탎)
 */
void startIdleDelay(void)
{
    // delay = 1000탎 (timer 1 in idle mode)
    WRITETIMER1(~TIMER1_IDLE);      // set delay in timer 1
    LNCON.TMR1_MODE = 0;            // 0: timer 1 in idle mode    
    // in idle mode, the leds on LN (RX + TX) can be turned off (active low)
    LATAbits.LATA5 = true;
    #if LN_RX_TX_LED
        LATEbits.LATE0 = true;
        LATEbits.LATE1 = true;
    #endif
}

/**
 * start of the carrier + master + priority delay
 */
void startCmpDelay(void)
{
    // delay CMP = 1200탎 + 360탎 + random (between 0탎 and 1023탎)
    uint16_t delay = getRandomValue(lastRandomValue);
    lastRandomValue = delay;        // store last value of random generator
    #ifdef _18FXXQ10_FAMILY_
        delay &= 2047U;             // get random value between 0 and 1023
        delay += 3120U;             // add C + M delay (= 1560탎)
    #else
        delay &= 1023U;             // get random value between 0 and 1023
        delay += 1560U;             // add C + M delay (= 1560탎)
    #endif
    WRITETIMER1(~delay);            // set delay in timer 1
    LNCON.TMR1_MODE = 1;            // 1: timer 1 in CMP delay mode
    // led 'data on LN' on (active low)
    LATAbits.LATA5 = false;
}

/**
 * random generator with Galois shift register
 * @param lfsr: initial value for the shift register
 * @return a 16 bit random vaule
 */
uint16_t getRandomValue(uint16_t lfsr)
{
    // with the help of a Galois LFSR (linear-feedback shift register)
    // we can get a random number. The LFSR is a shift register whose
    // input bit is a linear function of its previous state
    // refer: https://en.wikipedia.org/wiki/Linear-feedback_shift_register
    
    uint16_t lsb = lfsr & 1u;       // get LSB (i.e. the output bit)
    lfsr >>= 1;                     // shift register to right
    if (lsb)                        // if the output bit is 1
    {
        lfsr ^= 0xb400u;            // apply toggle mask
    }
    return lfsr;
}

/**
 * start of the linebreak delay (with a well defined time)
 * @param the time of the linebreak
 * @return 
 */
void startLinebreak(uint16_t time)
{
    // linebreak detect by framing error
    #ifdef _18FXXQ10_FAMILY_
        RC1STAbits.SPEN = false;    // stop EUSART
    #else
        RCSTAbits.SPEN = false;     // stop EUSART
    #endif
    PORTCbits.RC6 = true;
    // a LN linebreak definition 
    WRITETIMER1(~time);
    LNCON.TMR1_MODE = 2;            // 2: timer 1 in linebreak mode
}

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="BRG settings">

/**
 * routine to synchronize the BRG
 */
void startSyncBrg1(void)
{
    // this routine handles the synchronisation of the BRG
    // it is important that the transmission of the message could be started
    // directly alter putting the data in the RCREG (the start of the
    // transmission depends on the state/value of the timer of the BRG)
    // a delay in the start of the transmission may lead to a none-detection
    // whether the line is still free
    // to make this possible restart the BRG and start a delay of
    // approximately 60탎
    setBrg1();
    WRITETIMER1(~DELAY_60US);   // set delay approxity 60탎 (= 1 bit) in timer 1
    LNCON.TMR1_MODE = 3;        // 3: timer 1 mode in synchronisation BRG
}

/**
 * sets and initialise the value of the bautrate generator
 */
void setBrg1(void)
{
    #ifdef _18FXXQ10_FAMILY_
        // desired baudrate = 16.666
        // BRG value = (64.000.000 / (64 x 16.666)) - 1 = 59 (0x3B)
        // calculated baudrate = 64.000.000 / (64 x (59 + 1)) = 1.666,666667
        // error = (1.666,666667 - 1.666) / 1.666 = 0.04 %
        SP1BRG = 59U;

       // this let the BRG do the synchronisation
        TX1STAbits.TXEN = false;
        TX1STAbits.TXEN = true;
    #else
        // desired baudrate = 16.666
        // BRG value = (32.000.000 / (16 x 16.666)) - 1 = 119 (0x77)
        // calculated baudrate = 32.000.000 / (16 x (119 + 1)) = 1.666,666667
        // error = (1.666,666667 - 1.666) / 1.666 = 0.04 %
        
        // this let the BRG do the synchronisation
        SPBRGH = 0U;
        SPBRG = 119U;
    #endif
}

// </editor-fold>
