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
#include "sensor.h"
#include "IMG.h"

//the tvOff command, turns the video out 'off'
int tvOffCmd(char **argv, unsigned short argc){
  printf("Turning video off\r\n");
  Adafruit_VC0706_TVoff();
  printf("Video off\r\n\n");
}

int savePicCmd(char **argv, unsigned short argc){
    int ret;
    //set picture slot to use
    writePic=0;
    //turn the sensor on
    sensor_on();
    // Let the camera boot up for a little bit...
    ctl_timeout_wait(ctl_get_current_time()+500);
    //take picture
    ret=savepic();
    //check if picture saved correctly
    if(ret==IMG_RET_SUCCESS){
        printf("Image Stored Successfully\r\n");
    }else{
        printf("Error taking picture\r\n");
    }
    //turn the sensor off
    sensor_off();
    return 0;
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
   unsigned short e;
   //set picture slot
   writePic=0;
   //Trigger the takepic event and clear pic taken event
   ctl_events_set_clear(&IMG_events, IMG_EV_TAKEPIC,IMG_EV_PIC_TAKEN);
   //wait for picture to complete, set a timeout of 15 sec
   e=ctl_events_wait(CTL_EVENT_WAIT_ANY_EVENTS_WITH_AUTO_CLEAR,&IMG_events,IMG_EV_PIC_TAKEN,CTL_TIMEOUT_DELAY,30*1024);
   //check if picture was taken
   if(!(e&IMG_EV_PIC_TAKEN)){
       //print error message
       printf("Error timeout occoured\r\n");
   }
   return 0;
}

int dumpPicTask(char **argv,unsigned short argc)
{
  //Trigger the load picture event
  ctl_events_set_clear(&IMG_events, IMG_EV_LOADPIC,0);
}

int picloc_Cmd(char **argv,unsigned short argc){
    //savepic command always saves at IMG_ADDR_START
    printf("%i\r\n",IMG_ADDR_START);
    //everything is always good
    return 0;
}



//table of commands with help
const CMD_SPEC cmd_tbl[]={{"help"," [command]\r\n\t""get a list of commands or help on a spesific command.",helpCmd},
                         CTL_COMMANDS,ARC_COMMANDS,ERROR_COMMANDS,MMC_COMMANDS,
                         {"tvoff","\r\n\t""Turn the Composite video output off",tvOffCmd},
                         {"tvon","\r\n\t""Turn the Composite video output on",tvOnCmd},
                         {"camon","\r\n\t""Trun on power to the image sensor",camOnCmd},
                         {"camoff","\r\n\t""Trun off power to the image sensor",camOffCmd},
                         {"imgsize","\r\n\t""Read the image size",imgSizeCmd},
                         {"resume","\r\n\t""resume previewing after taking a picture",resumeVidCmd},
                         {"savepic","\r\n\t""Save a picture into the SD card",savePicCmd}, 
                         {"pbuff","\r\n\t""Print camera buffer",printBuffCmd},
                         {"version", "\r\n\t""Print camera version", versionCmd},
                         {"takepictask", "\r\n\t""Trigger take pic event", takePicTask},
                         {"loadpictask", "\r\n\t""Trigger load pic event", dumpPicTask},
                         {"picloc", "\r\n\t""Print sector for picture storage", picloc_Cmd},
                         //end of list
                         {NULL,NULL,NULL}};
