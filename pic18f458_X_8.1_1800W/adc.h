//********************************************************************************
// TAD = 1/Fosc*div
UINT ADC(UCHAR ch)
{
	ADCON0 = 0b10000001 | (ch << 3);// TAD = 1.6uS
	//Tacq >= 4*TAD 
	__delay_us(7);
	ADCON0bits.GODONE = 1;
	while(ADCON0bits.GODONE) continue; //11*TAD
	//
	return (((UINT)ADRESH) << 8) | ((UINT)ADRESL);
}
//******************************************************************************** 
void ADCmeasure(void)
{
    //one time values
	frwd = ADC(1);		//forward voltage
	refl = ADC(0);		//reflected voltage
	curr = ADC(4);      //current
   	volt = ADC(5);	    //supply voltage
    //adc noise filter
    if (frwd < 10) frwd = 0;
    if (refl < 10) refl = 0;
    if (curr < 10) curr = 0; 
    if (volt < 10) volt = 0;
    //peak values
    if (frwd > fpeak) fpeak = frwd;
    if (refl > rpeak) rpeak = refl;
    if (curr > cpeak) cpeak = curr;
}	
//********************************************************************************
void SlowControl(void)
{
    if (!PIR2bits.TMR3IF) return;
    WRITETIMER3(0x0BFF);//50ms
    PIR2bits.TMR3IF = 0;
    //
    PttHoldControl();
    FanControl();
    //
   	voltM += volt;   //voltage for view
   	tempM += ADC(3); //temperature
    //
    if (++med & 0x10)
    {
      //temperature
      temp = tempM >> 4;
      if (temp >= (560 - sensor)) temp = (temp - (560 - sensor)) >> 1;
      else temp = 0;
      //voltage
      volt_view = voltM >> 4;
      //
      med = 0; tempM = 0; voltM = 0;
    }
}
//********************************************************************************
