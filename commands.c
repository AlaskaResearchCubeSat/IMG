#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <msp430.h>
#include <ctl_api.h>
#include <terminal.h>
#include <ARCbus.h>
#include <UCA1_uart.h>
#include <Error.h>
#include "Proxy_errors.h"

//helper function to parse I2C address
//if res is true reject reserved addresses
unsigned char getI2C_addr(char *str,short res){
  unsigned long addr;
  unsigned char tmp;
  char *end;
  //attempt to parse a numeric address
  addr=strtol(str,&end,0);
  //check for errors
  if(end==str){
    //check for symbolic matches
    if(!strcmp(str,"LEDL")){
      return BUS_ADDR_LEDL;
    }else if(!strcmp(str,"ACDS")){
      return BUS_ADDR_ACDS;
    }else if(!strcmp(str,"COMM")){
      return BUS_ADDR_COMM;
    }else if(!strcmp(str,"IMG")){
      return BUS_ADDR_IMG;
    }else if(!strcmp(str,"CDH")){
      return BUS_ADDR_CDH;
    }else if(!strcmp(str,"GC")){
      return BUS_ADDR_GC;
    }
    //not a known address, error
    printf("Error : could not parse address \"%s\".\r\n",str);
    return 0xFF;
  }
  if(*end!=0){
    printf("Error : unknown sufix \"%s\" at end of address\r\n",end);
    return 0xFF;
  }
  //check address length
  if(addr>0x7F){
    printf("Error : address 0x%04lX is not 7 bits.\r\n",addr);
    return 0xFF;
  }
  //check for reserved address
  tmp=0x78&addr;
  if((tmp==0x00 || tmp==0x78) && res){
    printf("Error : address 0x%02lX is reserved.\r\n",addr);
    return 0xFF;
  }
  //return address
  return addr;
}

//reset a MSP430 on command
int restCmd(char **argv,unsigned short argc){
  unsigned char buff[10];
  unsigned char addr;
  unsigned short all=0;
  int resp;
  //force user to pass no arguments to prevent unwanted resets
  if(argc>1){
    puts("Error : too many arguments\r");
    return -1;
  }
  if(argc!=0){
    if(!strcmp(argv[1],"all")){
      all=1;
      addr=BUS_ADDR_GC;
    }else{
      //get address
      addr=getI2C_addr(argv[1],0);
      if(addr==0xFF){
        return 1;
      }
    }
    //setup packet 
    BUS_cmd_init(buff,CMD_RESET);
    resp=BUS_cmd_tx(addr,buff,0,0,BUS_I2C_SEND_FOREGROUND);
    switch(resp){
      case 0:
        puts("Command Sent Sucussfully.\r");
      break;
      case ERR_TIMEOUT:
        puts("IIC timeout Error.\r");
      break;
    }
  }
  //reset if no arguments given or to reset all boards
  if(argc==0 || all){
    //wait for UART buffer to empty
    while(UCA1_CheckBusy());
    //write to WDTCTL without password causes PUC
    reset(ERR_LEV_INFO,PROXY_ERR_SRC_CMD,CMD_ERR_RESET,0);
    //Never reached due to reset
    puts("Error : Reset Failed!\r");
  }
  return 0;
}

//set priority for tasks on the fly
int priorityCmd(char **argv,unsigned short argc){
  extern CTL_TASK_t *ctl_task_list;
  int i,found=0;
  CTL_TASK_t *t=ctl_task_list;
  if(argc<1 || argc>2){
    printf("Error: %s takes one or two arguments, but %u are given.\r\n",argv[0],argc);
    return -1;
  }
  while(t!=NULL){
    if(!strcmp(t->name,argv[1])){
      found=1;
      //match found, break
      break;
    }
    t=t->next;
  }
  //check that a task was found
  if(found==0){
      //no task found, return
      printf("Error: could not find task named %s.\r\n",argv[1]);
      return -3;
  }
  //print original priority
  printf("\"%s\" priority = %u\r\n",t->name,t->priority);
  if(argc==2){
      unsigned char val=atoi(argv[2]);
      if(val==0){
        printf("Error: invalid priority.\r\n");
        return -2;
      }
      //set priority
      ctl_task_set_priority(t,val);
      //print original priority
      printf("new \"%s\" priority = %u\r\n",t->name,t->priority);
  }
  return 0;
}

//get/set ctl_timeslice_period
int timesliceCmd(char **argv,unsigned short argc){
  if(argc>1){
    printf("Error: too many arguments.\r\n");
    return 0;
  }
  //if one argument given then set otherwise get
  if(argc==1){
    int en;
    CTL_TIME_t val=atol(argv[1]);
    //check value
    if(val==0){
      printf("Error: bad value.\r\n");
      return -1;
    }
    //disable interrupts so that opperation is atomic
    en=ctl_global_interrupts_set(0);
    ctl_timeslice_period=val;
    ctl_global_interrupts_set(en);
  }
  printf("ctl_timeslice_period = %ul\r\n",ctl_timeslice_period);
  return 0;
}

