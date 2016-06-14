//===============================================
// MODIFIED AND DESKTOP VERSION
// THIS VERSION SHOULD NOT BE IN THE Z DRIVE
// THIS IS SOLELY FOR TESTING PURPOSES
// ASYNC TEST
//===============================================

#include <msp430.h>
#include <ctl.h>
#include <stdio.h>
#include <string.h>
#include <ARCbus.h>
#include <Error.h>
#include <terminal.h>
#include <UCA1_uart.h>
#include <crc.h>
#include "SDlib.h"
#include "pins.h"
#include "subsystem.h"
#include "sensor.h"
#include "flash.h"
#include "timerA.h"

CTL_TASK_t tasks[3];

//stacks for tasks
unsigned stack1[1+200+1];  
unsigned stack2[1+500+1];
unsigned stack3[1+150+1];  

CTL_EVENT_SET_t cmd_parse_evt;

unsigned char buffer[80];

//set printf and friends to send chars out UCA1 uart
int __putchar(int c)
{
  //don't print if async connection is open
  if(!async_isOpen())
  {
    return UCA1_TxChar(c);
  }else
  {
    return EOF;
  }
}

//set printf and friends to send chars out UCA1 uart
int __getchar(void)
{
  return UCA1_Getc();
}

//handle subsystem specific commands
int SUB_parseCmd(unsigned char src,unsigned char cmd,unsigned char *dat,unsigned short len)
{
  //Return Error
  return ERR_UNKNOWN_CMD;
}

void cmd_parse(void *p) __toplevel
{
  unsigned int e;
  //init event
  ctl_events_init(&cmd_parse_evt,0);
  for(;;)
  {
    e=ctl_events_wait(CTL_EVENT_WAIT_ANY_EVENTS_WITH_AUTO_CLEAR,&cmd_parse_evt,0x00,CTL_TIMEOUT_NONE,0);
  }
}

int main(void)
{
  unsigned char addr;
  //DO this first
  ARC_setup(); 

  //turn on LED's this will flash the LED's during startup
  P7OUT=0xFF;
  P7DIR=0xFF;

  //Initialize camera sensor
  //sensor_init();

  //TESTING: set log level to report everything by default
  set_error_level(0);

  //BUS_set_test_mode(BUS_TM_NO_TIMESLICE);

  //initialize UART
  UCA1_init_UART(UART_PORT, UART_TX_PIN_NUM, UART_RX_PIN_NUM);
  UCA1_BR57600();

  mmcInit_msp();
  
  //addr = 0x1F;
  addr=0x1F;


  //setup bus interface
  initARCbus(addr);
  //BUS_register_cmd_callback(&IMG_parse);

  //initialize stacks
  memset(stack1,0xcd,sizeof(stack1));  // write known values into the stack
  stack1[0]=stack1[sizeof(stack1)/sizeof(stack1[0])-1]=0xfeed; // put marker values at the words before/after the stack
  
  memset(stack2,0xcd,sizeof(stack2));  // write known values into the stack
  stack2[0]=stack2[sizeof(stack2)/sizeof(stack2[0])-1]=0xfeed; // put marker values at the words before/after the stack
    
  memset(stack3,0xcd,sizeof(stack3));  // write known values into the stack
  stack3[0]=stack3[sizeof(stack3)/sizeof(stack3[0])-1]=0xfeed; // put marker values at the words before/after the stack

  //create tasks
  ctl_task_run(&tasks[0],BUS_PRI_LOW,cmd_parse,NULL,"cmd_parse",sizeof(stack1)/sizeof(stack1[0])-2,stack1+1,0);
  ctl_task_run(&tasks[1],BUS_PRI_NORMAL,terminal,"Program","terminal",sizeof(stack2)/sizeof(stack2[0])-2,stack2+1,0);
  ctl_task_run(&tasks[2],BUS_PRI_HIGH,sub_events,NULL,"sub_events",sizeof(stack3)/sizeof(stack3[0])-2,stack3+1,0);

  mainLoop(); 
}


