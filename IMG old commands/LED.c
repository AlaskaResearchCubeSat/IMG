#include <msp430.h>
#include "LED.h"



void LED_init(void){
  //setup P6 for LED's
  P6OUT&=~LED_PIN_MASK;
  P6DIR|= LED_PIN_MASK;
}

void LED_on(unsigned char mask){
    //mask out non LED pins
    mask&=LED_PIN_MASK;
    //set pin(s) high
    P6OUT|=mask;
}

void LED_off(unsigned char mask){
    //mask out non LED pins
    mask&=LED_PIN_MASK;
    //set pin(s) low
    P6OUT&=~mask;
}

void LED_toggle(unsigned char mask){
    //mask out non LED pins
    mask&=LED_PIN_MASK;
    //toggle pin(s)
    P6OUT^=mask;
}
