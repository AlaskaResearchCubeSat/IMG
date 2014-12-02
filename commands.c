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
#include <commandLib.h>
#include "SDlib.h"
#include "IMG_errors.h"
#include "Adafruit_VC0706.h"
#include "IMG_Events.h"

// This is causing errors but I don't need it for now, so I'm just ignoring it
#define busAddrSym 0

//change the stored I2C address. this does not change the address for the I2C peripheral
int addrCmd(char **argv,unsigned short argc){
  //unsigned long addr;
  //unsigned char tmp;
  //char *end;
  int en;
  unsigned char addr,addr_f,addr_p;
  const char *name;
  if(argc==0){
    addr_f=*(unsigned char*)0x01000;
    addr_p=((~UCGCEN)&UCB0I2COA);
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
  addr=getI2C_addr(argv[1],0,busAddrSym);
  if(addr==0xFF){
    return 1;
  }
  //setup packet 
  ptr=BUS_cmd_init(buff,6);
  //copy strings into buffer for sending
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
  addr=getI2C_addr(argv[1],0,busAddrSym);
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
  printf("\r\nSpamming complete %u chars sent\r\n",n);
  return 0;
}

//the tvOff command, turns the video out 'off'
int tvOffCmd(char **argv, unsigned short argc){
  printf("Turning video off\r\n");
  Adafruit_VC0706_TVoff();
  printf("Video off\r\n\n");
}

int savePicCmd(char **argv, unsigned short argc){
  uint32_t jpglen = Adafruit_VC0706_frameLength();
  int writeCount = 0;
  unsigned char *block;
  int count = 0;
  int nextBlock = 0;
  printf("Storing a ");
  printf("%lu", jpglen);
  printf(" byte image.\r\n");
  
  if((mmc_is_init() == MMC_SUCCESS) && (jpglen != 0)){
    block = BUS_get_buffer(CTL_TIMEOUT_NONE, 0);
    while(jpglen > 0){
      unsigned char* buffer;

      int bytesToRead;
      if (jpglen < 64){
        bytesToRead = jpglen;
      }
      else{
        bytesToRead = 64;
      }
      buffer = Adafruit_VC0706_readPicture(bytesToRead);
      memcpy(block + count*64, buffer, 64); count++;
      
      if (count >= 8){
        count = 0;
        mmcWriteBlock(nextBlock++, block);
      }
      if(++writeCount >= 64){
        printf(".");
        writeCount = 0;
      }
      
      jpglen -= bytesToRead;
    }
  if (count != 0){
    mmcWriteBlock(nextBlock++, block);
  }
  BUS_free_buffer();
  printf("\r\nDone writing image to SD card.\r\n");
  printf("Memory blocks used: ");
  printf("%i", (nextBlock-1));
  printf("\r\n\n");

  }
  else if(mmc_is_init() != MMC_SUCCESS){
    printf("Error: SD Card not initialized\r\n\n");
    return -1;
  }
  else if(jpglen <= 0){
    printf("Error: No image in buffer\r\n\n");
    return -1;
  }
}

//the tvOn command, turns the video output 'on'
int tvOnCmd(char **argv, unsigned short argc){
  printf("Turning video on\r\n");
  Adafruit_VC0706_TVon();
  printf("Video on\r\n\n");
}


int camOnCmd(char **argv, unsigned short argc){
  printf("Turning camera on\r\n");
  // Turn imager on
  P7OUT=BIT0;
  printf("Camera on\r\n\n");
}

int camOffCmd(char **argv, unsigned short argc){
  printf("Turning camera off\r\n");
  // Turn imager off
  P7OUT=BIT1;
  printf("Camera off\r\n\n");
}


//freezes the frame, keeps the picture in the camera buffer 
int takePicCmd(char **argv, unsigned short argc){
  Adafruit_VC0706_setImageSize(VC0706_640x480);
  printf("Taking a picture.. (Frame will freeze on video device)\r\n");
  //Adafruit_VC0706_takePicture();
  if(!Adafruit_VC0706_takePicture()){
    printf("Failed to take picture.\r\n");
  }
  else{
    printf("Picture taken.\r\n\n");
  }
}

//ask the camera how big the current picture is
int imgSizeCmd(char **argv, unsigned short argc){
  uint8_t imgsize = Adafruit_VC0706_getImageSize();
  uint16_t jpglen = Adafruit_VC0706_frameLength();
  printf("Image size: ");
  if (imgsize == VC0706_640x480) printf("640x480\r\n");
  if (imgsize == VC0706_320x240) printf("320x240\r\n");
  if (imgsize == VC0706_160x120) printf("160x120\r\n"); 
  printf("buffer contents size in bytes is: ");
  printf("%u", jpglen);
  printf("\r\n\n");
}

// resumes video feed to camera, picture data will no longer be available in the camera buffer
int resumeVidCmd(char **argv, unsigned short argc){
  printf("Resuming video..\r\n");
  Adafruit_VC0706_resumeVideo();
  printf("Video resumed.\r\n\n");
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
  printf("\r\nSpamming complete\r\n%u chars printed\r\n",c);
  return 0;
}

int versionCmd(char **argv, unsigned short argc){
  printf("Camera version: %s\r\n", Adafruit_VC0706_getVersion());
}

int printBuffCmd(char **argv,unsigned short argc){
  printf("Printing camera buffer: \r\n");
  Adafruit_VC0706_printBuff();
  printf("\r\n Done. \r\n\n");
}


int takePicTask(char **argv,unsigned short argc)
{
  // Call the takepic event
  ctl_events_set_clear(&IMG_events, IMG_EV_TAKEPIC,0);
}

int dumpPicTask(char **argv,unsigned short argc)
{
  // Call the load picture event
  ctl_events_set_clear(&IMG_events, IMG_EV_LOADPIC,0);
}
int ping(char **argv,unsigned short argc)
{
  // Pong!
  printf("Pong!\r\n");
  
}



//table of commands with help
const CMD_SPEC cmd_tbl[]={{"help"," [command]\r\n\t""get a list of commands or help on a spesific command.",helpCmd},
                         CTL_COMMANDS,ARC_COMMANDS,ERROR_COMMANDS,MMC_COMMANDS,
                         //{"addr"," [addr]\r\n\t""Get/Set I2C address.",addrCmd},
                         //{"print"," addr str1 [[str2] ... ]\r\n\t""Send a string to addr.",printCmd},
                         //{"tst"," addr len\r\n\t""Send test data to addr.",tstCmd},
                         //{"async","\r\n\t""Close async connection.",asyncCmd},
                         //{"exit","\r\n\t""Close async connection.",asyncCmd},
                         //{"spam","n\r\n\t""Spam the terminal with n chars",spamCmd},
                         //{"inc","n\r\n\t""Spam the terminal by printing numbers from 0 to n-1",incCmd},
                         {"tvoff","",tvOffCmd},
                         {"tvon","",tvOnCmd},
                         {"camon","",camOnCmd},
                         {"camoff","",camOffCmd},
                         {"imgsize","",imgSizeCmd},
                         {"takepic","",takePicCmd},
                         {"resume","",resumeVidCmd},
                         {"savepic","",savePicCmd}, 
                         {"pbuff","",printBuffCmd},
                         {"version", "", versionCmd},
                         {"takepictask", "", takePicTask},
                         {"loadpictask", "", dumpPicTask},
                         {"ping", "", ping},
                         //end of list
                         {NULL,NULL,NULL}};
