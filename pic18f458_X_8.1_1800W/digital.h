#ifndef _DIGITAL_H
#define _DIGITAL_H

#include "xc.h"
#include "stdlib.h"

void SetBand(void);
void SendUSART(void);

void BandDecode(void)
{
    if (freq < 200) band = 0;                          // 1.8 MHz
    else if ((freq >= 200) && (freq < 400)) band = 1;  // 3.5 MHz
    else if ((freq >= 400) && (freq < 1100)) band = 2; // 5-7-10 MHz
    else if ((freq >= 1100) && (freq < 1900)) band = 3;// 14-18 MHz
    else band = 4;                                     // 21-28 MHz

    SetBand();
}

UCHAR FreqMeasure(void)
{
    UCHAR jj, err;
    UINT  tmr;

    while(!_PTT_INPUT) // Measure while PTT is pressed
    {
        _WDT_RESET;
        if (PORTCbits.RC0) // Wait for rising edge
        {
            freq = 0; 
            err = 0;
            jj = 30; // ~3 ms total measure time
            
            INTCONbits.GIE = 0;
            while(jj--)
            {
                PIR1bits.TMR1IF = 0;
                TMR1H = 0;
                TMR1L = 0; // Reset counter
                
                __delay_us(100); // Sampling window
                tmr = (UINT)(TMR1L | ((UINT)TMR1H << 8)); // Read 16-bit Timer1
                
                if (abs((int)(freq - tmr)) >= 100) err++; // Check variation relative to max
                if (tmr > freq) freq = tmr;               // Keep peak value
            }
            INTCONbits.GIE = 1;

            if (err < 9) // Check error threshold (< 30%)
            {
                if (freq >= 100) // Valid frequency (> 1 MHz)
                {
                    BandDecode();
                    return(1);
                }
            }
        }
    }
    return(0);
}

void DelayS(UCHAR sec)
{
    sec *= 10;
    while(sec--)
    {
        _WDT_RESET;
        __delay_ms(100);
    }
}

void DelayMs(UCHAR ms)
{
    while(ms--)
    {
        _WDT_RESET;
        __delay_ms(1);
    }
}

void Beep(void)
{
    if (!beeper) return;
    _BEEPER = 1; DelayMs(100);
    _BEEPER = 0; DelayMs(100);
}

void SwitchOFF(void)
{
    _BIAS = 0;          // Turn off bias
    _POWER_VDS = 0;      // Turn off VDS
    _RELAY_INPUT = 0;   // Turn off input relay
    _RELAY_OUTPUT = 0;  // Turn off output relay
    ptt = 0;
}

void PTT_ON(void)
{
    if (ptt == 1) return;
    ptt = 1;

    // Sequencer: connect RF relays first
    _RELAY_OUTPUT = 1;
    _RELAY_INPUT = 1;

    // Frequency measurement in auto-band mode
    if (aband) 
    {
        DelayMs(reldel); // Delay for bypass relays
        if (!FreqMeasure()) return;
    }
    DelayMs(biasdel);    // Delay for LPF relays settling
    _BIAS = 1;           // Turn on transistor bias
}

void PTT_OFF(void)
{
    if (ptt == 0) return;
    ptt = 0;

    // Sequencer: remove bias first
    _BIAS = 0;
    _RELAY_INPUT = 0;
    DelayMs(reldel);     // Delay between input and output bypass relays
    _RELAY_OUTPUT = 0;			
}	

void PTT_read(void)
{
    if (bypass) return;
    
    if (!_PTT_INPUT) PTT_ON();
    else PTT_OFF();
}

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

void FanControl(void)
{
    _WDT_RESET;

    // Emergency cooling mode
    if (err & _ERROR_TMP)
    {
        _FAN_SPEED = 1; 
        fan = 1;
        if (temp <= (max_temp - hystT)) err &=~ _ERROR_TMP;
        return;
    }    			

    // Fan high-speed threshold
    if (temp >= max_tFAN) 
    {
        _FAN_SPEED = 1; 
        fan = 1;
        maxf = 1;
        return;
    }
    else 
    {
        // Return to normal speed
        if (temp <= (max_tFAN - hystF))
        {
            _FAN_SPEED = 0; 
            fan = 0;
            maxf = 0;
        }
    }

    // Force fan high speed during PTT transmit
    if (fanptt && (maxf == 0))
    {
        if (ptt) { _FAN_SPEED = 1; fan = 1; }
        else { _FAN_SPEED = 0; fan = 0; }
    }
}

void PttHoldControl(void)
{
    static UCHAR t = 0;

    if (ptt_hold == 0) return; // Protection disabled
    if ((ptt == 0) || (fpeak < _MIN_FRWD_HOLD))
    {
        ptt_htimer = 0; 
        t = 0; // Reset timer
        return;
    }
    
    // Increment timer every second (20 * 50ms)
    if (++t < 20) return;
    t = 0;
    ptt_htimer++;
}

#define _BEEP_NORMAL 2500
#define _BEEP_LONG   (_BEEP_NORMAL * 10)

void ProtectCheck(void)
{
    static UCHAR bp;
    static UINT  bpt, beepPer;

    _WDT_RESET;

    // Power, SWR, current limits check
    if (frwd >= max_pwr) err |= _ERROR_PWR;
    if (refl >= max_ref) err |= _ERROR_REF;
    if (curr >= max_curr) err |= _ERROR_CUR;

    // LPF fault detection
    if ((curr >= lpferrC) && (frwd <= lpferrP))
    {
        if (++elpf >= lpferrR) err |= _ERROR_LPF;
    }
    else elpf = 0;

    // Temperature and voltage limits check
    if (temp >= max_temp) err |= _ERROR_TMP;
    if (volt >= max_volt) err |= _ERROR_VOL;

    // Continuous PTT transmission time limit check
    if (ptt_hold)
    {
        if (ptt_htimer >= ptt_hold) err |= _ERROR_PTT;
    }

    // Handle any active errors
    if (err)
    {
        SwitchOFF();
        
        if (!beeper) return;
        if (err & _ERROR_TMP) beepPer = _BEEP_LONG;
        else beepPer = _BEEP_NORMAL;
        
        if (bp == 0) 
        {
            _BEEPER = 1;
            if (++bpt >= _BEEP_NORMAL) { bp = 1; bpt = 0; }	
        }
        else 
        {
            _BEEPER = 0;
            if (++bpt >= beepPer) { bp = 0; bpt = 0; }	
        }
    }
    else
    {
        _POWER_VDS = 1;
        PTT_read();
        
        _BEEPER = 0; 
        bp = 0; 
        bpt = 0;
    }
}

#endif