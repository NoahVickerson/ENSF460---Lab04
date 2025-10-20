/*
 * File:   TIMERs.c
 * Author: noahvickerson
 *
 * Created on September 29, 2025, 11:46 AM
 */


#include "xc.h"

extern TIMER2_event;

void TIMERinit(){  
    /*******************TIMER 2**********************/
    // configure timer 2 for the delay functions
    T2CONbits.T32 = 0; // set Timer2 to one 16 bit timer
    T2CONbits.TCKPS = 0b11; // 1:256 scaling, means ~2 cycles is 1 ms
    T2CONbits.TCS = 0;
    T2CONbits.TSIDL = 0; // timer continues in idle mode
    T2CONbits.TGATE = 0; // not sure what gate accumulation is
    // configure timer2 interrupt
    IPC1bits.T2IP = 2; // second lowest priority
    IFS0bits.T2IF = 0; // reset timer interrupt flag
    IEC0bits.T2IE = 1; // enable timer2 interrupt 

    /******************TIMER 3**********************/
    
}

// general delay function
void delay_ms(uint16_t ms){
    // asserts for clk 500kHz and timer configured
    
    PR2 = (uint16_t)(ms*((float)250/256)); // put the closest number of cycles to the actual time
    TMR2 = 0;
    T2CONbits.TON = 1; // start the timer
    
    // turn off the event
    TIMER2_event = 0;
    
    // idle while waiting, restarting idle until timer is finished or otherwise cut short
    while(!TIMER2_event) Idle();
}

// delay function that will be cut short by the CN interrupt
void delay_ms_itp(uint16_t ms){
    // asserts for clk 500kHz and timer configured
    
    PR2 = (uint16_t)(ms*((float)250/256)); // put the closest number of cycles to the actual time
    TMR2 = 0;
    T2CONbits.TON = 1; // start the timer
    
    // turn off the event
    TIMER2_event = 0;
    
    // idle while waiting, restarting idle until timer is finished or otherwise cut short
    while(!TIMER2_event && !CN_event) Idle();
}
