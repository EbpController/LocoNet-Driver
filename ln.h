/* 
 * file: ln.h
 * author: J. van Hooydonk
 * comments: LocoNet driver, following the project of G. Giebens https://github.com/GeertGiebens
 *
 * revision history:
 *  v0.1 Creation (14/01/2024)
 *  v1.0 Merge PIC18F2525/2620/4525/4620 and PIC18F24/25/26/27/45/46/47Q10 microcontrollers (20/07/2024)
 *  v1.1 Remove PIC18F2525/2620/4525/4620 (obsolete processor)
 */

// this is a guard condition so that contents of this file are not included
// more than once
#ifndef LN_H
#define	LN_H

#include "config.h"
#include "circular_queue.h"

// definitions
#define LINEBREAK_LONG 1800U
#define LINEBREAK_SHORT 600U
#define TIMER1_IDLE 2000U
#define DELAY_60US 42U

#define LN_RX_TX_LED true

// LN flag register
typedef struct
    {
        unsigned TMR1_MODE :2;      // 0 = idle
                                    // 1 = running CMP delay
                                    // 2 = running linebreak
                                    // 3 = running synchronisation BRG
    } LNCON_t;
LNCON_t LNCON;

// LN RX message callback definition (as function pointer)
typedef void (*lnRxMsgCallback_t)(lnQueue_t*);

// LN routines
void lnInit(lnRxMsgCallback_t);
void lnInitCmp1(void);
void lnInitEusart1(void);
void lnInitTmr1(void);
void lnInitIsr(void);
void lnInitLeds(void);

void lnIsr(void);
void lnIsrTmr1(void);
void lnIsrRcError(void);
void lnIsrRc(void);

void rxHandler(uint8_t);

void lnTxMessageHandler(lnQueue_t*);
void startLnTxMessage(void);
void txHandler(void);
bool isChecksumCorrect(lnQueue_t*);

bool isLnFree(void);

void startIdleDelay(void);
void startCmpDelay(void);
void startLinebreak(uint16_t);
void startSyncBrg1(void);
void setBrg1(void);

uint16_t getRandomValue(uint16_t);

// LN used variables
lnRxMsgCallback_t lnRxMsgCallback;
uint8_t _;                          // dummy variable
uint16_t lastRandomValue;           // initial value for the random generator
lnQueue_t lnTxQueue;
lnQueue_t lnTxTempQueue;
lnQueue_t lnRxQueue;
lnQueue_t lnRxTempQueue;

#endif	/* LN_H */

