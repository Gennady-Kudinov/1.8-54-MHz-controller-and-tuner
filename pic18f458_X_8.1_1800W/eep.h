//****************************************************************************
UCHAR readEEPROM(UCHAR addr)
{
    EECON1 = 0;
    EEADR = addr;
    EECON1bits.RD = 1;
    NOP();
    NOP();
    NOP();
    NOP();
    return EEDATA;
}
//****************************************************************************
void writeEEPROM(UCHAR address, UCHAR data)
{
    UCHAR INTCON_SAVE;
    //
    EEADR = address;
    EEDATA = data; 
    EECON1bits.EEPGD = 0; 
    EECON1bits.WREN = 1; 
    INTCON_SAVE = INTCON;
    INTCON = 0;
    EECON2 = 0x55;
    EECON2 = 0xAA;
    EECON1bits.WR = 1;
    while (EECON1bits.WR) _WDT_RESET;
    EECON1bits.WREN = 0; 
    INTCON = INTCON_SAVE;
}
//****************************************************************************
UINT readEEPROM16(UCHAR addr)
{
    UINT ee;
    ee = (UINT)(readEEPROM(addr)) | ((UINT)(readEEPROM(addr+1)) << 8);
    return(ee);
}
//****************************************************************************
void writeEEPROM16(UINT addr, UCHAR *data)
{
	writeEEPROM(addr+0, data[0]);
	writeEEPROM(addr+1, data[1]);	
}
//****************************************************************************
//****************************************************************************
//restore all settings from eeprom
void EEsetupRead(void)
{
	UINT pW,rW,lW;
	//
	pW = readEEPROM16(_EE_PWR)&0x7FF; //max forward voltage
	max_pwr = (UINT)sqrt((ULONG)pW * (ULONG)_PWR_COEFF); //convert to voltage
	rW = readEEPROM16(_EE_REF)&0x1FF; //max reflected voltage
	max_ref = (UINT)sqrt((ULONG)rW * (ULONG)_PWR_COEFF); //convert to voltage
	//
	max_curr = readEEPROM16(_EE_CUR)&0x3FF; //max current
	max_volt = readEEPROM16(_EE_VOL)&0x3FF; //max voltage
	max_temp = readEEPROM16(_EE_TMP)&0x7F; //max temperature
	max_tFAN = readEEPROM16(_EE_TFN)&0x7F; //temperature for max fan speed
    //
	lW = readEEPROM16(_EE_LPF_P)&0x3FF; //lpr error min power
    lpferrP = (UINT)sqrt((ULONG)lW * (ULONG)_PWR_COEFF); //convert to voltage
    lpferrC = readEEPROM16(_EE_LPF_C)&0x3FF; //lpf error max current
    //
    lpferrR = (readEEPROM(_EE_LPF_R)&0x7F) * 10;//lpf error reaction time
    ptt_hold = readEEPROM(_EE_HOLD)&0x7F; //ptt hold max time
    hystT = readEEPROM(_EE_HYSTT)&0x1F;  //max temp protect hysteresis
    hystF = readEEPROM(_EE_HYSTF)&0x1F;  //fan max speed hysteresis
    reldel = readEEPROM(_EE_RDEL)&0x7F;  //delay between input/ouput relays
    biasdel = readEEPROM(_EE_BDEL)&0x7F; //delay after all relay on
    sensor = readEEPROM(_EE_SENS)&0x1F;  //temperature sensor calibration value
    fanptt = readEEPROM(_EE_FPTT)&0x01;  //fan mode in ptt
    beeper = readEEPROM(_EE_BEEP)&0x01;  //beeper mode
    logo = readEEPROM(_EE_LOGO)&0x01;    //logo view
}
//****************************************************************************
void EEmbandRead(void)
{
	band = readEEPROM(_EE_BAND)&0x0F;   //last band for manual select mode
	aband = readEEPROM(_EE_ABAND)&0x01; //mode auto/manual
    bypass = readEEPROM(_EE_BYPASS)&0x01; //bypass state
}
//****************************************************************************
