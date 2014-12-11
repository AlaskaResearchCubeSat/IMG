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
#include "Error.h"
#include "IMG_errors.h"
#include "LED.h"
#include "sensor.h"
#include "IMG.h"

CTL_EVENT_SET_t cmd_parse_evt;
// Setup for imager events
CTL_EVENT_SET_t IMG_events;

int readPic,writePic;
unsigned char srcAddr;

//handle subsystem specific commands
int SUB_parseCmd(unsigned char src,unsigned char cmd,unsigned char *dat,unsigned short len){
  int i;
  int result = 0;
  ticker time;
  switch(cmd){

    case CMD_IMG_TAKE_TIMED_PIC:
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
      // Set the picture slot to the sent value
      writePic = dat[0];
      // Call the take picture event
      ctl_events_set_clear(&IMG_events,IMG_EV_TAKEPIC,0);
      //Return Success
      return RET_SUCCESS;
    case CMD_IMG_READ_PIC:
      // Set the picture slot to the sent value
      readPic = dat[0];
      srcAddr = src;
      // Call the load picture event
      ctl_events_set_clear(&IMG_events,IMG_EV_LOADPIC,0);

      //Return Success
      return RET_SUCCESS;
  }
  //Return Error
  return ERR_UNKNOWN_CMD;
}

void sub_events(void *p) __toplevel{
  unsigned int e,len;
  int i;
  char count;
  char *buffer=NULL;
  unsigned char buf[10],*ptr;
  extern unsigned char async_addr;
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
      //puts("Sending status\r");
      //setup packet 
      if(mmc_is_init() == MMC_SUCCESS){
        buf[0] = 1;
      }else{
        buf[0] = 0;
      }


      // count pictures on SD card
      buffer=BUS_get_buffer(CTL_TIMEOUT_DELAY,10000);
      for(i = 0; i < 25500; i+=100)
      {
        //read from SD card
        mmcReadBlock(i,(unsigned char*)buffer);
        
        if(buffer[0] == 255)
          count++;
      }
      BUS_free_buffer();
      buf[1] = count;

      
      ptr=BUS_cmd_init(buf,CMD_IMG_STAT);
      //TODO: fill in telemetry data
      //send command
      BUS_cmd_tx(BUS_ADDR_CDH,buf,2,0,BUS_I2C_SEND_FOREGROUND);
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
  // piclength would be here, but it needs to be global (?)
  uint32_t piclength;
  int writeCount = 0;
  int count = 0;
  int nextBlock = 0;
  int i;
  int j;
  int resp;

  unsigned char *buffer=NULL;


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
        // Let the camera boot up for a little bit...
        ctl_timeout_wait(ctl_get_current_time()+500);
        //take picture
        savepic();
        //turn the sensor off
        sensor_off();
        //clear in progress event and set pic taken event
        ctl_events_set_clear(&IMG_events,IMG_EV_PIC_TAKEN,IMG_EV_INPROGRESS);
    }
    if(e&IMG_EV_LOADPIC){ // Load the picture from the SD card and send it through the bus
      //print message
      //mmcInit_card();
      printf("Loaded picture (%i blocks):\r\n", nextBlock-1);
      //reserve buffer
      buffer=BUS_get_buffer(CTL_TIMEOUT_DELAY,10000);
      //Send blocks from image
      for(i = 0; i < IMG_SLOT_SIZE; i++)
      {
        LED_toggle(IMG_READ_LED);
        //read from SD card
        resp=mmcReadBlock(IMG_ADDR_START+readPic*IMG_SLOT_SIZE+i,buffer);

        if(resp != MMC_SUCCESS){
          report_error(ERR_LEV_ERROR,ERR_IMG,ERR_IMG_SD_CARD_READ, resp);
        }else{
            // Check to make sure this isn't a null packet (all 0s)
            if(!(buffer[0] == 0 && buffer[1] == 0 && buffer[1] == 0 && buffer[2] == 0 && buffer[3] == 0 && buffer[5] == 0 && buffer[8] == 0 && buffer[13] == 0 && buffer[21] == 0))
            {
              // Send three extra bytes telling us where the picture came from, and any errors from the SD card
              buffer[512] = readPic;
              buffer[513]= i%100;
              buffer[514]= resp;
              // Transmit this block across SPI
              resp = BUS_SPI_txrx(srcAddr,buffer,NULL,512 + BUS_SPI_CRC_LEN + 3);
              if(resp != RET_SUCCESS){
                report_error(ERR_LEV_ERROR,ERR_IMG,ERR_IMG_TX, resp);
              }

              LED_toggle(IMG_READ_LED);
              // Wait for a while, to let the packet fully transmit
              ctl_timeout_wait(ctl_get_current_time()+3000);
            }
        }
      }
      //done with buffer, free it
      BUS_free_buffer();
     

      printf("\r\nDone!\r\n");
    }
  }
}
