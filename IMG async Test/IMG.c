#include <msp430.h>
#include <ctl.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <ARCbus.h>
#include <UCA1_uart.h>
#include <SDlib.h>
#include "timerA.h"
#include <crc.h>
#include "Adafruit_VC0706.h"
#include "Error.h"
#include "IMG_errors.h"
#include "LED.h"
#include "sensor.h"
#include "IMG.h"
#include "status.h"

//CTL_EVENT_SET_t cmd_parse_evt;
// Setup for imager events
CTL_EVENT_SET_t IMG_events;

int readPic,writePic;
unsigned char picNum,readBlock;
int IMG_parse_cmd(unsigned char src,unsigned char cmd,unsigned char *dat,unsigned short len,unsigned char flags);

CMD_PARSE_DAT IMG_parse={IMG_parse_cmd,CMD_PARSE_ADDR1,BUS_PRI_NORMAL,NULL};

//Function for dumpPicTask
//handle subsystem specific commands
int IMG_parse_cmd(unsigned char src,unsigned char cmd,unsigned char *dat,unsigned short len,unsigned char flags){
  int i;
  int result = 0;
  ticker time;
  switch(cmd){

    case CMD_IMG_TAKE_TIMED_PIC:
        //check packet length
        if(len!=4){
            //packet length is incorrect
            return ERR_PK_LEN;
        }
        //read time
        time =dat[3];
        time|=((ticker)dat[2])<<8;
        time|=((ticker)dat[1])<<16;
        time|=((ticker)dat[0])<<24;
        //set alarm
        BUS_set_alarm(BUS_ALARM_0,time,&IMG_events,IMG_EV_TAKEPIC);

        return RET_SUCCESS;
        //Handle imager commands
    case CMD_IMG_TAKE_PIC_NOW:
      //check packet length
      if(len!=0){
        //packet length is incorrect
        return ERR_PK_LEN;
      }
      // Call the take picture event
      ctl_events_set_clear(&IMG_events,IMG_EV_TAKEPIC,0);
      //Return Success
      return RET_SUCCESS;
    case CMD_IMG_READ_PIC:
      //check packet length
      if(len!=2){
        //packet length is incorrect
        return ERR_PK_LEN;
      }
      // Get the picture to read
      readPic = dat[0];
      // Get the block to read
      readBlock = dat[1];
      // Call the load picture event
      ctl_events_set_clear(&IMG_events,IMG_EV_LOADPIC,0);

      //Return Success
      return RET_SUCCESS;
      case CMD_IMG_CLEARPIC:
        if(len!=0){
          return ERR_PK_LEN;
        }
        //free the picture alarm
        BUS_free_alarm(BUS_ALARM_0);
      return RET_SUCCESS;
  }
  //Return Error
  return ERR_UNKNOWN_CMD;
}

//Function for takPicTask
void sub_events(void *p) __toplevel
{
  unsigned int e,len;
  int i;
  unsigned char buf[10],*ptr;
    
  for(;;)
  {
    e=ctl_events_wait(CTL_EVENT_WAIT_ANY_EVENTS_WITH_AUTO_CLEAR,&SUB_events,SUB_EV_ALL,CTL_TIMEOUT_NONE,0);
    if(e&SUB_EV_PWR_OFF)
    {
      //print message
      puts("System Powering Down\r");
    }
    if(e&SUB_EV_PWR_ON)
    {
      //print message
      puts("System Powering Up\r");
    }
    if(e&SUB_EV_SEND_STAT){
      //send status -----------?
      
      //setup packet
      ptr=BUS_cmd_init(buf,CMD_IMG_STAT);
      //fill in beacon packet
      len=img_make_beacon((IMG_BEACON*)ptr);
      
      //wait a bit so packets don't clash
      ctl_timeout_wait(ctl_get_current_time()+17);
      
      //send command
      BUS_cmd_tx(BUS_ADDR_CDH,buf,len,0);
    }
    if(e&SUB_EV_SPI_DAT){
      puts("SPI data recived:\r");
      //get length
      len=arcBus_stat.spi_stat.len;
      //print out data
      for(i=0;i<len;i++){
        //printf("0x%02X ",rx[i]);
        printf("%03i ",arcBus_stat.spi_stat.rx[i]);
      }
      printf("\r\n");
      //free buffer
      BUS_free_buffer_from_event();
    }
    if(e&SUB_EV_SPI_ERR_CRC){
      puts("SPI bad CRC\r");
    }
    if(e&SUB_EV_SPI_ERR_BUSY){
      puts("SPI buffer busy\r");
    }

  }
}

// Event for recognizing commands to take/save/dump pictures
void img_events(void *p0) __toplevel
{
  unsigned int e;
  readPic = 0;
  writePic = 0;
  picNum=0;
  status_init();//read picture info into status

  for(;;)
  {
    e=ctl_events_wait(CTL_EVENT_WAIT_ANY_EVENTS_WITH_AUTO_CLEAR,&IMG_events,IMG_EV_ALL,CTL_TIMEOUT_NONE,0);
    if(e&IMG_EV_TAKEPIC){// Turn camera on, and then take picture
        //set in progress event
        ctl_events_set_clear(&IMG_events,IMG_EV_INPROGRESS,0);
        //turn the sensor on
        sensor_on();
        //take picture
        savepic();
        //turn the sensor off
        sensor_off();
        //clear in progress event and set pic taken event
        ctl_events_set_clear(&IMG_events,IMG_EV_PIC_TAKEN,IMG_EV_INPROGRESS);
    }
    if(e&IMG_EV_LOADPIC){ // Load the picture from the SD card and send it through the bus
      loadpic();
    }
  }
}
