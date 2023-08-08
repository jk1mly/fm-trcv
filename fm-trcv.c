/*
 * FM transceiver for PIC16F1503  PIC16F1705
 *
 *      JK1MLY:Hidekazu Inaba
 *
 *  (C)2021 JK1MLY All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <xc.h>
#include <pic.h>

#define _XTAL_FREQ 8000000
// CONFIG1
#pragma config FOSC = INTOSC    // Oscillator Selection
#pragma config WDTE = OFF       // Watchdog Timer
#pragma config PWRTE = OFF      // Power-up Timer
#pragma config MCLRE = OFF      // MCLR Pin Function Select
#pragma config CP = OFF         // Flash Program Memory Code Protection
#pragma config BOREN = ON       // Brown-out Reset Enable
#pragma config CLKOUTEN = OFF   // Clock Out Enable
// CONFIG2
#pragma config WRT = OFF        // Flash Memory Self-Write Protection
// #pragma config PLLEN = OFF      // 4x PLL OFF
#pragma config STVREN = OFF     // Stack Overflow/Underflow Reset Enable
#pragma config BORV = LO        // Brown-out Reset Voltage Selection
// #pragma config LPBOREN = OFF    // Low Power Brown-out Reset Enable
#pragma config LVP = OFF        // Low-Voltage Programming Enable

#define false       0
#define true        1

#define SDA_LOW     LATA0 = 0;TRISA0 = 0    // 0
#define SDA_HIGH    LATA0 = 1;TRISA0 = 1    // Z(1))
#define SCK_LOW     LATA1 = 0
#define SCK_HIGH    LATA1 = 1

#define I2C_ADR     0x90
#define I2C_LCD     0x7c

#define LED_ON      LATA5 = 1
#define LED_OFF     LATA5 = 0
//  #define BAND        RA2     // B4 refine
#define BAND        RA4
//  #define PTT         RA4     // B4 refine
#define PTT         RA2
#define SWON        0
#define RECV        0
#define SEND        1
#define S_VOL       2
#define S_SQL       3

#define BSW28       LATC1=1;LATC2=0;LATC3=0;LATC4=0;LATC5=0
#define BSW50       LATC1=0;LATC2=1;LATC3=0;LATC4=0;LATC5=0
#define BSW144      LATC1=0;LATC2=0;LATC3=1;LATC4=0;LATC5=0
//#define BSW144    LATC1=0;LATC2=1;LATC3=0;LATC4=0;LATC5=0
//#define BSW50     LATC1=0;LATC2=0;LATC3=1;LATC4=0;LATC5=0
//#define BSW430    LATC1=0;LATC2=0;LATC3=0;LATC4=1;LATC5=0
//#define BSWTUN    LATC1=0;LATC2=0;LATC3=0;LATC4=0;LATC5=1
#define BSW430      LATC1=0;LATC2=0;LATC3=0;LATC4=0;LATC5=1
#define BSWTUN      LATC1=0;LATC2=0;LATC3=0;LATC4=1;LATC5=0
#define BSWOFF      LATC1=0;LATC2=0;LATC3=0;LATC4=0;LATC5=0
#define RFAON       LATC0=1
#define RFAOFF      LATC0=0
#define REG_VOL     19
#define REG_SQL     22  //RSSI
//#define REG_SQL     23  //NOISE

#define FRQ_REG     frq_reg28a

#define PLL_28      0xC000
#define PLL_50      0x8000
#define PLL_144     0x2000
#define PLL_430     0x0000

/*
#define REF_OP      0b11010000
#define REF_CT      0b10010000

#define REF_LT      0b01110000
#define REF_RT      0b01010000
#define REF_DN      0b00110000
#define REF_UP      0b00010000
*/

#define REF_OP      0b11011000
#define REF_LT      0b10111000
#define REF_RT      0b10011000
#define REF_CT      0b01111000
#define REF_UP      0b00111000
#define REF_DN      0b00011000

#define STAT_OP     7
#define STAT_FN     6
#define STAT_CT     5
#define STAT_LT     4
#define STAT_RT     3
#define STAT_DN     2
#define STAT_UP     1
#define STAT_PT     0

