/*
 * File:   newmainXC16.c
 * Author: noahvickerson
 *
 * Created on October 20, 2025, 2:51 PM
 */

// FBS
#pragma config BWRP = OFF               // Table Write Protect Boot (Boot segment may be written)
#pragma config BSS = OFF                // Boot segment Protect (No boot program Flash segment)

// FGS
#pragma config GWRP = OFF               // General Segment Code Flash Write Protection bit (General segment may be written)
#pragma config GCP = OFF                // General Segment Code Flash Code Protection bit (No protection)

// FOSCSEL
#pragma config FNOSC = FRC              // Oscillator Select (Fast RC oscillator (FRC))
#pragma config IESO = OFF               // Internal External Switch Over bit (Internal External Switchover mode disabled (Two-Speed Start-up disabled))

// FOSC
#pragma config POSCMOD = NONE           // Primary Oscillator Configuration bits (Primary oscillator disabled)
#pragma config OSCIOFNC = ON            // CLKO Enable Configuration bit (CLKO output disabled; pin functions as port I/O)
#pragma config POSCFREQ = HS            // Primary Oscillator Frequency Range Configuration bits (Primary oscillator/external clock input frequency greater than 8 MHz)
#pragma config SOSCSEL = SOSCHP         // SOSC Power Selection Configuration bits (Secondary oscillator configured for high-power operation)
#pragma config FCKSM = CSECMD           // Clock Switching and Monitor Selection (Clock switching is enabled, Fail-Safe Clock Monitor is disabled)

// FWDT
#pragma config WDTPS = PS32768          // Watchdog Timer Postscale Select bits (1:32,768)
#pragma config FWPSA = PR128            // WDT Prescaler (WDT prescaler ratio of 1:128)
#pragma config WINDIS = OFF             // Windowed Watchdog Timer Disable bit (Standard WDT selected; windowed WDT disabled)
#pragma config FWDTEN = OFF             // Watchdog Timer Enable bit (WDT disabled (control is placed on the SWDTEN bit))

// FPOR
#pragma config BOREN = BOR3             // Brown-out Reset Enable bits (Brown-out Reset enabled in hardware; SBOREN bit disabled)
#pragma config PWRTEN = ON              // Power-up Timer Enable bit (PWRT enabled)
#pragma config I2C1SEL = PRI            // Alternate I2C1 Pin Mapping bit (Default location for SCL1/SDA1 pins)
#pragma config BORV = V18               // Brown-out Reset Voltage bits (Brown-out Reset set to lowest voltage (1.8V))
#pragma config MCLRE = ON               // MCLR Pin Enable bit (MCLR pin enabled; RA5 input pin disabled)

// FICD
#pragma config ICS = PGx2               // ICD Pin Placement Select bits (PGC2/PGD2 are used for programming and debugging the device)

// FDS
#pragma config DSWDTPS = DSWDTPSF       // Deep Sleep Watchdog Timer Postscale Select bits (1:2,147,483,648 (25.7 Days))
#pragma config DSWDTOSC = LPRC          // DSWDT Reference Clock Select bit (DSWDT uses LPRC as reference clock)
#pragma config RTCOSC = SOSC            // RTCC Reference Clock Select bit (RTCC uses SOSC as reference clock)
#pragma config DSBOREN = ON             // Deep Sleep Zero-Power BOR Enable bit (Deep Sleep BOR enabled in Deep Sleep)
#pragma config DSWDTEN = ON             // Deep Sleep Watchdog Timer Enable bit (DSWDT enabled)

// #pragma config statements should precede project file includes.

#include <xc.h>
#include "IOs.h"
#include "TIMERs.h"
#include "ADC.h"
#include "clkChange.h"
#include "UART2.h"

/******globals******/
typedef enum {
    MODE0,
    MODE1_waiting,
    MODE1_sending
}state_t;

state_t prog_st;
uint8_t CN_event;
uint8_t TIMER2_event;

void run_mode0();
void run_mode1_waiting();
void run_mode1_sending();

/**
 * You might find it useful to add your own #defines to improve readability here
 */

int main(void) {
    
    /** This is usually where you would add run-once code
     * e.g., peripheral initialization. For the first labs
     * you might be fine just having it here. For more complex
     * projects, you might consider having one or more initialize() functions
     */
    
    AD1PCFG = 0xFFBF; /* keep this line as it sets I/O pins that can also be analog to be digital */
    
    newClk(500);

    /* Lets set up some IOs */
    IOinit();
    
    /* Let's set up TIMERs */
    TIMERinit();
    
    /* Let's set up our UART */    
    InitUART2();
  
    prog_st = MODE0;
    
    while(1) {
        switch(prog_st){
            case MODE0:
                break;
            case MODE1_waiting:
                break;
            case MODE1_sending:
                break;
        }       
    }
    
    return 0;
}

void run_mode0(){
    uint16_t prev_adc = 0;
    uint16_t adc_val;
    while(1){
        if(CN_event) break; // break to handle button presses
        
        // get the value of the ADC port 
        adc_val = do_adc();
        
        if(adc_val != prev_adc){
            prev_adc = adc_val;
            
            // calculate the number of characters we want to add based on this value
            // we want 1-17 characters so divide the number by 4096 (or left shift by 12)
            // and then add one
            uint8_t num_chars = (adc_val >> 12) + 1;

            // print out the value via uart
            Disp2String("\rMode 0: ");
            XmitUART2('*', num_chars);
            XmitUART2(' ', 1);
            Disp2Hex(adc_val);
        }
    }
    if(IOCheck() == 0b10) prog_st = MODE1_waiting;
}

void run_mode1_waiting(){
    while(!CN_event) Idle(); // wait for button to switch modes or start sending data
    switch(IOCheck()){
        case 0b10: // pb1, switch modes
            prog_st = MODE0;
            break;
        case 0b01: // pb2, begin sending data
            prog_st = MODE1_sending;
            break;
        default:
            return;
    }
}

void run_mode1_sending(){
    // send a sync message with the sample period
    Disp2String("Syncing - sample period (ms): 100\n");
   
    uint16_t adc_val;
    while(!CN_event){ // send data until CN event
        // start a timer to go off every sample period ms
        
        
        adc_val = do_adc();
        
        Disp2Dec(adc_val);
        XmitUART2(',', 1);
        
        Idle(); // idle until the timer goes off for our next sample rate 
    }
    T3CONbits.TON = 0; // turn off the timer
    switch(IOCheck()){ // handle the IO
        case 0b10:
            prog_st = MODE0;
            break;
        case 0b01:
            prog_st = MODE1_waiting;
            break;
    }
}


// Timer 2 interrupt subroutine
void __attribute__((interrupt, no_auto_psv)) _T2Interrupt(void){
    //Don't forget to clear the timer 2 interrupt flag!
    IFS0bits.T2IF = 0;
}

void __attribute__((interrupt, no_auto_psv)) _T3Interrupt(void){
    //Don't forget to clear the timer 3 interrupt flag!
    IFS0bits.T3IF = 0;
    
    // turn the timer back on
    TMR3 = 0;
    T3CONbits.TON = 1;
}

void __attribute__((interrupt, no_auto_psv)) _CNInterrupt(void){
    //Don't forget to clear the CN interrupt flag!
    IFS1bits.CNIF = 0;
}

