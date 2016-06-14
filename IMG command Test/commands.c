//===============================================
// MODIFIED AND DESKTOP VERSION
// THIS VERSION SHOULD NOT BE IN THE Z DRIVE
// THIS IS SOLELY FOR TESTING PURPOSES
// IMAGER TEST
//===============================================

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <msp430.h>
#include <ctl.h>
#include <terminal.h>
#include <ARCbus.h>
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

int cmdPic = 0;

int CamOnCmd(char ** argv, unsigned short argc)
{
  printf("Turning camera sensor on\r\n");
  sensor_on();
  printf("Camera on\r\n");
  return 0;
}

int CamOffCmd(char ** argv, unsigned short argc)
{
  printf("Turning camera sensor off\r\n");
  sensor_off();
  printf("Camera off\r\n");
  return 0;
}

//size of current photo 
int imgSizeCmd(char **argv, unsigned short argc)
{
  const char *ptr="";
  char buff[20];
  uint8_t imgSize = Adafruit_VC0706_getImageSize();
  uint16_t jpgLen = Adafruit_VC0706_frameLength();
  switch (imgSize)
  {
    case VC0706_640x480:
      ptr = "640x480";
      break;
    case VC0706_320x240:
      ptr = "320x240";
      break;
    case VC0706_160x120:
      ptr = "160x120";
      break;
    case 0xFF:
      ptr = "Error: bad snsor response";
      break;
    default:
      printf(buff, "0x%02X", imgSize);
      ptr=buff;
      break;
  }
  printf("Image size: %s\r\n", ptr);
  printf("Buffer contents (bytes): %u\r\n", jpgLen);
  return 0;
}

int picListCmd(char **argv, unsigned short argc)
{
  IMG_DAT *block;
  int found,i,num;
  int res;
  ticker time;
  unsigned short check;
  // locate pictures on SD card
  block=(IMG_DAT*)BUS_get_buffer(CTL_TIMEOUT_DELAY,1000);
  if(block==NULL)
  {
      printf("Error: Buffer busy\r\n");
      return -2;
  }
  //print header
  printf("Slot\tImage\tBlocks\t Timestamp\r\n");
  //look at all the images
  for(i = 0,num=0; i < NUM_IMG_SLOTS; i++)
  {
      //read from SD card
      res=mmcReadBlock(IMG_ADDR_START+i*IMG_SLOT_SIZE,(unsigned char*)block);
      if(res==MMC_SUCCESS)
      {
          //check block type
          if(block->magic==BT_IMG_START)
          {
              //calculate CRC
              check=crc16(block,sizeof(*block)-sizeof(block->CRC));
              //check if CRC's match
              if(check==block->CRC)
              {
                  num++;
                  //time is at the beginning of data
                  time=*(ticker*)block->dat;
                  //print info
                  printf("%4i\t%5i\t%6i\t%10lu\r\n",i,block->num,block->block,time);
              }
          }
      }
      else
      {
          printf("Error Reading from SD card %s\r\n",SD_error_str(res));
          break;
      }
  }
  BUS_free_buffer();
  //check how many images were found
  switch(num)
  {
      case 0:
          printf("\r\nNo images found in memory\r\n");
      break;
      case 1:
          printf("\r\n1 image in memory\r\n");
      break;
      default:
          printf("\r\nThere are %i images in memory\r\n",num);
      break;
  }
  return 0;
}

//NOTE: Still initializes if theres no SDcard
int SDcardCmd(char ** argv, unsigned short argc)
{
  int resp;
  resp=mmc_is_init();
  printf("Initializing SD card\r\n");
  if(mmcInit_card())
  {
    if(resp==MMC_SUCCESS)
    {
      printf("Card Initialized\r\n");
    }else
    {
      printf("Card Not Initialized\r\n%s\r\n",SD_error_str(resp));
    }
    return 0;
  } 
}

int SDcheckCmd(char**argv,unsigned short argc)
{
  int resp;
  resp=mmc_is_init();
  if(resp==MMC_SUCCESS)
  {
    printf("Card Initialized\r\n");
  }else
  {
    printf("Card Not Initialized\r\n%s\r\n",SD_error_str(resp));
  }
  return 0;
}

