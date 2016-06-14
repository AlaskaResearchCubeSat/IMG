//===============================================
// MODIFIED AND DESKTOP VERSION
// THIS VERSION SHOULD NOT BE IN THE Z DRIVE
// THIS IS SOLELY FOR TESTING PURPOSES
// ASYNC TEST
//===============================================

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <msp430.h>
#include <ctl.h>
#include <terminal.h>
#include "ARCbus.h"
#include <UCA1_uart.h>
#include <Error.h>
#include <commandLib.h>
#include <SDlib.h>
#include <crc.h>
#include "IMG_errors.h"
#include "Adafruit_VC0706.h"
#include "sensor.h"
#include "IMG.h"
#include "status.h"
#include "flash.h"
#include <ctl.h>

int example_command(char **argv,unsigned short argc){
  int i,j;
  printf("This is an example command that shows how arguments are passed to commands.\r\n""The values in the argv array are as follows : \r\n");
  for(i=0;i<=argc;i++){
    printf("argv[%i] = 0x%p\r\n\t""string = \"%s\"\r\n",i,argv[i],argv[i]);
    //print out the string as an array of hex values
    j=0;
    printf("\t""hex = {");
    do{
      //check if this is the first element
      if(j!='\0'){
        //print a space and a comma
        printf(", ");
      }
      //print out value
      printf("0x%02hhX",argv[i][j]);
    }while(argv[i][j++]!='\0');
    //print a closing bracket and couple of newlines
    printf("}\r\n\r\n");
  }
  return 0;
}
/*
void write_settings(FLASH_SETTINGS *set)
{
  int en;
  //erase address section
  //disable interrupts
  en = BUS_stop_interrupts();
  //disable watchdog
  WDT_STOP();
  //unlock flash memory
  FCTL3=FWKEY;
  //setup flash for erase
  FCTL1=FWKEY|ERASE;
  //dummy write to indicate which segment to erase
  saved_settings->magic=0;
  //enable writing
  FCTL1=FWKEY|WRT;
  //write settings
  memcpy(saved_settings,set,sizeof(FLASH_SETTINGS));
  //disable writing
  FCTL1=FWKEY;

  //lock flash
  FCTL3=FWKEY|LOCK;
  //re-enable interrupts if enabled before
  BUS_restart_interrupts(en);
}*/

int addr(char **argv,unsigned short argc){
  //unsigned long addr;
  //unsigned char tmp;
  //char *end;
  int en;
  unsigned char addr,addr_f,addr_p;
  const char *name;
  if(argc==0){
    addr_f=*(unsigned char*)0x01000;
    addr_p=BUS_get_OA();
    if(addr_f!=addr_p){
      if((name=I2C_addr_revlookup(addr_f,busAddrSym))!=NULL){
        printf("Flash I2C address = 0x%02X (%s)\r\n",addr_f,name);
      }else{
        printf("Flash I2C address = 0x%02X\r\n",addr_f);
      }
      if((name=I2C_addr_revlookup(addr_p,busAddrSym))!=NULL){
        printf("Peripheral I2C address = 0x%02X (%s)\r\n",addr_p,name);
      }else{
        printf("Peripheral I2C address = 0x%02X\r\n",addr_p);
      }
    }else{
      if((name=I2C_addr_revlookup(addr_f,busAddrSym))!=NULL){
        printf("I2C address = 0x%02X (%s)\r\n",addr_f,name);
      }else{
        printf("I2C address = 0x%02X\r\n",addr_f);
      }
    }
    return 0;
  }
  if(argc>1){
    printf("Error : too many arguments\r\n");
    return 1;
  }
  addr=getI2C_addr(argv[1],1,busAddrSym);
  if(addr==0xFF){
    return 1;
  }
  //erase address section
  en=ctl_global_interrupts_set(0);
  //first disable watchdog
  WDT_STOP();
  //unlock flash memory
  FCTL3=FWKEY;
  //setup flash for erase
  FCTL1=FWKEY|ERASE;
  //dummy write to indicate which segment to erase
  *((char*)0x01000)=0;
  //enable writing
  FCTL1=FWKEY|WRT;
  //write address
  *((char*)0x01000)=addr;
  //disable writing
  FCTL1=FWKEY;
  //lock flash
  FCTL3=FWKEY|LOCK;
  ctl_global_interrupts_set(en);
  //Kick WDT to restart it
  //WDT_KICK();
  //print out message
  printf("I2C Address Changed. Changes will not take effect until after reset.\r\n");
  return 0;
}

//find devices on the ARCbus
int ARCsearch_Cmd(char **argv,unsigned short argc)
{
  //data for dummy command
  unsigned char buff[BUS_I2C_CRC_LEN+BUS_I2C_HDR_LEN],*ptr,*end;
  //symbolic name of subsystem
  const char* name;
  int i,ret,found=0;
  //setup bogus command
  //TODO: perhaps there should be a PING command that is sort of a no-operation command
  ptr=BUS_cmd_init(buff,CMD_PING);
  //loop through all I2C addresses and send a command to see if there is a device at that address
  for(i=0x00;i<=0x7F;i++)
  {
    //send command
    ret=BUS_cmd_tx(i,buff,0,0);
    if(ret==RET_SUCCESS) //RET_SUCCESS = 0;
    {
      name=I2C_addr_revlookup(i,busAddrSym);
      if(name!=NULL)
      {
        printf("Device Found at ADDR = 0x%02X (%s)\r\n",i,name);
      }else
      {
        printf("Device Found at ADDR = 0x%02X\r\n",i);
      }
      found++;
    }else if(ret!=ERR_I2C_NACK && ret!=ERR_I2C_TX_SELF)
    {
      printf("Error sending to addr 0x%02X : %s \r\nError encountered, aborting scan\r\n",i,BUS_error_str(ret));
      break;
    }
  }
  if(found==0)
  {
    printf("No devices found on the ARCbus\r\n");
  }else
  {
    printf("%i %s found on the ARCbus\r\n",found,found==1?"device":"devices");
  }
  return 0;
}

//table of commands with help
const CMD_SPEC cmd_tbl[]={{"help"," [command]",helpCmd},
                   ARC_ASYNC_PROXY_COMMAND,
                   {"ex", "[arg1] [arg2] ...\r\n\t""Example command to show how arguments are passed",example_command},
                   //{"VidOn", "[arg1] [arg2] ...\r\n\t" "Turn camera video on", VidOn},//in IMG code
                   //{"VidOff", "[arg1] [arg2] ...\r\n\t" "Turn camera video off", VidOff},//in IMG code  
                   {"addr", "[arg1] [arg2] ...\r\n\t" "Get I2C address", addr},
                   {"async", "[arg1] [arg2] ...\r\n\t" "Initialize async for IMG", asyncProxyCmd},
                   ARC_COMMANDS,CTL_COMMANDS,ERROR_COMMANDS,
                   //end of list
                   {NULL,NULL,NULL}};

