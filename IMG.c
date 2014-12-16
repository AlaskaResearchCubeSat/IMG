#include <msp430.h>
#include <ctl_api.h>
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

CTL_EVENT_SET_t cmd_parse_evt;
// Setup for imager events
CTL_EVENT_SET_t IMG_events;

int readPic,writePic;

//handle subsystem specific commands
int SUB_parseCmd(unsigned char src,unsigned char cmd,unsigned char *dat,unsigned short len){
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
      if(len!=1){
        //packet length is incorrect
        return ERR_PK_LEN;
      }
      // Set the picture slot to the sent value
      readPic = dat[0];
      // Call the load picture event
      ctl_events_set_clear(&IMG_events,IMG_EV_LOADPIC,0);

      //Return Success
      return RET_SUCCESS;
  }
  //Return Error
  return ERR_UNKNOWN_CMD;
}

unsigned int img_make_beacon(IMG_BEACON *dest){
    int i,res;
    char count;
    unsigned short check;
    unsigned char *buffer=NULL;
    IMG_DAT *block;
    //check if card is initialized
    if(mmc_is_init() != MMC_SUCCESS){
        //attempt to reinitialize the card
        dest->sd_stat = mmcReInit_card();
    }else{
        //everything is awesome
        dest->sd_stat = MMC_SUCCESS;
    }

    // count pictures on SD card
    //TODO: make this better
    buffer=BUS_get_buffer(CTL_TIMEOUT_DELAY,1000);
    if(buffer!=NULL){
        for(i = 0,count=0; i < NUM_IMG_SLOTS; i++){
            //read from SD card
            res=mmcReadBlock(IMG_ADDR_START+i*IMG_SLOT_SIZE,buffer);
            if(res==MMC_SUCCESS){
                block=(IMG_DAT*)buffer;
                if(block->magic==BT_IMG_START){
                    //calculate CRC
                    check=crc16(block,sizeof(*block)-sizeof(block->CRC));
                    //check if CRC's match
                    if(check==block->CRC){
                        //incrament picture count
                        count++;
                    }
                }
            }else{
                report_error(ERR_LEV_ERROR,ERR_IMG,ERR_IMG_BEACON_SD_READ,res);
            }
        }
        BUS_free_buffer();
        //set image number
        dest->num = count;
    }else{
        dest->num=-1;
    }

    //get picture time
    dest->img_time=BUS_get_alarm_time(BUS_ALARM_0);

    //TODO: fill in flags
    dest->flags=0;

    return sizeof(IMG_BEACON);
}

void sub_events(void *p) __toplevel{
  unsigned int e,len;
  int i;
  unsigned char buf[10],*ptr;
    
  for(;;){
    e=ctl_events_wait(CTL_EVENT_WAIT_ANY_EVENTS_WITH_AUTO_CLEAR,&SUB_events,SUB_EV_ALL,CTL_TIMEOUT_NONE,0);
    if(e&SUB_EV_PWR_OFF){
      //print message
      puts("System Powering Down\r");
    }
    if(e&SUB_EV_PWR_ON){
      //print message
      puts("System Powering Up\r");
    }
    if(e&SUB_EV_SEND_STAT){
      //send status
      
      //setup packet
      ptr=BUS_cmd_init(buf,CMD_IMG_STAT);
      //fill in beacon packet
      len=img_make_beacon((IMG_BEACON*)ptr);
      
      //send command
      BUS_cmd_tx(BUS_ADDR_CDH,buf,len,0,BUS_I2C_SEND_FOREGROUND);
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

  }
}

// Event for recognizing commands to take/save/dump pictures
void img_events(void *p0) __toplevel{
  unsigned int e;

  readPic = 0;
  writePic = 0;

  for(;;){
    e=ctl_events_wait(CTL_EVENT_WAIT_ANY_EVENTS_WITH_AUTO_CLEAR,&IMG_events,IMG_EV_ALL,CTL_TIMEOUT_NONE,0);
    if(e&IMG_EV_TAKEPIC){// Turn camera on, and then take picture
        //set picture slot to use
        writePic=0;
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