// reference
// https://aitendo3.sakura.ne.jp/aitendo_data/product_img/ic/wirless/BK4802P/BK4802N-BEKEN.pdf
// http://pdf-html.ic37.com/pdf_file_U1/20200531/pdf_pdf/uploadpdf/ETC/BK4802_datasheet_1240666/202073/BK4802_datasheet.pdf
// https://github.com/BG7QKU/BK4802P-4BANDS-FM-TRANSCEIVER-WITH-BLUETOOTH-CONTROL-BASED-STC8F-MCU/blob/master/MCUMAIN
// https://github.com/BG7QKU/BK4802N-EEPROM-CALC-V0.1

void port_init(void) {
    /* CONFIGURE GPIO */
    OSCCON  = 0b01101000;    //4MHz
//  TRISA   = 0b00011101;    //Input(1)     // B4 refine
    TRISA   = 0b00011101;    //Input(1)
    OPTION_REG = 0b00000000; //MSB WPUENn
    WPUA    = 0b00011001;    //PupOn(1)
    INTCON  = 0b00000000;
    LATA    = 0b00000011;
//  ANSELA  = 0b00010000;    //ANSA RA4;AN3=DET     // B4 refine
    ANSELA  = 0b00000100;    //ANSA RA2;AN2=DET
//  ADCON0  = 0b00001101;    //0 CHS4:0 GO ON AN3   // B4 refine
    ADCON0  = 0b00001001;    //0 CHS4:0 GO ON AN2
    ADCON1  = 0b10000000;    //ADFM ADCS2:0 00 ADPREF1:0
                //ADCS 000=F/2 100=F/4 001=F/8 101=F/101
    ADCON2  = 0b00000000;
    TRISC   = 0b00000000;
    LATC    = 0b00000000;

    for(uint8_t i = 0; i < 3; i++){
        LED_ON;
        __delay_ms(100);
        LED_OFF;
        __delay_ms(100);
    }
}

void i2c_snd(uint8_t data)
{
    for(uint8_t i = 0; i < 8; i++){
        if ( (data & 0x80) ==0  ){
            SDA_LOW;
        }else{
            SDA_HIGH;
        }

        data = (uint8_t)(data << 1) ;
        __delay_us(2);
        SCK_HIGH;
        __delay_us(4);
        SCK_LOW;
        __delay_us(2);
    }

// ACK
    SDA_HIGH;
    __delay_us(8);
    SCK_HIGH;
    __delay_us(4);
    SCK_LOW;
//    __delay_us(2);
    SDA_LOW;
    __delay_us(20);
}

/*
 FREQ    RX              TX                      STEP
 29.00M 56ED B66A A000  5757 5757 A000
  + 02  56FD 21FF       5766 C2EB         F6B95
  + 64  58DB 28FB       5944 C9E8       1ED7291  F6B94   F6B94
 51.00M 4C97 FC56 4000  4CCC CCCC 4000
  + 02  4C9F B220       4CD4 8297         7B5CA
  + 64  4D8E B59F       4DC3 8615        F6B949  7B5CA   7B5CA
 52.00M 4E19 7DD7 4000  4E4E 4E4E 4000
  + 02  4E21 33A2       4E56 0418
  + 64  4F10 3720       4F45 0796        F6B949
144.00M 513D 8324 2000  5151 5151 2000
  + 02  5140 6750       5154 357D         2E42C
  + 64  519A 08A0       51AD D6CC        5C857C  2E42B   2E42B
145.00M 51CE 13B5 2000  51E1 E1E1 2000
  + 02  51D0 F7E1       51E4 C60D         2E42C
  + 64  522A 9930       523E 675D        5C857B  2E42B   2E42B
431.00M 511A 8712 0000  5121 2121 0000
  + 02  511B 7DCB       5122 17DA          F6B9
  + 64  5139 5E3B       513F F84A        1ED729   F6B9    F6B9
432.00M 514A B742 0000  5151 5151 0000
  + 02  514B ADFB       5152 480A
  + 64  5169 8E6B       5170 287A        1ED729
433.00M 517A E772 0000  5181 8181
  + 02  517B DE2B       5182 783A
  + 64  5199 BE9B       51A0 58AA        1ED729
*/

