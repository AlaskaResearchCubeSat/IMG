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
#include "sensor.h"

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
  printf("Storing a %lu byte image.\r\n", jpglen);
  
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
  printf("\r\nDone writing image to SD card.\r\n""Memory blocks used: %i\r\n",(nextBlock-1));

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
  // Turn sensor on
  sensor_on();
  printf("Camera on\r\n\n");
}

int camOffCmd(char **argv, unsigned short argc){
  printf("Turning camera off\r\n");
  // Turn sensor off
  sensor_off();
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
  printf("buffer contents size in bytes is: %u\r\n\n", jpglen);
}

// resumes video feed to camera, picture data will no longer be available in the camera buffer
int resumeVidCmd(char **argv, unsigned short argc){
  printf("Resuming video..\r\n");
  Adafruit_VC0706_resumeVideo();
  printf("Video resumed.\r\n\n");
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
  //Trigger the takepic event
  ctl_events_set_clear(&IMG_events, IMG_EV_TAKEPIC,0);
}

int dumpPicTask(char **argv,unsigned short argc)
{
  //Trigger the load picture event
  ctl_events_set_clear(&IMG_events, IMG_EV_LOADPIC,0);
}



//table of commands with help
const CMD_SPEC cmd_tbl[]={{"help"," [command]\r\n\t""get a list of commands or help on a spesific command.",helpCmd},
                         CTL_COMMANDS,ARC_COMMANDS,ERROR_COMMANDS,MMC_COMMANDS,
                         {"tvoff","\r\n\t""Turn the Composite video output off",tvOffCmd},
                         {"tvon","\r\n\t""Turn the Composite video output on",tvOnCmd},
                         {"camon","\r\n\t""Trun on power to the image sensor",camOnCmd},
                         {"camoff","\r\n\t""Trun off power to the image sensor",camOffCmd},
                         {"imgsize","\r\n\t""Read the image size",imgSizeCmd},
                         {"takepic","\r\n\t""Command the camera to take a picture",takePicCmd},
                         {"resume","\r\n\t""resume previewing after taking a picture",resumeVidCmd},
                         {"savepic","\r\n\t""Save a picture into the SD card",savePicCmd}, 
                         {"pbuff","\r\n\t""Print camera buffer",printBuffCmd},
                         {"version", "\r\n\t""Print camera version", versionCmd},
                         {"takepictask", "\r\n\t""Trigger take pic event", takePicTask},
                         {"loadpictask", "\r\n\t""Trigger load pic event", dumpPicTask},
                         //end of list
                         {NULL,NULL,NULL}};
