//*********************************************************************************
void SetupMode(void);
void WaitLCDmainPage(void);
UCHAR LCDpageSelect(UCHAR page);
//*********************************************************************************
void ConvertNumber(ULONG number, UCHAR *conv)
{
	conv[0] = 0x30;
	conv[1] = 0x30;
	conv[2] = 0x30;
	conv[3] = 0x30;
	conv[4] = 0x30;
	conv[5] = 0x30;
    //
    while(number > 99999) {number -= 100000; conv[0]++;}
    while(number > 9999) {number -= 10000; conv[1]++;}
	while(number > 999) {number -= 1000; conv[2]++;}
	while(number > 99) {number -= 100; conv[3]++;}
	while(number > 9) {number -= 10; conv[4]++;}
	conv[5] += number;
}
//*********************************************************************************
//create buffer for send to lcd
void CreateBuffTX(const char *str, ULONG data)
{
	for(tx_len = 0; str[tx_len] != '\0'; tx_len++) buffTX[tx_len] = str[tx_len];
	//value
	if (data != 0xFFFFFF)
	{
		buffTX[tx_len++] = '=';
		ConvertNumber(data, (UCHAR *)&buffTX[tx_len]);
		tx_len += 6;
	}
	//end message
	buffTX[tx_len++] = 0xFF;
	buffTX[tx_len++] = 0xFF;
	buffTX[tx_len] = 0xFF;
}
//*********************************************************************************
void Tramsmitt(void)
{
	//start transmitt in interrupts
    TMR0 = 0x2800; //~10 ms
	INTCONbits.TMR0IF = 0;
	txb = 0; PIE1bits.TXIE = 1;
}
//*********************************************************************************
#define _TIME_REPEAT    1000 //~85 ms
//
void SendStatusToLCD(void)
{
    _WDT_RESET;
    //to next queuing data
	if (lcd < 3) lcd++;
	else lcd = 0;
    //discret timers decrease
    if (frtout) frtout--;
    if (cvtout) cvtout--;
    //wait end transmitt and timeout for next message
	if ((PIE1bits.TXIE) || (!INTCONbits.TMR0IF)) return;
    //queuing all data for lcd --------------------------------
	//forward and reflected voltage value
    if (lcd == 0)
    {
        if (frtout == 0) //do not send next new value often than minimum 50 ms
        {
            //do not send if value does not change
            if ((fpeak != fpeak_old) || (rpeak != rpeak_old))
            {
                tmp = ((ULONG)fpeak) | (((ULONG)rpeak) << 10);
                CreateBuffTX("frV", tmp);
                fpeak_old = fpeak;
                rpeak_old = rpeak;              
                frtout = _TIME_REPEAT; //refresh timer
                Tramsmitt(); return;
            }
            else 
            {
                if (frwd < fpeak) fpeak = frwd; //update (reset) peak
                if (refl < rpeak) rpeak = refl;   
            }
        }
    }
    //vds and current voltage value
    if (lcd == 1)
    {
        if (cvtout == 0) //do not send next new value often than minimum 50 ms
        {
            //do not send if value does not change
            if ((cpeak != cpeak_old) || (volt_view != volt_old))
            {
                tmp = ((ULONG)volt_view) | (((ULONG)cpeak) << 10);
                CreateBuffTX("cvV", tmp);
                cpeak_old = cpeak;
                volt_old = volt_view;
                cvtout = _TIME_REPEAT; //refresh timer
                Tramsmitt(); return;
            }
            else if (curr < cpeak) cpeak = curr; //update (reset) peak
        }
    }
    //temperature voltage value, bands, buttons/fan state
    if (lcd == 2)
    {
        bstate = (fan << 3) | (bypass << 2) | (ptt << 1) | aband;
        if ((temp != temp_old) || (band != band_old) || (bstate != bstate_old))
        {
            tmp = ((ULONG)temp) | (((ULONG)band) << 7) | (((ULONG)bstate) << 10);
            CreateBuffTX("tbs", tmp);
            temp_old = temp;
            band_old = band;
            bstate_old = bstate;
            Tramsmitt(); return;
        }
    }
    //errors
    if (lcd == 3)
    {
        if (err != err_old)
        {
            CreateBuffTX("err", err);
            err_old = err; 
            Tramsmitt(); return;
        }
    }
}
//*********************************************************************************
//*********************************************************************************
//*********************************************************************************
void ReadStatusFromLCD(void)
{
    _WDT_RESET;
	if (rx_end == 0) return;
	if (buffRX[0] != 0x90) {RXrestart(); return;}
	//
	if (err) //any errors present
	{
		if (!(err & _ERROR_TMP)) //not cooling 
		{
			//reset error
			if (buffRX[1] == 0x12) err &=~ buffRX[2];
		}	
	}
	else //no errors
	{
		//bypass button
		if (buffRX[1] == 0x11) 
		{
			bypass ^= 1;
            writeEEPROM(_EE_BYPASS, bypass);
			if (bypass) PTT_OFF();  //disable PTT
		}
		//no operate needed
		if (ptt == 0)
		{
			if (bypass == 0)
			{
				//auto band button
				if (buffRX[1] == 0x13) 
				{
					aband ^= 1;
					writeEEPROM(_EE_ABAND, aband);
					if (aband == 0) 
                    {
                        EEmbandRead();//restore manual band
                        SetBand();
                    }
				}
				//band buttons
				if (buffRX[1] <= 0x04)
				{
					aband = 0;
					band = buffRX[1];	
					SetBand(); 
					writeEEPROM(_EE_BAND, band);
					writeEEPROM(_EE_ABAND, aband);
				}
			}
			//setup
			if (buffRX[1] == 0x20)
            {
                SetupMode();
                EEsetupRead();//update all values
                LCDpageSelect(_PAGE_MAIN);//to main page
                Beep();
            }
		}		
	}
	RXrestart();
}
//*********************************************************************************
//*********************************************************************************
//*********************************************************************************
void SendUSART(void)
{
    txb = 0; PIE1bits.TXIE = 1;
	while(PIE1bits.TXIE) _WDT_RESET;
    DelayMs(5);
}
//*********************************************************************************
void SetupMode(void)
{
	UINT  pW,rW,lW;
    UCHAR eadr;
    //
	SwitchOFF();//not possible any operate
	//convert to watt
	pW = (UINT)(((ULONG)max_pwr * (ULONG)max_pwr) / (ULONG)_PWR_COEFF) + 1;
	rW = (UINT)(((ULONG)max_ref * (ULONG)max_ref) / (ULONG)_PWR_COEFF) + 1;
	lW = (UINT)(((ULONG)lpferrP * (ULONG)lpferrP) / (ULONG)_PWR_COEFF) + 1;
	//send values
	CreateBuffTX("mfrw", pW); SendUSART();
	CreateBuffTX("mref", rW); SendUSART();
	CreateBuffTX("mcur", max_curr); SendUSART();
	CreateBuffTX("mvol", max_volt); SendUSART();
	CreateBuffTX("mlp", lW); SendUSART();
	CreateBuffTX("mlc", lpferrC); SendUSART();
    CreateBuffTX("mrt", lpferrR/10); SendUSART();
    CreateBuffTX("mhe", ptt_hold); SendUSART();
	CreateBuffTX("mtp", max_temp); SendUSART();
	CreateBuffTX("mtf", max_tFAN); SendUSART();
    CreateBuffTX("mtph", hystT); SendUSART();
    CreateBuffTX("mtfh", hystF); SendUSART();
	CreateBuffTX("mrd", reldel); SendUSART();
 	CreateBuffTX("mbd", biasdel); SendUSART();
    CreateBuffTX("mptt", fanptt); SendUSART();
    CreateBuffTX("mbp", beeper); SendUSART();
    CreateBuffTX("mts", sensor); SendUSART();
    CreateBuffTX("mlg", logo); SendUSART();
	//to setup page
	if (!LCDpageSelect(_PAGE_SETUP)) return;
    //
	RXrestart();
	while(1)
	{
        _WDT_RESET;
        if (rx_end)
		{
            //data for save
			if ((buffRX[0] >= 0xB0) && (buffRX[0] < 0xEE))
			{
                eadr = buffRX[0] - 0xB0;
                if (buffRX[1] == 0x1) writeEEPROM(eadr, buffRX[2]);
                if (buffRX[1] == 0x2) writeEEPROM16(eadr, (UCHAR *)&buffRX[2]);
			}
            //end data
            if ((buffRX[0] == 0xF0) && (buffRX[1] == 0xF0)) break;
			RXrestart();
		}
	}
}
//*********************************************************************************
//*********************************************************************************
UCHAR waitAckFromLCD(void)
{
    UCHAR ack = 0;
    //
    RXrestart();
    TMR0 = 0;
    T0CON = 0b10000101; //~0.45 s
    INTCONbits.TMR0IF = 0;
    //wait
    while(!INTCONbits.TMR0IF)
    {
        _WDT_RESET;
        if (rx_end)
        {
            if ((buffRX[1] == 0x55) && (buffRX[2] == 0x77))
            {
                if ((buffRX[0] >= 0xA0) && (buffRX[0] <= 0xA2))
                {
                    ack = buffRX[0];
                    break;
                }
            }
            RXrestart();
        }
    }
    T0CON = 0b10000000;
    return(ack);
}
//*********************************************************************************
UCHAR LCDpageSelect(UCHAR page)
{
    UCHAR tt = 6;//~3 sec ack wait
    //
    while (tt--)
    {
        if (page == _PAGE_MAIN)  CreateBuffTX("page main",0xFFFFFF);
        if (page == _PAGE_LOGO)  CreateBuffTX("page logo",0xFFFFFF);
        if (page == _PAGE_SETUP) CreateBuffTX("page setup1",0xFFFFFF);
        SendUSART();
        //
        if (waitAckFromLCD() == page) return(1);
    }
    return(0);
}
//*********************************************************************************
void LCDversionSend(void)
{
    UCHAR rep = 3;
    while(rep--)
    {
        CreateBuffTX("version",_VERSION);  SendUSART();
    }
}
//*********************************************************************************