uint32_t frq_cal(uint8_t tr_mode, uint8_t fr_band, uint8_t fr_chan)
{
    // 29M
    const uint32_t frq_029[3] = {
        0x56EDB66A,
        0x57575757,
        0x000F6B94
    };

    // 51M
    const uint32_t frq_051[3] = {
        0x562AFBE1,
        0x56666666,
        0x0008AC84
    };

    // 144M
    const uint32_t frq_144[3] = {
        0x513D8324,
        0x51515151,
        0x0002E42B
    };

    // 145M
    const uint32_t frq_145[3] = {
        0x51CE13B5,
        0x51E1E1E1,
        0x0002E42B
    };

    // 431M
    const uint32_t frq_431[3] = {
        0x511A8712,
        0x51212121,
        0x0000F6B9
    };

    // 432M
    const uint32_t frq_432[3] = {
        0x514AB742,
        0x51515151,
        0x0000F6B9
    };

    // 433M
    const uint32_t frq_433[3] = {
        0x517AE772,
        0x51818181,
        0x0000F6B9
    };

    // 438M
    const uint32_t frq_438[3] = {
        0x526BD863,
        0x52727272,
        0x0000F6B9
    };

    uint32_t fr_base;
    uint32_t fr_step;
    uint32_t fr_calc;

        switch (fr_band){
            case 0:
                fr_base = frq_029[tr_mode];
                fr_step = frq_029[2];
                break;
            case 1:
                fr_base = frq_051[tr_mode];
                fr_step = frq_051[2];
                break;
            case 2:
                fr_base = frq_144[tr_mode];
                fr_step = frq_144[2];
                break;
            case 3:
                fr_base = frq_145[tr_mode];
                fr_step = frq_145[2];
                break;
            case 4:
                fr_base = frq_431[tr_mode];
                fr_step = frq_431[2];
                break;
            case 5:
                fr_base = frq_432[tr_mode];
                fr_step = frq_432[2];
                break;
            case 6:
                fr_base = frq_433[tr_mode];
                fr_step = frq_433[2];
                break;
            case 7:
                fr_base = frq_438[tr_mode];
                fr_step = frq_438[2];
                break;
            default:
                fr_base = frq_438[tr_mode];
                fr_step = frq_438[2];
        }

    fr_calc = fr_base + (fr_step * (uint32_t)(fr_chan >> 1));
    return fr_calc;
}

void rcv_set(uint8_t fr_band, uint8_t fr_chan, uint8_t af_vol, uint8_t sq_vol)
{
    uint32_t pll_cal;
    uint16_t frq_reg[3];
    uint8_t data;

    const uint16_t rcv_reg[24] = {
        0x517b,     // REG0 433.02
        0xde2c,
        0x0000,
        0x0000,
        0x0300,
        0x0c04,     // REG5
        0xf140,
        0xed00,
        0x17e0,
        0xe0e4,     // for 4802o
//      0xe0e0,     // for 4802n?
        0x8543,     // REG10
        0x0700,
        0xa066,
        0xffff,
        0xffe0,
        0x061f,     // REG15
        0x9e3c,
        0x1f00,
        0xd1d1,
//      0x200f,     // VOL
//      0x2400,     // VOL can not recv
        0x2000,
        0x01ff,     // REG20
        0xe000,
//      0x1800,     // SQL RSSI
        0x0000,     // SQL RSSI recv 40h
//      0x0300,     // SQL RSSI can not recv
//      0xacd0
//      0x0020      // SQL NOISE
//      0x0000      // SQL NOISE  can not recv
//      0x00F0      // SQL NOISE  can not recv
        0x00e8      // SQL NOISE
    };

    pll_cal = frq_cal(0, fr_band, fr_chan);
    frq_reg[0] = ((pll_cal >> 16) & 0x0000FFFF);
    frq_reg[1] = (pll_cal & 0x0000FFFF);

    RFAOFF;
//  Filter
    switch (fr_band){
        case 0:
            BSW28;
            frq_reg[2] = PLL_28;
            break;
        case 1:
            BSW50;
            frq_reg[2] = PLL_50;
            break;
        case 2:
            BSW144;
            frq_reg[2] = PLL_144;
            break;
        case 3:
            BSW144;
            frq_reg[2] = PLL_144;
            break;
        case 4:
//          BSW430;
            BSWTUN;
            frq_reg[2] = PLL_430;
            break;
        case 5:
//          BSW430;
            BSWTUN;
            frq_reg[2] = PLL_430;
            break;
        case 6:
//          BSW430;
            BSWTUN;
            frq_reg[2] = PLL_430;
            break;
        case 7:
//          BSW430;
            BSWTUN;
            frq_reg[2] = PLL_430;
            break;
        default:
            BSWTUN;
            frq_reg[2] = PLL_430;
    }

    // set
    for(uint8_t lp = 4; lp < 24; lp++){

    // Start
        SDA_HIGH;
        __delay_us(2);
        SCK_HIGH;
        __delay_us(2);
        SDA_LOW;
        __delay_us(2);
        SCK_LOW;
        __delay_us(2);
    //
        data = (uint8_t)(I2C_ADR & 0xFE );
        i2c_snd(data);
    //
        data = (uint8_t)((lp) & 0xFF );
        i2c_snd(data);
    //
        data = (uint8_t)((rcv_reg[lp] >> 8) & 0xFF );
        i2c_snd(data);
    //
        if(lp == REG_VOL){
//VOL
            data = (uint8_t)((rcv_reg[lp] + af_vol) & 0xFF );
        } else if(lp == REG_SQL){
//SQL
            data = (uint8_t)((rcv_reg[lp] + sq_vol) & 0xFF );
        } else {
            data = (uint8_t)(rcv_reg[lp] & 0xFF );
        }
        i2c_snd(data);
    // Stop
        SCK_HIGH;
        __delay_us(6);
        SDA_HIGH;
    // Wait
        __delay_us(20);
    }

    for(uint8_t lp = 0; lp < 3; lp++){
        uint16_t fdat;
        uint8_t adr;
        adr = (uint8_t)(2 - lp);
        fdat = frq_reg[adr];

    // Start
        SDA_HIGH;
        __delay_us(2);
        SCK_HIGH;
        __delay_us(2);
        SDA_LOW;
        __delay_us(2);
        SCK_LOW;
        __delay_us(2);
    //
        data = (uint8_t)(I2C_ADR & 0xFE);
        i2c_snd(data);
    //
        data = (uint8_t)(adr & 0xFF);
        i2c_snd(data);
    //
        data = (uint8_t)((fdat >> 8) & 0xFF);
        i2c_snd(data);
    //
        data = (uint8_t)(fdat & 0xFF);
        i2c_snd(data);
    // Stop
        SCK_HIGH;
        __delay_us(6);
        SDA_HIGH;
    // Wait
        __delay_us(20);
    }
}

