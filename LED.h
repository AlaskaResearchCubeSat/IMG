#ifndef __LED_H
#define __LED_H


#define LED_PIN_MASK    (0xF0)

enum{PWR_LED=1<<4,IMG_PWR_LED=1<<5,IMG_READ_LED=1<<6};

void LED_init(void);
void LED_on(unsigned char mask);
void LED_off(unsigned char mask);
void LED_toggle(unsigned char mask);

#endif
    