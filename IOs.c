/*
 * File:   IOs.c
 * Author: noahvickerson
 *
 * Created on September 29, 2025, 11:28 AM
 */
#include <xc.h>
#include "IOs.h"
#include "TIMERs.h"

extern CN_event;

void IOinit()
{
    // PB 1
    TRISBbits.TRISB7 = 1;
    CNPU2bits.CN23PUE = 1; // pull up for PB1
    CNEN2bits.CN23IE = 1; // enable CN interrupt for PB1
    
    // PB 2
    TRISAbits.TRISA4 = 1;
    CNPU1bits.CN0PUE = 1;
    CNEN1bits.CN0IE = 1;
    
    // configure the CN interrupt
    IPC4bits.CNIP = 3; // CN interrupt priority
    IFS1bits.CNIF = 0; // Clear CN interrupt flag
    IEC1bits.CNIE = 1; // Enable CN interrupt
}

// check the IO states, with a delay (determined by the global defines)
// returns a 3 bit response of PB1 | PB2 | PB3 
uint8_t IOCheck(uint8_t check_duration)
{  
    //delay_ms(INPUT_CHECK_DELAY_MS); // set a small uninterruptible delay to allow for any button switches to resolve
    return (!PORTBbits.RB7 << 1) | (!PORTAbits.RA4);
}