//*********************************************************************************
void SetBand(void);
void SendUSART(void);
//*********************************************************************************
void BandDecode(void)
{
    if (freq < 200) band = 0;//1.8
    else
    if ((freq >= 200) && (freq < 400)) band = 1;//3.5
    else
    if ((freq >= 400) && (freq < 1100)) band = 2;//5-7-10
    else
    if ((freq >= 1100) && (freq < 1900)) band = 3;//14-18
    else band = 4;//21-28
	//
	SetBand();
}
//*********************************************************************************
UCHAR FreqMeasure(void)
{
    UCHAR jj, err;
    UINT  tmr;
    //
    while(!_PTT_INPUT)//try to measure while PTT present
    {
        _WDT_RESET;
        if (PORTCbits.RC0)//wait rising
        {
            freq = 0; err = 0;
            jj = 30; // ~3 ms - overal measure time
            //
            INTCONbits.GIE = 0;
            while(jj--)
            {
                TMR1H = 0;
                PIR1bits.TMR1IF = 0;
                TMR1L = 0;//reset counter
                //
                __delay_us(100);//measure time
                tmr = READTIMER1();//read counter value
                //
                if (fabs(freq - tmr) >= 100) err++;//check differences relatively maximum
                if (tmr > freq) freq = tmr;//save maximum value only
            }
            INTCONbits.GIE = 1;
            if (err < 9)//check errors amount (less than 30%)
            {
                //set band
                if (freq >= 100)//valid freq and more than 1 MHz
                {
                    BandDecode();
                    return(1);
                }
            }//lot of errors - repeat measure
        }
    }
    return(0);
}
//*********************************************************************************
void DelayS(UCHAR sec)
{
    sec *= 10;
    while(sec--)
    {
        _WDT_RESET;
        __delay_ms(100);
    }
}
//*********************************************************************************
void DelayMs(UCHAR ms)
{
    while(ms--)
    {
        _WDT_RESET;
        __delay_ms(1);
    }
}
//*********************************************************************************
void Beep(void)
{
    if (!beeper) return;
    _BEEPER = 1; DelayMs(100);
	_BEEPER = 0; DelayMs(100);
}
//*********************************************************************************
void SwitchOFF(void)
{
	_BIAS = 0; //off bias
	_POWER_VDS = 0; //off vds
	_RELAY_INPUT = 0; //off input relay
	_RELAY_OUTPUT = 0;//off output relay
	//
	ptt = 0;
}
//*********************************************************************************
void PTT_ON(void)
{
	if (ptt == 1) return;
	ptt = 1;
	//ptt on sequencer
	_RELAY_OUTPUT = 1;
	_RELAY_INPUT = 1;
	//freq meas
    if (aband) 
    {
        DelayMs(reldel);//delay for bypass relays
        if (!FreqMeasure()) return;
    }
	DelayMs(biasdel);//delay for LPF relays
	_BIAS = 1;
}
//*********************************************************************************
void PTT_OFF(void)
{
	if (ptt == 0) return;
	ptt = 0;
	//ptt off sequencer
	_BIAS = 0;
	_RELAY_INPUT = 0;
	DelayMs(reldel);//delay between input and output bypass relays
	_RELAY_OUTPUT = 0;			
}	
//*********************************************************************************
//scan ptt input
void PTT_read(void)
{
	if (bypass) return;
	//
	if (!_PTT_INPUT) PTT_ON();
	else PTT_OFF();
}
//*********************************************************************************
void SetBand(void)
{
	if (band == 1) _BAND_35 = 1;
	else _BAND_35 = 0;
	if (band == 2) _BAND_710 = 1;
	else _BAND_710 = 0;
	if (band == 3) _BAND_1418 = 1;
	else _BAND_1418 = 0;
	if (band == 4) _BAND_2128 = 1;
	else _BAND_2128 = 0;
}
//*********************************************************************************
void FanControl(void)
{
    _WDT_RESET;
    //cooling
	if (err & _ERROR_TMP)
	{
		_FAN_SPEED = 1; fan = 1;
		if (temp <= (max_temp - hystT)) err &=~ _ERROR_TMP;
		return;
	}    			
	//fan max speed
	if (temp >= max_tFAN) 
    {
        _FAN_SPEED = 1; fan = 1;
        maxf = 1;
        return;
    }
	else 
	{
        //return to slow speed
		if (temp <= (max_tFAN - hystF))
        {
            _FAN_SPEED = 0; fan = 0;
            maxf = 0;
        }
    }
    //max fan speed on ptt
    if (fanptt && (maxf == 0))
    {
        if (ptt) {_FAN_SPEED = 1; fan = 1;}
        else {_FAN_SPEED = 0; fan = 0;}
	}
}
//*********************************************************************************
void PttHoldControl(void)
{
    static UCHAR t = 0;
    //
    if (ptt_hold == 0) return; //ptt protect off
    if ((ptt == 0) || (fpeak < _MIN_FRWD_HOLD))
    {
        ptt_htimer = 0; t = 0; //reset hold timer
        return;
    }
    //start counter
    if (++t < 20) return; //1 sec
    t = 0;
    ptt_htimer++;
}
//*********************************************************************************
//*********************************************************************************
#define _BEEP_NORMAL	2500
#define _BEEP_LONG		_BEEP_NORMAL*10
//
void ProtectCheck(void)
{
	static UCHAR bp;
	static UINT  bpt, beepPer;
	//
    _WDT_RESET;
    //
	//forward
	if (frwd >= max_pwr) err |= _ERROR_PWR;
	//reflected
	if (refl >= max_ref) err |= _ERROR_REF;
	//current
	if (curr >= max_curr) err |= _ERROR_CUR;
	//lpf
	if ((curr >= lpferrC) && (frwd <= lpferrP))
    {
        if (++elpf >= lpferrR) err |= _ERROR_LPF;
    }
    else elpf = 0;
	//temperature
	if (temp >= max_temp) err |= _ERROR_TMP;
	//voltage
	if (volt >= max_volt) err |= _ERROR_VOL;
    //ptt hold
    if (ptt_hold)
    {
        if (ptt_htimer >= ptt_hold) err |= _ERROR_PTT;
    }
	//any errors
    if (err) //--------------------------------------------
	{
        SwitchOFF();
        //beeper
        if (!beeper) return;
        if (err & _ERROR_TMP) beepPer = _BEEP_LONG;//long pause beep
		else beepPer = _BEEP_NORMAL;//normal beep
		//
		if (bp == 0) 
		{
			_BEEPER = 1;
			if (++bpt >= _BEEP_NORMAL) {bp = 1; bpt = 0;}	
		}
		else 
		{
			_BEEPER = 0;
			if (++bpt >= beepPer) {bp = 0; bpt = 0;}	
		}
	}
    else
    {
        _POWER_VDS = 1;
        PTT_read();
        //
        _BEEPER = 0; bp = 0; bpt = 0; //beeper off
    }
}
//*********************************************************************************
