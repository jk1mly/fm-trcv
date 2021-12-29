/* 
 * FM transceiver for PIC12F1501
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
//#include <c90/stdlib.h>
#include <stdint.h>
#include <xc.h>
#include <pic.h>
#include <pic12F1501.h>

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

#define	SDA_LOW     LATA0 = 0;TRISA0 = 0    // 0
#define SDA_HIGH	LATA0 = 1;TRISA0 = 1    // Z(1))
#define	SCK_LOW     LATA1 = 0
#define	SCK_HIGH	LATA1 = 1

#define	I2C_ADR     0x90

#define	LED_ON      LATA5 = 1
#define	LED_OFF     LATA5 = 0
#define	BAND    	RA2
#define	PTT     	RA4
#define	SWON    	0
#define	RECV    	0
#define	SEND    	1

#define	FRQ_REG     frq_reg430


// reference
// https://aitendo3.sakura.ne.jp/aitendo_data/product_img/ic/wirless/BK4802P/BK4802N-BEKEN.pdf
// http://pdf-html.ic37.com/pdf_file_U1/20200531/pdf_pdf/uploadpdf/ETC/BK4802_datasheet_1240666/202073/BK4802_datasheet.pdf
// https://github.com/BG7QKU/BK4802P-4BANDS-FM-TRANSCEIVER-WITH-BLUETOOTH-CONTROL-BASED-STC8F-MCU/blob/master/MCUMAIN

void port_init(void) {
    /* CONFIGURE GPIO */ 
    OSCCON  = 0b01101000;    //4MHz
    TRISA   = 0b00011101;    //Input(1)
    OPTION_REG = 0b00000000; //MSB WPUENn
    WPUA    = 0b00011101;    //PupOn(1)
    INTCON  = 0b00000000;
    LATA    = 0b00000011;
    ANSELA  = 0b00000000;
	ADCON0  = 0b00000000;
	ADCON1  = 0b10010000;    //ADFM ADCS001 2us
	ADCON2  = 0b00000000;

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
    
void vol_set(void)
{    
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
        data = (uint8_t)(I2C_ADR & 0xFE );
        i2c_snd(data);	
    // 
        data = (uint8_t)( 32 );
        i2c_snd(data);	
    // 	
        data = (uint8_t)( 0x01 );
        i2c_snd(data);	 
    // 
        data = (uint8_t)( 0xff );
        i2c_snd(data);	
    // Stop            
        SCK_HIGH;
        __delay_us(6);	
        SDA_HIGH;
    // Wait
        __delay_us(20);
}

void rcv_chk(void)
{
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
        0xe0e4,     
        0x8543,     // REG10
        0x0700,
        0xa066,
        0xffff,
        0xffe0,
        0x061f,     // REG15
        0x9e3c,
        0x1f00,
        0xd1d1,
        0x200f,
        0x01ff,     // REG20
        0xe000,
        0x1800,        
        0xacd0
    };

    // 29.18M
    const uint16_t frq_reg28x[3] = {
        0x5778,
        0x7ea3,
        0xa000
    };

    // 29.18M
    const uint16_t frq_reg28a[3] = {
        0x5778,
        0x7ea3,
        0xa000
    };

    // 29.18M
    const uint16_t frq_reg28b[3] = {
        0x6267,
        0x8e77,
        0xc000
    };

    // 51.18M
    const uint16_t frq_reg50x[3] = {
        0x4cdd,
        0x6072,
        0x6000
    };

    // 51.18M
    const uint16_t frq_reg50a[3] = {
        0x4cdd,
        0x6072,
        0x4000
    };

    // 51.18M
    const uint16_t frq_reg50b[3] = {
        0x5679,
        0x0c81,
        0x6000
    };

    // 145.18M
    const uint16_t frq_reg144[3] = {
        0x51e8,
        0x1940,
        0x2000
    };

    // 433.18M
    const uint16_t frq_reg430[3] = {
        0x5183,
        0x93f6,
        0x0000
    };
        
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
        data = (uint8_t)(rcv_reg[lp] & 0xFF );
        i2c_snd(data);	
    // Stop            
        SCK_HIGH;
        __delay_us(6);	
        SDA_HIGH;
    // Wait
        __delay_us(20);
    }    

// Freq
    for(uint8_t lp = 0; lp < 3; lp++){
        uint8_t adr;
        adr = 2 - lp;
    
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
        data = (uint8_t)((frq_reg430[adr] >> 8) & 0xFF);
//        data = (uint8_t)((frq_reg50x[adr] >> 8) & 0xFF);
        i2c_snd(data);	
    // 
        data = (uint8_t)(frq_reg430[adr] & 0xFF);
//        data = (uint8_t)(frq_reg50x[adr] & 0xFF);
        i2c_snd(data);	
    // Stop            
        SCK_HIGH;
        __delay_us(6);	
        SDA_HIGH;
    // Wait
        __delay_us(20);
    }    
}

void snd_chk(void)
{
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

    // 29.18M
    const uint16_t frq_reg28x[3] = {
        0x57e2,
        0x1f90,
        0xc000
    };

    // 29.18M
    const uint16_t frq_reg28a[3] = {
        0x57e2,
        0x1f90,
        0xa000
    };

    // 29.18M
    const uint16_t frq_reg28b[3] = {
        0x62de,
        0x6382,
        0xc000
    };

    // 51.18M
    const uint16_t frq_reg50x[3] = {
        0x4d12,
        0x30e9,
        0x6000
    };

     // 51.18M
    const uint16_t frq_reg50a[3] = {
        0x4d12,
        0x30e9,
        0x4000
    };

   // 51.18M
    const uint16_t frq_reg50b[3] = {
        0x57d2,
        0xb3fb,
        0x6000
    };

    // 145.18M
    const uint16_t frq_reg144[3] = {
        0x51fb,
        0xe76c,
        0x2000
    };

    // 433.18M
    const uint16_t frq_reg430[3] = {
        0x518a,
        0x2e05,
        0x0000
    };

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

// Freq
    for(uint8_t lp = 0; lp < 3; lp++){
        uint8_t adr;
        adr = 2 - lp;
    
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
        data = (uint8_t)((frq_reg430[adr] >> 8) & 0xFF );
//        data = (uint8_t)((frq_reg50a[adr] >> 8) & 0xFF );
        i2c_snd(data);	
    // 
        data = (uint8_t)(frq_reg430[adr] & 0xFF );
//        data = (uint8_t)(frq_reg50a[adr] & 0xFF );
        i2c_snd(data);	
    // Stop            
        SCK_HIGH;
        __delay_us(6);	
        SDA_HIGH;
    // Wait
        __delay_us(20);
    }    
}

void main(void) {

    uint8_t flag = RECV;

//Initialize
	port_init();
 
//    rcv_chk();
//    snd_chk();
//    __delay_ms(1000);
    vol_set();
    rcv_chk();
    
//Loop    
    while(1){
        if(BAND == SWON){
            LED_ON;
            __delay_ms(1000);        
            LED_OFF;
        }

        LED_ON;
        if(PTT == SWON){
// TX
            if(flag == RECV){
                snd_chk();
                flag = SEND;
//                snd_chk();
            }
            __delay_ms(200);
            LED_OFF;
            __delay_ms(50);
// RX
        } else {
//            if(flag == SEND){
                rcv_chk();
                flag = RECV;
//            }
            __delay_ms(50);
            LED_OFF;
            __delay_ms(200);
        }
//        __delay_ms(200);                
    }
}
