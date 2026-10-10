#ifndef _EEP_H
#define _EEP_H

#include "xc.h"

UCHAR readEEPROM(UCHAR addr)
{
    EEADR = addr;
    EECON1bits.EEPGD = 0; // Access data EEPROM
    EECON1bits.CFGS = 0;
    EECON1bits.RD = 1;    // Initiate read
    NOP();
    NOP();
    NOP();
    NOP();
    return EEDATA;
}

void writeEEPROM(UCHAR address, UCHAR data)
{
    UCHAR INTCON_SAVE;

    EEADR = address;
    EEDATA = data; 
    EECON1bits.EEPGD = 0; 
    EECON1bits.CFGS = 0;
    EECON1bits.WREN = 1;  // Enable writes
    
    INTCON_SAVE = INTCON;
    INTCONbits.GIE = 0;   // Disable interrupts during unlock sequence
    
    EECON2 = 0x55;        // Unlock sequence
    EECON2 = 0xAA;
    EECON1bits.WR = 1;    // Initiate write
    
    while (EECON1bits.WR) _WDT_RESET;
    EECON1bits.WREN = 0;  // Disable writes
    INTCON = INTCON_SAVE; // Restore interrupts
}

UINT readEEPROM16(UCHAR addr)
{
    UINT ee;
    ee = (UINT)(readEEPROM(addr)) | ((UINT)(readEEPROM(addr + 1)) << 8);
    return(ee);
}

void writeEEPROM16(UINT addr, UCHAR *data)
{
    writeEEPROM(addr + 0, data[0]);
    writeEEPROM(addr + 1, data[1]);	
}

// Restore all settings from EEPROM
void EEsetupRead(void)
{
    UINT pW, rW, lW;

    pW = readEEPROM16(_EE_PWR) & 0x7FF; // Max forward voltage
    max_pwr = (UINT)sqrt((ULONG)pW * (ULONG)_PWR_COEFF);

    rW = readEEPROM16(_EE_REF) & 0x1FF; // Max reflected voltage
    max_ref = (UINT)sqrt((ULONG)rW * (ULONG)_PWR_COEFF);

    max_curr = readEEPROM16(_EE_CUR) & 0x3FF; // Max current
    max_volt = readEEPROM16(_EE_VOL) & 0x3FF; // Max voltage
    max_temp = readEEPROM16(_EE_TMP) & 0x7F;  // Max temperature
    max_tFAN = readEEPROM16(_EE_TFN) & 0x7F;  // Fan max speed temperature

    lW = readEEPROM16(_EE_LPF_P) & 0x3FF;     // LPF error min power
    lpferrP = (UINT)sqrt((ULONG)lW * (ULONG)_PWR_COEFF);
    lpferrC = readEEPROM16(_EE_LPF_C) & 0x3FF;// LPF error max current

    lpferrR = (readEEPROM(_EE_LPF_R) & 0x7F) * 10; // LPF reaction time
    ptt_hold = readEEPROM(_EE_HOLD) & 0x7F;       // PTT hold max time
    hystT = readEEPROM(_EE_HYSTT) & 0x1F;         // Temperature hysteresis
    hystF = readEEPROM(_EE_HYSTF) & 0x1F;         // Fan hysteresis
    reldel = readEEPROM(_EE_RDEL) & 0x7F;         // Relay delay
    biasdel = readEEPROM(_EE_BDEL) & 0x7F;        // Bias delay
    sensor = readEEPROM(_EE_SENS) & 0x1F;         // Sensor calibration
    fanptt = readEEPROM(_EE_FPTT) & 0x01;         // Fan mode on PTT
    beeper = readEEPROM(_EE_BEEP) & 0x01;         // Beeper mode
    logo = readEEPROM(_EE_LOGO) & 0x01;           // Logo view
}

void EEmbandRead(void)
{
    band = readEEPROM(_EE_BAND) & 0x0F;     // Last manual band
    aband = readEEPROM(_EE_ABAND) & 0x01;   // Auto/manual mode
    bypass = readEEPROM(_EE_BYPASS) & 0x01; // Bypass state
}

#endif