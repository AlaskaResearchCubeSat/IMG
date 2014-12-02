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


    case 13:
        //read time
        time =dat[3];
        time|=((ticker)dat[2])<<8;
        time|=((ticker)dat[1])<<16;
        time|=((ticker)dat[0])<<24;
        //set alarm
        BUS_set_alarm(BUS_ALARM_0,time,&IMG_events,IMG_EV_TAKEPIC);


        return RET_SUCCESS;
        //Handle imager commands
    case 14:
      // Set the picture slot to the sent value
      writePic = dat[0];
      // Call the take picture event
      ctl_events_set_clear(&IMG_events,IMG_EV_TAKEPIC,0);
      //Return Success
      return RET_SUCCESS;
    case 15:
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
    /*
    if(e&SUB_EV_TIME_CHECK){
      printf("time ticker = %li\r\n",get_ticker_time());
    }*/
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
  unsigned char *block;
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
      // print message
      printf("Booting up imager\r\n");

      // Turn imager on
      P7OUT=BIT0;
      P6OUT^=BIT5;

      Adafruit_VC0706_init();
      Adafruit_VC0706_TVon();
      Adafruit_VC0706_setImageSize(VC0706_640x480);
      // Let the camera boot up for a little bit...
      ctl_timeout_wait(ctl_get_current_time()+500);

      if(!Adafruit_VC0706_takePicture()){
        report_error(ERR_LEV_CRITICAL,ERR_IMG,ERR_IMG_TAKEPIC, 0);
        break;
      }
      
      printf("Saving\r\n");
      // Initialize the SD card
      mmcInit_card();

      if(mmc_is_init() != MMC_SUCCESS)
      {
        report_error(ERR_LEV_CRITICAL,ERR_IMG,ERR_IMG_SD_CARD_INIT, 0);
      }

      // Set nextblock
      nextBlock = writePic * 100;
      
      // Store the image
      piclength = Adafruit_VC0706_frameLength();
      block = BUS_get_buffer(CTL_TIMEOUT_NONE, 0);
      while(piclength > 0){
        unsigned char* buffer;

        int bytesToRead;
        if (piclength < 64){
          bytesToRead = piclength;
        }
        else{
          bytesToRead = 64;
        }
        buffer = Adafruit_VC0706_readPicture(bytesToRead);
        memcpy(block + count*64, buffer, 64); count++;
    
        if (count >= 8){
          count = 0;
          resp = mmcWriteBlock(nextBlock++, block);
          if(resp != MMC_SUCCESS)
          {
            report_error(ERR_LEV_ERROR,ERR_IMG,ERR_IMG_SD_CARD_WRITE, 0);
          }
        }
        if(++writeCount >= 64){
          printf(".");
          writeCount = 0;
        }
      
        piclength -= bytesToRead;
      }
      if (count != 0){
        resp = mmcWriteBlock(nextBlock++, block);
        if(resp != MMC_SUCCESS)
        {
          report_error(ERR_LEV_ERROR,ERR_IMG,ERR_IMG_SD_CARD_WRITE, 0);
        }
      }
      // Reset the buffer to 0
      for(i = 0; i < BUS_get_buffer_size(); i++)
      {
        block[i] = 0;
      }
      // Keep writing blocks until you have written 100 blocks total 
      while(nextBlock % 100 != 0)
      {
        mmcWriteBlock(nextBlock++, block);
      }
      BUS_free_buffer();
      // End storing image

      // Save picture length
      piclength = Adafruit_VC0706_frameLength();

      Adafruit_VC0706_TVoff();
      // Turn imager off
      P7OUT=BIT1;
      P6OUT^=BIT5;

      printf("\n\rDone.\r\n");
    }
    if(e&IMG_EV_LOADPIC){ // Load the picture from the SD card and send it through the bus
      //print message
      //mmcInit_card();
      printf("Loaded picture (%i blocks):\r\n", nextBlock-1);
      // Send 100 packets over.
      for(i = (readPic * 100); i < (readPic * 100) + 2/*100*/; i++)
      {
        buffer=BUS_get_buffer(CTL_TIMEOUT_DELAY,10000);
        //read from SD card
        P6OUT^=BIT6;
        resp=mmcReadBlock(i,buffer);

        if(resp != MMC_SUCCESS)
        {
          report_error(ERR_LEV_ERROR,ERR_IMG,ERR_IMG_SD_CARD_READ, resp);
        }
        else
        {
          
        // Check to make sure this isn't a null packet (all 0s)
        if(!(buffer[0] == 0 && buffer[1] == 0 && buffer[1] == 0 && buffer[2] == 0 && buffer[3] == 0 && buffer[5] == 0 && buffer[8] == 0 && buffer[13] == 0 && buffer[21] == 0))
        {
          // Send three extra bytes telling us where the picture came from, and any errors from the SD card
          buffer[512] = readPic;
          buffer[513]= i%100;
          buffer[514]= resp;
          // Transmit this block across SPI
          resp = BUS_SPI_txrx(srcAddr,buffer,NULL,512 + BUS_SPI_CRC_LEN + 3);
          if(resp != 0) //(RET_SUCCESS)
          {
            report_error(ERR_LEV_ERROR,ERR_IMG,ERR_IMG_TX, resp);
          }

          P6OUT^=BIT6;
          // Wait for a while, to let the packet fully transmit
          ctl_timeout_wait(ctl_get_current_time()+3000);
        }
        }
        BUS_free_buffer();
      }
     

      printf("\r\nDone!\r\n");
    }
  }
}