void snd_set(uint8_t fr_band, uint8_t fr_chan, uint8_t po_vol)
{
    uint32_t pll_cal;
    uint16_t frq_reg[3];
    uint8_t data;

    const uint16_t snd_reg[24] = {
        0x5182,     // REG0 433.02
        0x783a,
        0x0000,
        0x0000,
        0x7c00,
        0x0004,     // REG5
        0xf140,
        0xed00,
        0x17e0,
        0xe0e4,
        0x8543,     // REG10
        0x0700,
        0xa066,
        0xffff,
        0xffe0,
        0x061f,     // REG15
        0x9e3c,
        0x1f00,
        0xd1c1,
        0x200f,
        0x01ff,     // REG20
        0xe000,
        0x0340,
        0xaed0
    };

    pll_cal = frq_cal(1, fr_band, fr_chan);
    frq_reg[0] = ((pll_cal >> 16) & 0x0000FFFF);
    frq_reg[1] = (pll_cal & 0x0000FFFF);

    RFAON;
//  Filter
    switch (fr_band){
        case 0:
            BSW28;
            frq_reg[2] = PLL_28;
            break;
        case 1:
            BSW50;
            frq_reg[2] = PLL_50;
            break;
        case 2:
            BSW144;
            frq_reg[2] = PLL_144;
            break;
        case 3:
            BSW144;
            frq_reg[2] = PLL_144;
            break;
        case 4:
//          BSW430;
            BSWTUN;
            frq_reg[2] = PLL_430;
            break;
        case 5:
//          BSW430;
            BSWTUN;
            frq_reg[2] = PLL_430;
            break;
        case 6:
//          BSW430;
            BSWTUN;
            frq_reg[2] = PLL_430;
            break;
        case 7:
//          BSW430;
            BSWTUN;
            frq_reg[2] = PLL_430;
            break;
        default:
            BSWTUN;
            frq_reg[2] = PLL_430;
    }

// set
    for(uint8_t lp = 4; lp < 24; lp++){

    // Start
        SDA_HIGH;
        __delay_us(2);
        SCK_HIGH;
        __delay_us(2);
        SDA_LOW;
        __delay_us(2);
        SCK_LOW;
        __delay_us(2);
    //
        data = (uint8_t)(I2C_ADR & 0xFE);
        i2c_snd(data);
    //
        data = (uint8_t)(lp & 0xFF);
        i2c_snd(data);
    //
        data = (uint8_t)((snd_reg[lp] >> 8) & 0xFF);
        i2c_snd(data);
    //
        data = (uint8_t)(snd_reg[lp] & 0xFF);
        i2c_snd(data);
    // Stop
        SCK_HIGH;
        __delay_us(6);
        SDA_HIGH;
    // Wait
        __delay_us(20);
    }

    for(uint8_t lp = 0; lp < 3; lp++){
        uint16_t fdat;
        uint8_t adr;
        adr = (uint8_t)(2 - lp);
        fdat = frq_reg[adr];

    // Start
        SDA_HIGH;
        __delay_us(2);
        SCK_HIGH;
        __delay_us(2);
        SDA_LOW;
        __delay_us(2);
        SCK_LOW;
        __delay_us(2);
    //
        data = (uint8_t)(I2C_ADR & 0xFE );
        i2c_snd(data);
    //
        data = (uint8_t)(adr & 0xFF );
        i2c_snd(data);
    //
        data = (uint8_t)((fdat >> 8) & 0xFF );
        i2c_snd(data);
    //
        data = (uint8_t)(fdat & 0xFF );
        i2c_snd(data);
    // Stop
        SCK_HIGH;
        __delay_us(6);
        SDA_HIGH;
    // Wait
        __delay_us(20);
    }
}

