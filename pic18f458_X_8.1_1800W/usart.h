#ifndef _USART_H
#define _USART_H

#include "xc.h"

void RXstartTIMER(void) 
{
    TMR2 = 0;
    T2CON = 0b01111111; // ~5 ms 
    PIR1bits.TMR2IF = 0;
    PIE1bits.TMR2IE = 1;
}

void RXstopTIMER(void)
{
    T2CON = 0;
    PIE1bits.TMR2IE = 0;
}

void RXrestart(void)
{
    RXstopTIMER();
    for (rxb = 0; rxb < (_MAX_USART_RX - 1); rxb++) buffRX[rxb] = 0x00;
    rx_end = 0; 
    rxb = 0;
}

void RecieveError(void)
{
    // Overrun error check on EUSART1
    if (RCSTA1bits.OERR)        
    {                
        RCSTA1bits.CREN = 0; // Restart receiver
        RCSTA1bits.CREN = 1;   
    }               
    // Framing error check on EUSART1
    if (RCSTA1bits.FERR)
    {
        volatile UCHAR dummy = RCREG1; // Flush buffer
        dummy = RCREG1;
        (void)dummy;
    }
}

// Global interrupt service routine (XC8 format)
void __interrupt() isr(void)
{
    UCHAR rx;	
    // ------------------------ RECEIVE (EUSART1) ------------------------
    if (PIR1bits.RC1IF && PIE1bits.RC1IE)
    {
        RecieveError();
        rx = RCREG1;
        
        // Start timeout timer
        if (rxb == 0) RXstartTIMER();
        
        buffRX[rxb] = rx;
        
        // Check end of message signature (three 0xEE bytes)
        if (rxb >= 2)
        {
            if ((buffRX[rxb] == 0xEE) && (buffRX[rxb - 1] == 0xEE) && (buffRX[rxb - 2] == 0xEE))
            {
                RXstopTIMER();
                rx_end = 1;
            }
        }
        if (++rxb >= _MAX_USART_RX) RXrestart();
    }

    // ------------------------ TIMER2 TIMEOUT ---------------------------
    if (PIE1bits.TMR2IE && PIR1bits.TMR2IF)
    {
        PIR1bits.TMR2IF = 0;
        RXrestart();
    }

    // ------------------------ TRANSMIT (EUSART1) -----------------------
    if (PIR1bits.TX1IF && PIE1bits.TX1IE)
    {	  	
        if (txb <= tx_len) 
        {
            TXREG1 = buffTX[txb++];
        }
        else 
        {
            PIE1bits.TX1IE = 0; // Transmission complete, disable interrupt
        }
    }	
}

#endif