int SDeraseCmd(char ** argv, unsigned short argc)
{
  int ret;
  //erase data from SD card
  ret=mmcErase(IMG_ADDR_START,IMG_ADDR_END);
  //check return value
  if(ret==MMC_SUCCESS)
  {
      //clear imager variables
      writePic = 0;
      picNum=0;
      //print message
      printf("Picture data erased\r\n");
      //refresh status info
      status_refresh();
  }else
  {
      //print error
      printf("Error erase failed %s\r\n",SD_error_str(ret));
  }
  return 0;
}

int takePicCmd(char** argv, unsigned short argc)
{
  unsigned short e;
  ctl_events_set_clear(&IMG_events, IMG_EV_TAKEPIC, IMG_EV_PIC_TAKEN);
  printf("Control even set clear\r\n");
  e=ctl_events_wait(CTL_EVENT_WAIT_ANY_EVENTS_WITH_AUTO_CLEAR,&IMG_events,IMG_EV_PIC_TAKEN,CTL_TIMEOUT_DELAY,30*1024);
  //check if picture was taken
  if(!(e&IMG_EV_PIC_TAKEN))
  {
    //print error message
    printf("Error timeout occoured\r\n");
    return 1; 
  }else 
  {
    //set picture slot
    cmdPic=writePic;
    printf("Picture taken\r\n");
    return 0;
  }
}

int VidOffCmd(char **argv, unsigned short argc)
{
  printf("Turning video off\r\n");
  if(Adafruit_VC0706_TVoff())
  {
    printf("Video off\r\n");
    return 0;
  }else 
  {
    printf("Error turning video off\r\n");//elaborate?
    return 1;
  }
}

int VidOnCmd(char **argv, unsigned short argc)
{
  printf("Turning video on\r\n");
  if(Adafruit_VC0706_TVon())
  {
      printf("Video on\r\n");
      return 0;
  }else
  {
      printf("Error turning video on\r\n");//elaborate?
      return 1;
  }
}

//table of commands with help
//all commands are found in alphabetical order above
const CMD_SPEC cmd_tbl[]={{"help"," [command]",helpCmd},
                   ARC_COMMANDS,CTL_COMMANDS,ERROR_COMMANDS,
                   {"SDcard", "[arg1] [arg2] ...\r\n\t" "Initialize SD card", SDcardCmd},
                   {"SDcheck", "[arg1] [arg2] ...\r\n\t" "Check SD card initialization", SDcheckCmd},
                   {"SDerase", "[arg1] [arg2] ...\r\n\t" "Erase contents of SD card", SDeraseCmd}, //Used for flight 
                   {"VidOn", "[arg1] [arg2] ...\r\n\t" "Turning video off, testing purposes", VidOnCmd}, //Used for testing 
                   {"VidOff", "[arg1] [arg2] ...\r\n\t" "Turn video on, testing purposes", VidOffCmd}, //Used for testing
                   {"CamOn","[arg1] a[rg2] ...\r\n\t" "Turning camera sensor on, flight purposes", CamOnCmd}, //Used for flight
                   {"CamOff", "[arg1] [arg2] ...\r\n\t" "Turning camera sensor off, flight pruposes", CamOffCmd}, //Used for flight
                   {"takePic", "[arg1] [arg2] ...\r\n\t" "Taking photo", takePicCmd},
                   {"imgSize", "[arg1] [arg2] ...\r\n\t" "Checking image size", imgSizeCmd}, // used for flight 
                   {"picList", "[arg1] [arg2] ...\r\n\t" "List pictures on SD card", picListCmd}, //used for flight 
                   //{"addr", "[arg1] [arg2] ...\r\n\t" "Read address", addrCmd},
                   
                   //end of list
                   {NULL,NULL,NULL}};

//--------------------------TESTING--------------------------//
/*
int addrCmd(char **argv,unsigned short argc){
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
*/