void lcd_cmd(uint8_t cmd){
    uint8_t data;

    // Start
        SDA_HIGH;
        __delay_us(2);
        SCK_HIGH;
        __delay_us(2);
        SDA_LOW;
        __delay_us(2);
        SCK_LOW;
        __delay_us(2);
    //
        data = (uint8_t)(I2C_LCD & 0xFE );
        i2c_snd(data);
    //
        data = 0x00;
        i2c_snd(data);
    //
        i2c_snd(cmd);
    // Stop
        SCK_HIGH;
        __delay_us(6);
        SDA_HIGH;
    // Wait
        __delay_us(20);
}

void lcd_init(){
    __delay_ms(40);
    lcd_cmd(0x38);      // Function set
    lcd_cmd(0x39);      // Function set w/ IS bit
    lcd_cmd(0x14);      // Internal OSC freq
    lcd_cmd(0x70);      // Contrast set
    lcd_cmd(0x56);      // Power/ICON/Contrast control
    lcd_cmd(0x6C);      // Follower control
    __delay_ms(200);
//  lcd_cmd(0x38);      // 2 Function set w/o IS bit
    lcd_cmd(0x34);      // 1 Function set w/o IS bit
    lcd_cmd(0x0C);      // Display On
    lcd_cmd(0x01);      // Clear Display
}

void lcd_dsp(uint8_t chr){
    uint8_t data;

    // Start
        SDA_HIGH;
        __delay_us(2);
        SCK_HIGH;
        __delay_us(2);
        SDA_LOW;
        __delay_us(2);
        SCK_LOW;
        __delay_us(2);
    //
        data = (uint8_t)(I2C_LCD & 0xFE );
        i2c_snd(data);
    //
        data = 0x40;
        i2c_snd(data);
    //
        i2c_snd(chr);
    // Stop
        SCK_HIGH;
        __delay_us(6);
        SDA_HIGH;
    // Wait
        __delay_us(20);
}

void lcd_txmode(void){
    lcd_cmd(0x02);      // Home
    lcd_dsp('T');
}

void lcd_rxmode(void){
    lcd_cmd(0x02);      // Home
    lcd_dsp('R');
}

void lcd_mode(uint8_t set_md){
    switch (set_md){
        case 0:
            lcd_dsp('R');
            lcd_dsp('X');
            break;

        case 1:
            lcd_dsp('B');
            lcd_dsp(' ');
            break;

        case 2:
            lcd_dsp('H');
            lcd_dsp(' ');
            break;

        case 3:
            lcd_dsp('L');
            lcd_dsp(' ');
            break;

        case 4:
            lcd_dsp('V');
            lcd_dsp(' ');
            break;

        case 5:
            lcd_dsp('S');
            lcd_dsp(' ');
            break;

        case 6:
            lcd_dsp('P');
            lcd_dsp(' ');
            break;

        default:
            lcd_cmd(0x14);
            lcd_cmd(0x14);
    }
}

