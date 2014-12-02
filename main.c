#include <msp430.h>
#include <ctl_api.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <ARCbus.h>
#include <UCA1_uart.h>
#include <SDlib.h>
#include "timerA.h"
#include <terminal.h>
#include "Adafruit_VC0706.h"
#include "IMG_Events.h"
#include "Error.h"
#include "Proxy_errors.h"
#include "IMG.h"

CTL_TASK_t tasks[4];

//stacks for tasks
unsigned stack1[1+100+1];          
unsigned stack2[1+200+1];
unsigned stack3[1+350+1];   
unsigned stack4[1+300+1];                                                                  


//set printf and friends to send chars out UCA1 uart
int __putchar(int c){
  return async_TxChar(c);
}

int __getchar(void){
  return async_Getc();
}

//init mmc card before starting terminal task
void async_wait_term(void *p) __toplevel{
  int resp;
  //wait for async connection to open
  while(!async_isOpen()){
    ctl_timeout_wait(ctl_get_current_time()+1024);
  }
  /*P7OUT^=0xFF;
  ctl_timeout_wait(ctl_get_current_time()+500);
  P7OUT^=0xFF;*/
  //start terminal
  terminal(p);
}



int main(void){
  unsigned char addr;
  //DO this first

  // Resetting for some reason
  ARC_setup(); 

  //setup UCA1 uart, may not be needed
  UCA1_init_UART();
  
  //setup system specific peripherals
  Adafruit_VC0706_init();

  //setup mmc interface
  mmcInit_msp();
 
  //setup P7.0 for imager on/off
  P7OUT=BIT0;
  P7DIR=0xFF;
  P7REN=0;
  P7SEL=0;
  // Set imager to off to start with (this will save power)
  P7OUT=BIT1;



  //setup P6 for LED's
  P6OUT&=~0xF0;
  P6DIR|= 0xF0;
  P6OUT|=BIT4;

  //P6OUT|=BIT7;
  
  //setup bus interface
  initARCbus(BUS_ADDR_IMG);

  //initialize stacks
  memset(stack1,0xcd,sizeof(stack1));  // write known values into the stack
  stack1[0]=stack1[sizeof(stack1)/sizeof(stack1[0])-1]=0xfeed; // put marker values at the words before/after the stack
  
  memset(stack2,0xcd,sizeof(stack2));  // write known values into the stack
  stack2[0]=stack2[sizeof(stack2)/sizeof(stack2[0])-1]=0xfeed; // put marker values at the words before/after the stack
    
  memset(stack3,0xcd,sizeof(stack3));  // write known values into the stack
  stack3[0]=stack3[sizeof(stack3)/sizeof(stack3[0])-1]=0xfeed; // put marker values at the words before/after the stack

  memset(stack4,0xcd,sizeof(stack4));  // write known values into the stack
  stack4[0]=stack4[sizeof(stack4)/sizeof(stack4[0])-1]=0xfeed; // put marker values at the words before/after the stack

  //create tasks
  // Use this task to handle commands
  ctl_task_run(&tasks[0],BUS_PRI_LOW,cmd_parse,NULL,"cmd_parse",sizeof(stack1)/sizeof(stack1[0])-2,stack1+1,0);
  //ctl_task_run(&tasks[1],2,async_wait_term,(void*)&async_term,"terminal",sizeof(stack2)/sizeof(stack2[0])-2,stack2+1,0);
  ctl_task_run(&tasks[1],2,terminal,"IMG Test Program","terminal",sizeof(stack2)/sizeof(stack2[0])-2,stack2+1,0);
  ctl_task_run(&tasks[2],BUS_PRI_HIGH,sub_events,NULL,"sub_events",sizeof(stack3)/sizeof(stack3[0])-2,stack3+1,0);
  ctl_task_run(&tasks[3],3,img_events,NULL,"img_events",sizeof(stack4)/sizeof(stack4[0])-2,stack4+1,0);
 
  mainLoop();
}

