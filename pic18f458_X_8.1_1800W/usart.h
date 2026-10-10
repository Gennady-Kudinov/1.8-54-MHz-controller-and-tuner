//*********************************************************************************
void RXstartTIMER(void) 
{
	TMR2 = 0;
	T2CON = 0b01111111;//~5 ms 
	PIR1bits.TMR2IF = 0;
	PIE1bits.TMR2IE = 1;
}
//*********************************************************************************
void RXstopTIMER(void)
{
	T2CON = 0;
	PIE1bits.TMR2IE = 0;
	//PIE1bits.RCIE = 0;
}
//*********************************************************************************
void RXrestart(void)
{
	RXstopTIMER();
	for (rxb = 0; rxb < (_MAX_USART_RX-1); rxb++) buffRX[rxb] = 0x00;
	rx_end = 0; rxb = 0;
	//PIE1bits.RCIE = 1;
}
//*********************************************************************************
void RecieveError(void)
{
    //Overrun error bit
    if (RCSTAbits.OERR)        
    {                
    	RCSTAbits.CREN = 0;//Restart USART
        RCSTAbits.CREN = 1;   
    }               
    //If error ...
    if (RCSTAbits.FERR)
    {
        RCREG = RCREG;//Flush buffer
        RCREG = RCREG;
        RCREG = RCREG;            
    }
}
//*********************************************************************************
//*********************************************************************************
//*********************************************************************************
//interrupt vector
void interrupt isr(void)
{
	UCHAR rx;	
  	//------------------------ RECEIVE --------------------------
  	if ((PIR1bits.RCIF) && (PIE1bits.RCIE))
  	{
        RecieveError();
        rx = RCREG;
        //start timer
        if (rxb == 0) RXstartTIMER();
		//
	    buffRX[rxb] = rx;
	    //end of message
        if (rxb >= 2)
        {
            if ((buffRX[rxb] == 0xEE) && (buffRX[rxb-1] == 0xEE) && (buffRX[rxb-2] == 0xEE))
            {
                RXstopTIMER();
                rx_end = 1;
            }
        }
	    if (++rxb >= _MAX_USART_RX) RXrestart();
	}
	//------------------------ TIMER2 ---------------------------
	if ((PIE1bits.TMR2IE) && (PIR1bits.TMR2IF))
	{
		RXrestart();
	}
 	//------------------------ TRANSMITT ------------------------
  	if ((PIR1bits.TXIF) && (PIE1bits.TXIE))
  	{	  	
	  	if (txb <= tx_len) TXREG = buffTX[txb++];
		else PIE1bits.TXIE = 0;//stop
	}	
}
//*********************************************************************************