void lcd_band(uint8_t fr_band){
    switch (fr_band){
        case 0:
            lcd_dsp(' ');
            lcd_dsp('2');
            lcd_dsp('9');
            break;
        case 1:
            lcd_dsp(' ');
            lcd_dsp('5');
            lcd_dsp('1');
            break;
        case 2:
            lcd_dsp('1');
            lcd_dsp('4');
            lcd_dsp('4');
            break;
        case 3:
            lcd_dsp('1');
            lcd_dsp('4');
            lcd_dsp('5');
            break;
        case 4:
            lcd_dsp('4');
            lcd_dsp('3');
            lcd_dsp('1');
            break;
        case 5:
            lcd_dsp('4');
            lcd_dsp('3');
            lcd_dsp('2');
            break;
        case 6:
            lcd_dsp('4');
            lcd_dsp('3');
            lcd_dsp('3');
            break;
        case 7:
            lcd_dsp('4');
            lcd_dsp('3');
            lcd_dsp('4');
            break;
        default:
            lcd_dsp('e');
            lcd_dsp('r');
            lcd_dsp('r');
        }
}

void lcd_ch(uint8_t set_md, uint8_t fr_band, uint8_t fr_chan){
    uint8_t ch_h = 0;
    uint8_t ch_l = 0;

    lcd_cmd(0x01);      // Clear
    lcd_mode(set_md);
    lcd_band(fr_band);

    ch_h = fr_chan / 10;
    ch_l = fr_chan % 10;
    lcd_dsp('.');
    lcd_dsp('0' + ch_h);
    lcd_dsp('0' + ch_l);
}

void lcd_fnc(uint8_t fnc, uint8_t vol){
    uint8_t vol_h;
    uint8_t vol_l;

    vol_h = (vol >> 4);
    vol_l = vol - (vol_h * 16);

    lcd_cmd(0x01);      // Clear

    switch (fnc){
        case 4:
            lcd_dsp('V');
            lcd_dsp(' ');
            break;

        case 5:
            lcd_dsp('S');
            lcd_dsp(' ');
            break;

        case 6:
            lcd_dsp('P');
            lcd_dsp(' ');
            break;

        default:
            lcd_dsp(' ');
            lcd_dsp(' ');
    }

    lcd_dsp(' ');

    if(vol_h > 9){
        lcd_dsp('A' + vol_h -10);
    } else {
        lcd_dsp('0' + vol_h);
    }
    if(vol_l > 9){
        lcd_dsp('A' + vol_l -10);
    } else {
        lcd_dsp('0' + vol_l);
    }
}

uint8_t sw_state(void)
{
    uint16_t adc_val;
    uint8_t cmp_val;
    __delay_us(20);
    GO_nDONE = 1;
    while(GO_nDONE) ;
    adc_val = ADRESH;
    adc_val = ( adc_val << 8 ) | ADRESL;
    cmp_val = (uint8_t)(adc_val >> 2);

//  lcd_cmd(0x01);
//  lcd_dsp(((cmp_val & 0xF0) >> 4) + '0');
//  lcd_dsp(((cmp_val & 0x0F)     ) + '0');

    if(cmp_val > REF_OP)
        return (STAT_OP);
    else if(cmp_val > REF_LT)
        return (STAT_LT);
    else if(cmp_val > REF_RT)
        return (STAT_RT);
    else if(cmp_val > REF_CT)
        return (STAT_CT);
    else if(cmp_val > REF_UP)
        return (STAT_UP);
    else if(cmp_val > REF_DN)
        return (STAT_DN);
    else
        return (STAT_PT);
}

void sw_check(void)
{
    uint16_t adc_val;
    uint8_t cmp_val;

    for(uint8_t i = 0; i < 10; i++){
        __delay_us(100);
        GO_nDONE = 1;
        while(GO_nDONE) ;
        adc_val = ADRESH;
        adc_val = ( adc_val << 8 ) | ADRESL;
        cmp_val = (uint8_t)(adc_val >> 2);

        lcd_cmd(0x01);
        if((((cmp_val & 0xF0) >> 4) + '0') > '9')
            lcd_dsp(((cmp_val & 0xF0) >> 4) + 'A' -10);
        else
            lcd_dsp(((cmp_val & 0xF0) >> 4) + '0');

        if((((cmp_val & 0x0F)     ) + '0') > '9')
            lcd_dsp(((cmp_val & 0x0F)     ) + 'A' -10);
        else
            lcd_dsp(((cmp_val & 0x0F)     ) + '0');
        
        __delay_ms(100);
    }
}

