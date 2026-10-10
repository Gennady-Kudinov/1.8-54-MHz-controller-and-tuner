#ifndef _ADC_H
#define _ADC_H

#include "xc.h"

// TAD = 1 / Fosc * div
UINT ADC(UCHAR ch)
{
    ADCON0bits.CHS = ch;   // Select ADC channel
    ADCON0bits.ADON = 1;   // Enable ADC module
    __delay_us(10);        // Acquisition delay (Tacq)
    ADCON0bits.GO = 1;     // Start conversion
    while(ADCON0bits.GO) continue;
    
    // Result in 10-bit mode (right justified)
    return (((UINT)ADRESH) << 8) | ((UINT)ADRESL);
}

void ADCmeasure(void)
{
    // Single measurements
    frwd = ADC(1); // Forward voltage
    refl = ADC(0); // Reflected voltage
    curr = ADC(4); // Current
    volt = ADC(5); // Supply voltage

    // ADC noise filter
    if (frwd < 10) frwd = 0;
    if (refl < 10) refl = 0;
    if (curr < 10) curr = 0; 
    if (volt < 10) volt = 0;

    // Peak values hold
    if (frwd > fpeak) fpeak = frwd;
    if (refl > rpeak) rpeak = refl;
    if (curr > cpeak) cpeak = curr;
}	

void SlowControl(void)
{
    if (!PIR2bits.TMR3IF) return;
    
    // Reset Timer3 for ~50ms period (0x0BFF)
    TMR3H = 0x0B;
    TMR3L = 0xFF;
    PIR2bits.TMR3IF = 0;

    PttHoldControl();
    FanControl();

    voltM += volt;   // Accumulated voltage for averaging
    tempM += ADC(3); // Temperature sensor

    if (++med & 0x10)
    {
        // Calculate average temperature
        temp = tempM >> 4;
        if (temp >= (560 - sensor)) temp = (temp - (560 - sensor)) >> 1;
        else temp = 0;

        // Calculate average voltage
        volt_view = voltM >> 4;

        med = 0; 
        tempM = 0; 
        voltM = 0;
    }
}

#endif