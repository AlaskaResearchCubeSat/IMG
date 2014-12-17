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
#include <SDlib.h>
#include <crc.h>
#include "IMG_errors.h"
#include "Adafruit_VC0706.h"
#include "sensor.h"
#include "IMG.h"

//the tvOff command, turns the video out 'off'
int tvOffCmd(char **argv, unsigned short argc){
    printf("Turning video off\r\n");
    if(Adafruit_VC0706_TVoff()){
        printf("Video off\r\n");
        return 0;
    }else{
        printf("Error turning video off\r\n");
        return 1;
    }
}

int savePicCmd(char **argv, unsigned short argc){
    int ret;
    //set picture slot to use
    writePic=0;
    //turn the sensor on
    printf("Turning on sensor...\r\n");
    sensor_on();
    //take picture
    printf("Taking Picture please wait\r\n");
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
    if(Adafruit_VC0706_TVon()){
        printf("Video on\r\n");
        return 0;
    }else{
        printf("Error turning video on\r\n");
        return 1;
    }
}


int camOnCmd(char **argv, unsigned short argc){
  printf("Turning camera on\r\n");
  // Turn sensor on
  sensor_on();
  printf("Camera on\r\n");
  return 0;
}

int camOffCmd(char **argv, unsigned short argc){
  printf("Turning camera off\r\n");
  // Turn sensor off
  sensor_off();
  printf("Camera off\r\n");
  return 0;
}

//ask the camera how big the current picture is
int imgSizeCmd(char **argv, unsigned short argc){
    const char *sptr="";
    char buf[20];
    uint8_t imgsize = Adafruit_VC0706_getImageSize();
    uint16_t jpglen = Adafruit_VC0706_frameLength();
    switch(imgsize){
        case VC0706_640x480:
            sptr="640x480";
        break;
        case VC0706_320x240:
            sptr="320x240";
        break;
        case VC0706_160x120:
            sptr="160x120"; 
        break;
        case 0xFF:
            sptr="Error bad response from sensor";
        break;
        default:
            sprintf(buf,"0x%02X",imgsize);
            sptr=buf;
        break;
    }
    printf("Image size: %s\r\n",sptr);
    printf("buffer contents size in bytes is: %u\r\n", jpglen);
    return 0;
}

// resumes video feed to camera, picture data will no longer be available in the camera buffer
int resumeVidCmd(char **argv, unsigned short argc){
    printf("Resuming video..\r\n");
    if(Adafruit_VC0706_resumeVideo()){
        printf("Video resumed.\r\n");
        return 0;
    }else{
        printf("Error resuming video\r\n");
        return 1;
    }
}

int versionCmd(char **argv, unsigned short argc){
    const char *ver;
    //get version
    ver=Adafruit_VC0706_getVersion();
    //check for error
    if(ver!=NULL){
        //print camera version
        printf("Camera version: %s\r\n",ver);
    }else{
        //print error
        printf("Error reading camera version\r\n");
    }
    return 0;
}

int takePicTask(char **argv,unsigned short argc)
{
   unsigned short e;
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
  return 0;
}

int picloc_Cmd(char **argv,unsigned short argc){
    //savepic command always saves at IMG_ADDR_START
    printf("%i\r\n",IMG_ADDR_START+IMG_SLOT_SIZE*writePic);
    //everything is always good
    return 0;
}

int beacon_Cmd(char **argv,unsigned short argc){
    IMG_BEACON dat;
    //make beacon data
    img_make_beacon(&dat);
    //print data
    printf("sd_stat = %s (%i)\r\n",(dat.sd_stat==MMC_SUCCESS)?"Initialized":SD_error_str(dat.sd_stat),dat.sd_stat);
    printf("img_time = %lu\r\n",dat.img_time);
    printf("num = %u\r\n",(unsigned short)dat.num);
    printf("flags = 0x%02X\r\n",(unsigned short)dat.flags);
    return 0;
}

int eraseImg_Cmd(char **argv,unsigned short argc){
    int ret;
    //erase data from SD card
    ret=mmcErase(IMG_ADDR_START,IMG_ADDR_END);
    //check return value
    if(ret==MMC_SUCCESS){
        //clear imager variables
        writePic = 0;
        picNum=0;
        //print message
        printf("Picture data erased\r\n");
    }else{
        //print error
        printf("Error erase failed %s\r\n",SD_error_str(ret));
    }
    return 0;
}

int pictlist_Cmd(char **argv,unsigned short argc){
    IMG_DAT *block;
    int found,i,num;
    int res;
    unsigned short check;
    // locate pictures on SD card
    block=(IMG_DAT*)BUS_get_buffer(CTL_TIMEOUT_DELAY,1000);
    if(block==NULL){
        printf("Error: Buffer busy\r\n");
        return -2;
    }
    //look at all the images
    for(i = 0,num=0; i < NUM_IMG_SLOTS; i++){
        //read from SD card
        res=mmcReadBlock(IMG_ADDR_START+i*IMG_SLOT_SIZE,(unsigned char*)block);
        if(res==MMC_SUCCESS){
            //check block type
            if(block->magic==BT_IMG_START){
                //calculate CRC
                check=crc16(block,sizeof(*block)-sizeof(block->CRC));
                //check if CRC's match
                if(check==block->CRC){
                    num++;
                    //print info
                    printf("slot #%i : Image #%i, %i blocks\r\n",i,block->num,block->block);
                }
            }
        }else{
            printf("Error Reading from SD card %s\r\n",SD_error_str(res));
            break;
        }
    }
    BUS_free_buffer();
    //check how many images were found
    switch(num){
        case 0:
            printf("No images found in memory\r\n");
        break;
        case 1:
            printf("1 image in memory\r\n");
        break;
        default:
            printf("There are %i images in memory\r\n",num);
        break;
    }
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
                         {"version", "\r\n\t""Print camera version", versionCmd},
                         {"takepictask", "\r\n\t""Trigger take pic event", takePicTask},
                         {"loadpictask", "\r\n\t""Trigger load pic event", dumpPicTask},
                         {"picloc", "\r\n\t""Print sector for picture storage", picloc_Cmd},
                         {"beacon","\r\n\t""Generate and print beacon packet",beacon_Cmd},
                         {"eraseImg","\r\n\t""Erase image memory",eraseImg_Cmd},
                         {"pictlist","\r\n\t""List pictures from the SD card",pictlist_Cmd},
                         //end of list
                         {NULL,NULL,NULL}};