void joys_chk(void){
    uint8_t joys;
    joys = sw_state();
    while(joys != STAT_OP){
        for(uint8_t i = 0; i < 8 ; i++){
            switch (joys){
                case STAT_LT:
                    lcd_dsp('L');
                    break ;
                case STAT_RT:
                    lcd_dsp('R');
                    break ;
                case STAT_FN:
                    lcd_dsp('F');
                    break ;
                case STAT_UP:
                    lcd_dsp('U');
                    break ;
                case STAT_DN:
                    lcd_dsp('D');
                    break ;
                case STAT_CT:
                    lcd_dsp('C');
                    break ;
                case STAT_PT:
                    lcd_dsp('P');
                    break ;
                default:
                    lcd_dsp('O');
            }
            __delay_ms(100);
            joys = sw_state();
        }
        if(joys != STAT_OP){
            __delay_ms(200);
        }
        lcd_cmd(0x01);
    }
}

uint8_t chg_vol(uint8_t joys, uint8_t af_old){
    uint8_t af_vol;
    af_vol = af_old;
        switch (joys){
            case STAT_DN:
                LED_ON;
                if(af_vol <= 0){
                    af_vol = 0;
                } else {
                    af_vol = af_vol - 1;
                }
                __delay_ms(10);
                LED_OFF;
                break ;
            case STAT_UP:
                LED_ON;
                if(af_vol >= 15){
                    af_vol = 15;
                } else {
                    af_vol = af_vol + 1;
                }
                __delay_ms(10);
                LED_OFF;
                break ;
            default:
                ;
        }
        return af_vol;
}

uint8_t chg_sql(uint8_t joys, uint8_t sq_old){
    uint8_t sq_vol;
    sq_vol = sq_old;
        switch (joys){
            case STAT_DN:
                LED_ON;
                if(sq_vol < 4){
                    sq_vol = 0;
                } else {
                    sq_vol = sq_vol - 4;
                }
                __delay_ms(10);
                LED_OFF;
                break ;
            case STAT_UP:
                LED_ON;
                if(sq_vol >= 0xFC){
                    sq_vol = 0xFC;
                } else {
                    sq_vol = sq_vol + 4;
                }
                __delay_ms(10);
                LED_OFF;
                break ;
            default:
                ;
        }
        return sq_vol;
}

uint8_t chg_pow(uint8_t joys, uint8_t po_old){
    uint8_t po_vol;
    po_vol = po_old;
        switch (joys){
            case STAT_DN:
                LED_ON;
                if(po_vol <= 0){
                    po_vol = 0;
                } else {
                    po_vol = po_vol - 1;
                }
                __delay_ms(10);
                LED_OFF;
                break ;
            case STAT_UP:
                LED_ON;
                if(po_vol >= 7){
                    po_vol = 7;
                } else {
                    po_vol = po_vol + 1;
                }
                __delay_ms(10);
                LED_OFF;
                break ;
            default:
                ;
        }
        return po_vol;
}

uint8_t chg_frb(uint8_t joys, uint8_t fr_old){
    uint8_t fr_band;
    fr_band = fr_old;
        switch (joys){
            case STAT_DN:
                LED_ON;
                if(fr_band == 0){
                    fr_band = 7;
                } else {
                    fr_band--;
                }
                __delay_ms(10);
                LED_OFF;
                break ;
            case STAT_UP:
                LED_ON;
                if(fr_band >= 7){
                    fr_band = 0;
                } else {
                    fr_band++;
                }
                __delay_ms(10);
                LED_OFF;
                break ;
            default:
                ;
        }
        return fr_band;
}

uint8_t chg_frh(uint8_t joys, uint8_t fr_old){
    uint8_t fr_chan;
    fr_chan = fr_old;
        switch (joys){
            case STAT_DN:
                LED_ON;
                if(fr_chan < 10){
                    fr_chan = fr_chan + 90;
                } else {
                    fr_chan = fr_chan - 10;
                }
                __delay_ms(10);
                LED_OFF;
                break ;
            case STAT_UP:
                LED_ON;
                fr_chan = fr_chan + 10;
                if(fr_chan >= 100){
                    fr_chan = fr_chan - 100;
                }
                __delay_ms(10);
                LED_OFF;
                break ;
            default:
                ;
        }
        if(fr_chan >= 100){
            fr_chan = 0;
        }
        return fr_chan;
}