//return state name
const char *stateName(unsigned char state){
  switch(state){
    case CTL_STATE_RUNNABLE:
      return "CTL_STATE_RUNNABLE";
    case CTL_STATE_TIMER_WAIT:
      return "CTL_STATE_TIMER_WAIT";
    case CTL_STATE_EVENT_WAIT_ALL|CTL_STATE_TIMER_WAIT:
    case CTL_STATE_EVENT_WAIT_ALL:
      return "CTL_STATE_EVENT_WAIT_ALL";
    case CTL_STATE_EVENT_WAIT_ALL_AC|CTL_STATE_TIMER_WAIT:
    case CTL_STATE_EVENT_WAIT_ALL_AC:
      return "CTL_STATE_EVENT_WAIT_ALL_AC";
    case CTL_STATE_EVENT_WAIT_ANY|CTL_STATE_TIMER_WAIT:
    case CTL_STATE_EVENT_WAIT_ANY:
      return "CTL_STATE_EVENT_WAIT_ANY";
    case CTL_STATE_EVENT_WAIT_ANY_AC|CTL_STATE_TIMER_WAIT:
    case CTL_STATE_EVENT_WAIT_ANY_AC:
      return "CTL_STATE_EVENT_WAIT_ANY_AC";
    case CTL_STATE_SEMAPHORE_WAIT|CTL_STATE_TIMER_WAIT:
    case CTL_STATE_SEMAPHORE_WAIT:
      return "CTL_STATE_SEMAPHORE_WAIT";
    case CTL_STATE_MESSAGE_QUEUE_POST_WAIT|CTL_STATE_TIMER_WAIT:
    case CTL_STATE_MESSAGE_QUEUE_POST_WAIT:
      return "CTL_STATE_MESSAGE_QUEUE_POST_WAIT";
    case CTL_STATE_MESSAGE_QUEUE_RECEIVE_WAIT|CTL_STATE_TIMER_WAIT:
    case CTL_STATE_MESSAGE_QUEUE_RECEIVE_WAIT:
      return "CTL_STATE_MESSAGE_QUEUE_RECEIVE_WAIT";
    case CTL_STATE_MUTEX_WAIT|CTL_STATE_TIMER_WAIT:
    case CTL_STATE_MUTEX_WAIT:
      return "CTL_STATE_MUTEX_WAIT";
    case CTL_STATE_SUSPENDED:
      return "CTL_STATE_SUSPENDED";
    default:
      printf("Unknown Stat : 0x%02X\r\n",state);
      return "unknown state";
  }
}

//print the status of all tasks in a table
int statsCmd(char **argv,unsigned short argc){
  extern CTL_TASK_t *ctl_task_list;
  int i;
  CTL_TASK_t *t=ctl_task_list;
  //format string
  const char *fmt="%-10s\t%u\t\t%s%c%-35s\t%lu\r\n";
  //print out nice header
  printf("\r\nName\t\tPriority\t   State\t\t\t\tTime\r\n---------------------------------------------------------------------------------\r\n");
  //loop through tasks and print out info
  while(t!=NULL){
    printf(fmt,t->name,t->priority,(t->state&CTL_STATE_TIMER_WAIT)?"tw":"  ",(t==ctl_task_executing)?'*':' ',stateName(t->state),t->execution_time);
    t=t->next;
  }
  //add a blank line after table
  printf("\r\n");
  return 0;
}


