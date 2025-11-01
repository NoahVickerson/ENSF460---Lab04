/*
 * File:   ADC.c
 * Author: noahvickerson
 *
 * Created on October 20, 2025, 3:32 PM
 */


#include "xc.h"
#include "ADC.h"

uint16_t do_adc(void)
{
    uint16_t ADCvalue;   // 10-bit result comes back in ADC1BUF0

    /* ------------- ADC INITIALIZATION ------------------*/
    AD1CON1bits.ADON = 0;          // ensure module OFF during config

    AD1PCFG = 0xFFFF;              // all digital first
    AD1PCFGbits.PCFG4 = 0;         // AN4 analog (pin 7)
    TRISAbits.TRISA2 = 1;          // RA2 input

    // AD1CON1: sample/convert control (SW-triggered, auto-convert)
    AD1CON1bits.FORM  = 0b00;      // integer, right-justified
    AD1CON1bits.SSRC  = 0b111;     // auto-convert after SAMC*TAD  (slides ?SW triggered?) 
    AD1CON1bits.ASAM  = 0;         // manual start of sampling

    // AD1CON2: Vref, buffer, scan
    AD1CON2bits.VCFG  = 0b000;     // AVDD/AVSS as Vref (per slide) 
    AD1CON2bits.CSCNA = 0;         // no scan
    AD1CON2bits.SMPI  = 0;         // flag each sample (we'll poll DONE)
    AD1CON2bits.BUFM  = 0;         // one 16-word buffer
    AD1CON2bits.ALTS  = 0;         // use MUX A

    // AD1CON3: timing
    AD1CON3bits.ADRC  = 0;         // use system clock (slide) 
    AD1CON3bits.SAMC  = 16;        // auto-sample time (TAD units) (slide on sample time) 
    AD1CON3bits.ADCS  = 10;        // TAD = (ADCS+1)*Tcy (safe default for our clocks)

    // Channel select: CH0 +input = AN5, -input = Vref-
    AD1CHSbits.CH0NA  = 0;         // Vref-
    AD1CHSbits.CH0SA  = 4;         // AN4 (pin 7)  (slide shows AN5 path) :contentReference[oaicite:4]{index=4}

    /* ------------- ADC SAMPLING AND CONVERSION ------------------*/
    AD1CON1bits.ADON = 1;          // **turn ON** ADC module (note: slides show this step) 
    AD1CON1bits.SAMP = 1;          // start sampling (auto-convert after SAMC)
    

    while (AD1CON1bits.DONE == 0)  // poll until conversion complete (slide)
    {}

    ADCvalue = ADC1BUF0 & 0x03FF;  // 10-bit ADC result
    AD1CON1bits.SAMP = 0;          // stop sampling
    AD1CON1bits.ADON = 0;          // power down between reads (matches slide?s flow) 

    return ADCvalue;
}