uint8_t chg_frl(uint8_t joys, uint8_t fr_old){
    uint8_t fr_chan;
    fr_chan = fr_old;
        switch (joys){
            case STAT_DN:
                LED_ON;
                if(fr_chan == 0){
                    fr_chan = 98;
                } else {
                    fr_chan = fr_chan - 2;
                }
                __delay_ms(10);
                LED_OFF;
                break ;
            case STAT_UP:
                LED_ON;
                if(fr_chan >= 98){
                    fr_chan = 0;
                } else {
                    fr_chan = fr_chan + 2;
                }
                __delay_ms(10);
                LED_OFF;
                break ;
            default:
                ;
        }
        if(fr_chan >= 100){
            fr_chan = 0;
        }
        return fr_chan;
}

void main(void) {

    uint8_t flag = RECV;
    uint8_t freq = 5;
    uint8_t joys;
    uint8_t set_md = 0;
    uint8_t s_timer = 0;
    uint8_t af_vol = 10;
    uint8_t sq_vol = 0x40;
    uint8_t po_vol = 7;
    uint8_t fr_band = 6;
    uint8_t fr_chan = 0;

//Initialize
    port_init();
    lcd_init();
    __delay_ms(100);
//  sw_check();
    joys_chk();
    __delay_ms(100);
    lcd_ch(set_md, fr_band, fr_chan);
    rcv_set(fr_band, fr_chan, 10, 0x48);
    __delay_ms(100);

//Loop
    while(1){
        joys = sw_state();
        __delay_ms(1);

// Change
        if(s_timer > 0){
            switch (set_md){
                case 1:
                    fr_band = chg_frb(joys, fr_band);
                    lcd_ch(set_md, fr_band, fr_chan);
                    break;
                case 2:
                    fr_chan = chg_frh(joys, fr_chan);
                    lcd_ch(set_md, fr_band, fr_chan);
                    break;
                case 3:
                    fr_chan = chg_frl(joys, fr_chan);
                    lcd_ch(set_md, fr_band, fr_chan);
                    break;
                case 4:
                    af_vol = chg_vol(joys, af_vol);
                    lcd_fnc(set_md, af_vol);
                    break;
                case 5:
                    sq_vol = chg_sql(joys, sq_vol);
                    lcd_fnc(set_md, sq_vol);
                    break;
                case 6:
                    po_vol = chg_pow(joys, po_vol);
                    lcd_fnc(set_md, po_vol);
                    break;
                default:
                    ;
            }
        }

        switch (joys){
// left/right
            case STAT_RT:
                LED_ON;
                if(set_md >=6){
                    set_md = 6;
                } else {
                    set_md++;
                }
                s_timer = 50;
                __delay_ms(120);
                LED_OFF;
                break ;
            case STAT_LT:
                LED_ON;
                if(set_md == 0){
                    set_md = 0;
                    s_timer = 0;
                } else {
                    set_md--;
                    s_timer = 50;
                }
                __delay_ms(120);
                LED_OFF;
                break ;
// up/down
            case STAT_UP:
            case STAT_DN:
                if(set_md == 0){
                    s_timer = 0;
                } else {
                    s_timer = 30;
                }
                __delay_ms(80);
                break ;
// SEND
            case STAT_PT:
                if(flag == RECV){
                    flag = SEND;
                    lcd_ch(set_md, fr_band, fr_chan);
                    lcd_txmode();
                    snd_set(fr_band, fr_chan, 7);
                }
                s_timer = 0;
                set_md = 0;
                break ;
// RECV
            default:
//              rcv_set(fr_band, fr_chan, 10, 0x48);
                rcv_set(fr_band, fr_chan, af_vol, sq_vol);
                if(flag != RECV){
                    flag = RECV;
                    lcd_ch(set_md, fr_band, fr_chan);
//                  rcv_set(fr_band, fr_chan, 10, 0x48);
                    rcv_set(fr_band, fr_chan, af_vol, sq_vol);
                }
                if(s_timer == 0){
                    set_md = 0;
                    lcd_ch(set_md, fr_band, fr_chan);
//                  rcv_set(fr_band, fr_chan, 10, 0x48);
                    rcv_set(fr_band, fr_chan, af_vol, sq_vol);
                } else {
                    s_timer--;
                    __delay_ms(50);
                }
        }
    }
}
