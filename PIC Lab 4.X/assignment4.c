/*
 * File:   PIC and Galileo communication          
 *         
 * 
 * simple PIC program example
 * for UMass Lowell 16.480/552
 * 
 * Team: Steve
 * Members: Xavier Guay
 *          Chris Cook
 *          Carl Loeffler
 *
 * Edited on 10/22/2015
 */

/*
PIC:                Galileo:
RC0/AN4 = Pin 10    -> GP_4 = GPIO 28
RC1/AN5 = Pin 9     -> GP_5 = GPIO 17
RC2/AN6 = Pin 8     -> GP_6 = GPIO 24
RC3/AN7 = Pin 7     -> GP_7 = GPIO 27
RC4 = Pin 6         -> GP_8 - GPIO 26  <- Strobe
 * 
RA2/AN2  = Pin 10 <- photoresistor
*/

//turn off the watchdog timer
#pragma config WDTE = OFF // 0b11

#include <pic16f688.h>
#include <stdlib.h>
#include <stdio.h>
#include <htc.h>
#include <string.h>
#include <xc.h>

#define _XTAL_FREQ 4000000

#define STROBE RC4

//master command messages
#define MSG_RESET 0x00
#define MSG_PING  0x01
#define MSG_GET   0x02

//slave special characters
#define MSG_ACK    0x0E
#define MSG_NOTHING   0x0F

//function prototypes
void init();
void set_read();
void set_write();
unsigned char read_msg();
void write_msg(char msg);
void read_adc();
int vread();
void wait_while_strobe_high();
void wait_while_strobe_low();
void debug_write(char msg);

//call only while strobe is high, will return after strobe goes low
void wait_while_strobe_high(){
    //Computer pulls strobe high
    while(1){
        while(STROBE){
            continue;
        }
        __delay_ms(1);
        if(!STROBE){
            return;
        }
    }
}

//call only while strobe is low, will return after strobe goes high
void wait_while_strobe_low(){
    //Computer pulls strobe high
    while(1){
        while(!STROBE){
            continue;
        }
        __delay_ms(1);
        if(STROBE){
            return;
        }
    }
}

void init(){
    //ADC
    //pin 10 to analog input
    TRISA2 = 1;
    ANSEL = 0b00001001;
    
    //Set channel to AN2
    ADCON0 = 0b00001001;
    ADCON1 = 0b00010000;
    
    //digital
    //set_write();
    TRISC5 = 0; //Set debug line to write
    CMCON0 = 0x7;
    
}

void set_read()
{
    TRISC0 = 1;
    TRISC1 = 1;
    TRISC2 = 1;
    TRISC3 = 1;
    TRISC4 = 1;
}

void set_write(){
    TRISC0 = 0;
    TRISC1 = 0;
    TRISC2 = 0;
    TRISC3 = 0;
    TRISC4 = 1;// <- don't touch the strobe pin, we still need to read timing from this
}

/*
PIC:                Galileo:
RC0/AN4 = Pin 10    -> GP_4 = GPIO 28
RC1/AN5 = Pin 9     -> GP_5 = GPIO 17
RC2/AN6 = Pin 8     -> GP_6 = GPIO 24
RC3/AN7 = Pin 7     -> GP_7 = GPIO 27
RC4 = Pin 6         -> GP_8 - GPIO 26  <- Strobe
*/

//write_message writes bottom half of byte to the bottom half of PORTC
void write_msg(char msg){

    //we assume bus is idle when function called
    wait_while_strobe_low();
    
    //Computer pulls strobe high
    wait_while_strobe_high();
    
    //Once strobe is low write msg to bus and wait for strobe change
    PORTC = msg;
    set_write();
    wait_while_strobe_low();
    
    //Galileo reads data
    wait_while_strobe_high();
    
    //prepare to read
    PORTC = 0x0;
    set_read();
    
    //and bus is low again when we leave this function
}

char read_msg()
{
    //set to high impedance
    set_read();
    //wait while the bus is idle (strobe is low)
    wait_while_strobe_low();
    
    //Wait for computer to announce upcoming write cycle
    wait_while_strobe_high();
    
    //Wait while computer puts data on bus
    wait_while_strobe_low();
    
    //when strobe goes high, there's data on the bus
    //Read data off bottom nibble
    char msg = PORTC;
    msg &= 0x0f;
    //wait to return until bus is in idle state again
    wait_while_strobe_high();
    
    debug_write(msg);
    //return data
    return msg;
}

void debug_write(char msg){
    for(int i = 0x01; i < 0x10; i <<= 1){
        RC5 = 0;
        __delay_ms(0.1);
        RC5 = 1;
        __delay_ms(0.1);
        RC5 = 0;
        __delay_ms(0.1);
        if(msg & i){
            RC5 = 1;
        }else{
            RC5 = 0;
        }
        __delay_ms(1);
        RC5 = 0;
        __delay_ms(0.1);
        RC5 = 1;
        __delay_ms(0.1);
        RC5 = 0;
        __delay_ms(0.1);
    }
}



//performs ADC read (contains busyloop because we're bitbang reading the internal ADC
//finished flag, we're good at things we swear)
void read_adc(){
   
   //start read, leave busyloop when it finishes
   GO = 1;
   for(;;){
       if(!GO){
           return;
       }
   }
}

int vread(){
        read_adc();
        //Left justified ADFM=0
        int rval = ADRESH;
        rval << 2;
        rval += (ADRESL>>6);
        return rval;
}

// Main program
void main (void)
{
    init();
    set_read();
    //RC5 = 1;
    //wait_while_strobe_low();
    //RC5 = 0;
    
    char cmd;
    int data;
    RC5 = 0;
    
    debug_write(0xA);
   
    while(1){
        //asm("RSTWDT");
        RC5 = 1;
        cmd = read_msg();
        RC5 = 0;
        switch(cmd){
            case(MSG_RESET): //RESET
                init();
                //send ACK
                write_msg(MSG_ACK);//ACK
                break;
            case(MSG_PING): //PING
                //send ACK
                write_msg(MSG_ACK); //ACK
                break;
            case(MSG_GET): //GET
                //send data
                data = vread();
                char chunk1, chunk2, chunk3;  //chunk 1 holds lowest 4 bits, chunk 2 middle, etc.
                chunk1 = (char) (data & 0x000f);
                chunk2 = (char) ((data >> 4) & 0x000f);
                chunk3 = (char) ((data >> 8) & 0x000f);
                write_msg(chunk3);//chunk3);
                write_msg(chunk2);//chunk2);
                write_msg(chunk1);//chunk1);
                //send ACK
                write_msg(MSG_ACK);
                break;
            default:
                write_msg(MSG_NOTHING);
            break;
        }
    }
}