//change the stored I2C address. this does not change the address for the I2C peripheral
int addrCmd(char **argv,unsigned short argc){
  //unsigned long addr;
  //unsigned char tmp;
  //char *end;
  int en;
  unsigned char addr;
  if(argc==0){
    if(*(unsigned char*)0x01000!=((~UCGCEN)&UCB0I2COA)){
        printf("Flash I2C address = 0x%02X\r\nPeripheral I2C address = 0x%02X\r\n",*(unsigned char*)0x01000,((~UCGCEN)&UCB0I2COA));
    }else{
      printf("I2C address = 0x%02X\r\n",*(unsigned char*)0x01000);
    }
    return 0;
  }
  if(argc>1){
    printf("Error : too many arguments\r\n");
    return 1;
  }
  addr=getI2C_addr(argv[1],1);
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

//transmit command over I2C
int txCmd(char **argv,unsigned short argc){
  unsigned char buff[10],*ptr,id;
  unsigned char addr;
  unsigned short len;
  unsigned int e;
  char *end;
  int i,resp,nack=BUS_CMD_FL_NACK;
  if(!strcmp(argv[1],"noNACK")){
    nack=0;
    //shift arguments
    argv[1]=argv[0];
    argv++;
    argc--;
  }
  //check number of arguments
  if(argc<2){
    printf("Error : too few arguments.\r\n");
    return 1;
  }
  if(argc>sizeof(buff)){
    printf("Error : too many arguments.\r\n");
    return 2;
  }
  //get address
  addr=getI2C_addr(argv[1],0);
  if(addr==0xFF){
    return 1;
  }
  //get packet ID
  id=strtol(argv[2],&end,0);
  if(end==argv[2]){
      printf("Error : could not parse element \"%s\".\r\n",argv[2]);
      return 2;
  }
  if(*end!=0){
    printf("Error : unknown sufix \"%s\" at end of element \"%s\"\r\n",end,argv[2]);
    return 3;
  }
  //setup packet 
  ptr=BUS_cmd_init(buff,id);
  //pares arguments
  for(i=0;i<argc-2;i++){
    ptr[i]=strtol(argv[i+3],&end,0);
    if(end==argv[i+1]){
        printf("Error : could not parse element \"%s\".\r\n",argv[i+3]);
        return 2;
    }
    if(*end!=0){
      printf("Error : unknown sufix \"%s\" at end of element \"%s\"\r\n",end,argv[i+3]);
      return 3;
    }
  }
  len=i;
  resp=BUS_cmd_tx(addr,buff,len,nack,BUS_I2C_SEND_FOREGROUND);
  switch(resp){
    case RET_SUCCESS:
      printf("Command Sent Sucussfully.\r\n");
    break;
  }
  //check if an error occured
  if(resp<0){
    printf("Error : unable to send command\r\n");
  }
  printf("Resp = %i\r\n",resp);
  return 0;
}

//Send data over SPI
int spiCmd(char **argv,unsigned short argc){
  unsigned char addr;
  char *end;
  unsigned short crc;
  //static unsigned char rx[2048+2];
  unsigned char *rx=NULL;
  int resp,i,len=100;
  if(argc<1){
    printf("Error : too few arguments.\r\n");
    return 3;
  }
  //get address
  addr=getI2C_addr(argv[1],0);
  if(addr==0xFF){
    return 1;
  }
  if(argc>=2){
    //Get packet length
    len=strtol(argv[2],&end,0);
    if(end==argv[2]){
        printf("Error : could not parse length \"%s\".\r\n",argv[2]);
        return 2;
    }
    if(*end!=0){
      printf("Error : unknown sufix \"%s\" at end of length \"%s\"\r\n",end,argv[2]);
      return 3;
    }    
    if(len+2>BUS_get_buffer_size()){
      printf("Error : length is too long. Maximum Length is %u\r\n",BUS_get_buffer_size());
      return 4;
    }
  }
  //get buffer, set a timeout of 2 secconds
  rx=BUS_get_buffer(CTL_TIMEOUT_DELAY,2048);
  //check for error
  if(rx==NULL){
    printf("Error : Timeout while waiting for buffer.\r\n");
    return -1;
  }
  //fill buffer with "random" data
  for(i=0;i<len;i++){
    rx[i]=i;
  }
  //send data
  //TESTING: set pin high
  P8OUT|=BIT0;
  //send SPI data
  resp=BUS_SPI_txrx(addr,rx,rx,len);
  //TESTING: wait for transaction to fully complete
  while(UCB0STAT&UCBBUSY);
  //TESTING: set pin low
  P8OUT&=~BIT0;
  //check return value
  if(resp==RET_SUCCESS){
      //print out data message
      printf("SPI data recived\r\n");
      //print out data
      for(i=0;i<len;i++){
        //printf("0x%02X ",rx[i]);
        printf("%03i ",rx[i]);
      }
      printf("\r\n");
  }else{
    printf("%s\r\n",BUS_error_str(resp));
  }
  //free buffer
  BUS_free_buffer();
  return 0;
}

int printCmd(char **argv,unsigned short argc){
  unsigned char buff[40],*ptr,id;
  unsigned char addr;
  unsigned short len;
  int i,j,k;
  //check number of arguments
  if(argc<2){
    printf("Error : too few arguments.\r\n");
    return 1;
  }
  //get address
  addr=getI2C_addr(argv[1],0);
  if(addr==0xFF){
    return 1;
  }
  //setup packet 
  ptr=BUS_cmd_init(buff,6);
  //coppy strings into buffer for sending
  for(i=2,k=0;i<=argc && k<sizeof(buff);i++){
    j=0;
    while(argv[i][j]!=0){
      ptr[k++]=argv[i][j++];
    }
    ptr[k++]=' ';
  }
  //get length
  len=k;
  //TESTING: set pin high
  P8OUT|=BIT0;
  //send command
  BUS_cmd_tx(addr,buff,len,0,BUS_I2C_SEND_FOREGROUND);
  //TESTING: set pin low
  P8OUT&=~BIT0;
  return 0;
}

int tstCmd(char **argv,unsigned short argc){
  unsigned char buff[40],*ptr,*end;
  unsigned char addr;
  unsigned short len;
  int i,j,k;
  //check number of arguments
  if(argc<2){
    printf("Error : too few arguments.\r\n");
    return 1;
  }
  if(argc>2){
    printf("Error : too many arguments.\r\n");
    return 1;
  }
  //get address
  addr=getI2C_addr(argv[1],0);
  len = atoi(argv[2]);
  /*if(len<0){
    printf("Error : bad length");
    return 2;
  }*/
  //setup packet 
  ptr=BUS_cmd_init(buff,7);
  //fill packet with dummy data
  for(i=0;i<len;i++){
    ptr[i]=i;
  }
  //TESTING: set pin high
  P8OUT|=BIT0;
  //send command
  BUS_cmd_tx(addr,buff,len,0,BUS_I2C_SEND_FOREGROUND);
  //TESTING: wait for transaction to fully complete
  while(UCB0STAT&UCBBUSY);
  //TESTING: set pin low
  P8OUT&=~BIT0;
  return 0;
}

//print current time
int timeCmd(char **argv,unsigned short argc){
  printf("time ticker = %li\r\n",get_ticker_time());
  return 0;
}

int asyncCmd(char **argv,unsigned short argc){
   char c;
   int err;
   CTL_EVENT_SET_t e=0,evt;
   unsigned char addr;
   if(argc>0){
    printf("Error : %s takes no arguments\r\n",argv[0]);
    return -1;
  }
  if(err=async_close()){
    printf("\r\nError : async_close() failed : %s\r\n",BUS_error_str(err));
  }
}

const char spamdat[]="This is a spam test\r\n";

int spamCmd(char **argv,unsigned short argc){
  unsigned int n,i,j;
  if(argc<1 || argc==0){
    printf("Error : %s takes only one argument but %i given\r\n",argv[0],argc);
    return -1;
  }
  n=atoi(argv[1]);
  for(i=0,j=0;i<n;i++,j++){
    if(j>=sizeof(spamdat)){
      j=0;
    }
    async_TxChar(spamdat[j]);
  }
  //pause to let chars clear
  ctl_timeout_wait(ctl_get_current_time()+100);
  //print message
  printf("\r\nSpaming complete %u chars sent\r\n",n);
  return 0;
}

int incCmd(char **argv,unsigned short argc){
  unsigned int n,i,c;
  if(argc<1 || argc==0){
    printf("Error : %s takes only one argument but %i given\r\n",argv[0],argc);
    return -1;
  }
  n=atoi(argv[1]);
  for(i=0;i<n;i++){
    c+=printf("%u \r\n",i);
  }
  //pause to let chars clear
  ctl_timeout_wait(ctl_get_current_time()+100);
  //print message
  printf("\r\nSpaming complete\r\n%u chars printed\r\n",c);
  return 0;
}



//table of commands with help
const CMD_SPEC cmd_tbl[]={{"help"," [command]\r\n\t""get a list of commands or help on a spesific command.",helpCmd},
                         {"priority"," task [priority]\r\n\t""Get/set task priority.",priorityCmd},
                         {"timeslice"," [period]\r\n\t""Get/set ctl_timeslice_period.",timesliceCmd},
                         {"stats","\r\n\t""Print task status",statsCmd},
                         {"reset","\r\n\t""reset the msp430.",restCmd},
                         {"addr"," [addr]\r\n\t""Get/Set I2C address.",addrCmd},
                         {"tx"," [noACK] [noNACK] addr ID [[data0] [data1]...]\r\n\t""send data over I2C to an address",txCmd},
                         {"SPI","addr [len]\r\n\t""Send data using SPI.",spiCmd},
                         {"print"," addr str1 [[str2] ... ]\r\n\t""Send a string to addr.",printCmd},
                         {"tst"," addr len\r\n\t""Send test data to addr.",tstCmd},
                         {"time","\r\n\t""Return current time.",timeCmd},
                         {"async","\r\n\t""Close async connection.",asyncCmd},
                         {"exit","\r\n\t""Close async connection.",asyncCmd},
                         {"spam","n\r\n\t""Spam the terminal with n chars",spamCmd},
                         {"inc","n\r\n\t""Spam the by printing numbers from 0 to n-1",incCmd},
                         //end of list
                         {NULL,NULL,NULL}};
