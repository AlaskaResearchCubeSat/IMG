//===============================================
// MODIFIED AND DESKTOP VERSION
// THIS VERSION SHOULD NOT BE IN THE Z DRIVE
// THIS IS SOLELY FOR TESTING PURPOSES
// IMAGER TEST
//===============================================

#include <msp430.h>
#include <ctl.h>
#include <stdio.h>
#include <string.h>
#include <Error.h>
#include <terminal.h>
#include <UCA3_uart.h>
#include <crc.h>
#include "ARCbus.h"
#include "SDlib.h"
#include "pins.h"
#include "subsystem.h"
#include "sensor.h"
#include "timerA.h"
#include "IMG.h"
#include "IMG_errors.h"

CTL_TASK_t tasks[4];

//stacks for tasks       
unsigned stack1[1+300+1];
unsigned stack2[1+350+1];   
unsigned stack3[1+200+1];                                                                  


//set printf and friends to send chars out UCA1 uart
int __putchar(int c)
{
  return UCA3_TxChar(c);
}

int __getchar(void)
{
  return UCA3_Getc();
}

//init mmc card before starting terminal task
void async_wait_term(void *p) __toplevel
{
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


int main(void)
{
  //unsigned char addr;
  //DO this first

  // Resetting for some reason
  ARC_setup(); 

  //setup image sensor
  sensor_init();

  //setup mmc interface
  mmcInit_msp();

  UCA3_init_UART(UART_PORT,UART_TX_PIN_NUM,UART_RX_PIN_NUM);
  UCA3_BR57600();

  P7OUT=0xFF;
  P7DIR=0xFF;
  //setup bus interface
  initARCbus(BUS_ADDR_IMG);

  BUS_register_cmd_callback(&IMG_parse);

  //initialize stacks
  memset(stack1,0xcd,sizeof(stack1));  // write known values into the stack
  stack1[0]=stack1[sizeof(stack1)/sizeof(stack1[0])-1]=0xfeed; // put marker values at the words before/after the stack
  
  memset(stack2,0xcd,sizeof(stack2));  // write known values into the stack
  stack2[0]=stack2[sizeof(stack2)/sizeof(stack2[0])-1]=0xfeed; // put marker values at the words before/after the stack
    
  memset(stack3,0xcd,sizeof(stack3));  // write known values into the stack
  stack3[0]=stack3[sizeof(stack3)/sizeof(stack3[0])-1]=0xfeed; // put marker values at the words before/after the stack
/*
  //create tasks
  //ctl_task_run(&tasks[1],BUS_PRI_LOW,async_wait_term,(void*)&async_term,"terminal",sizeof(stack1)/sizeof(stack1[0])-2,stack1+1,0);
  ctl_task_run(&tasks[1],BUS_PRI_LOW,terminal,"IMG Test Program","terminal",sizeof(stack1)/sizeof(stack1[0])-2,stack1+1,0);
  ctl_task_run(&tasks[2],BUS_PRI_HIGH,sub_events,NULL,"sub_events",sizeof(stack2)/sizeof(stack2[0])-2,stack2+1,0);
  ctl_task_run(&tasks[3],BUS_PRI_NORMAL,img_events,NULL,"img_events",sizeof(stack3)/sizeof(stack3[0])-2,stack3+1,0);
 */
  //create tasks
  //ctl_task_run(&tasks[1],BUS_PRI_LOW,async_wait_term,(void*)&async_term,"terminal",sizeof(stack1)/sizeof(stack1[0])-2,stack1+1,0);
  ctl_task_run(&tasks[1],BUS_PRI_LOW,terminal,"IMG Test Program","terminal",sizeof(stack1)/sizeof(stack1[0])-2,stack1+1,0);
  ctl_task_run(&tasks[2],BUS_PRI_HIGH,sub_events,NULL,"sub_events",sizeof(stack2)/sizeof(stack2[0])-2,stack2+1,0);
  //ctl_task_run(&tasks[3],BUS_PRI_NORMAL,img_events,NULL,"img_events",sizeof(stack3)/sizeof(stack3[0])-2,stack3+1,0);
 

  mainLoop();
}

