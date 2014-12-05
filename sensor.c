#include <msp430.h>
#include <string.h>
#include "sensor.h"
#include "Adafruit_VC0706.h"
#include <UCA1_uart.h>
#include "LED.h"
#include "IMG.h"
#include "IMG_errors.h"
#include <Error.h>
#include <SDlib.h>


void sensor_init(void){

    //setup UCA1 uart for image communication
    UCA1_init_UART();

    //setup system specific peripherals
    Adafruit_VC0706_init();

    //setup P7.0 for imager on/off
    // Set imager to off to start with (this will save power)
    P7OUT&=BIT0;
    //set pin direction to output
    P7DIR|=BIT0;
    //turn off pull resistor
    P7REN&=BIT0;
    //set pin as GPIO
    P7SEL&=BIT0;
}

void sensor_on(void){
    //power on sensor
    P7OUT|=BIT0;
    //turn power LED on
    LED_on(IMG_PWR_LED);
}

void sensor_off(void){
    //power off sensor
    P7OUT&=~BIT0;
    //turn power LED off
    LED_off(IMG_PWR_LED);
}

int savepic(void){
    uint32_t jpglen,i;
    unsigned char *block;
    int j;
    int blockAddr;
    unsigned char* buffer=NULL;
    int resp;
    int bytesToRead;
    
    //set image size
    Adafruit_VC0706_setImageSize(VC0706_640x480);
    //take the picture
    if(!Adafruit_VC0706_takePicture()){
        report_error(ERR_LEV_CRITICAL,ERR_IMG,ERR_IMG_TAKEPIC, 0);
        printf("Failed to take picture.\r\n");
        return 7;
    }
    
    //get frame length
    jpglen = Adafruit_VC0706_frameLength();
    //print frame length for Matlab    
    printf("Storing a %lu byte image.\r\n", jpglen);
    //check if there is an image available
    if(jpglen == 0){
        printf("Error: No image in buffer\r\n\n");
        return 1;
    }
    
    //get buffer
    block = BUS_get_buffer(CTL_TIMEOUT_DELAY,1000);
    //check if timeout expired
    if(block==NULL){
        printf("Error : buffer busy\r\n");
        return 2;
    }
    
    // Set block address
    blockAddr = IMG_ADDR_START + writePic * IMG_SLOT_SIZE;
    
    for(i=0;i<jpglen;blockAddr++){
        for(j=0;j<512 && i<jpglen;){
            //check if there is more than 64 bytes
            if (jpglen-i > 64){
                //read 64 bytes
                bytesToRead = 64;
            }else{
                //calculate number of bytes remaining
                bytesToRead =jpglen-i;
            }
            //get data from sensor
            buffer = Adafruit_VC0706_readPicture(i,bytesToRead);
            //check for errors
            if(buffer==NULL){
                printf("Error Reading image data. aborting image transfer\r\n");
                report_error(ERR_LEV_ERROR,ERR_IMG,ERR_IMG_READPIC,0);
                BUS_free_buffer();
                return 3;
            }
            //copy into buffer
            memcpy(block + j, buffer, bytesToRead);
            //add bytes to indexes
            j+=bytesToRead;
            i+=bytesToRead;
        }
       //write block to SD card
        resp = mmcWriteBlock(blockAddr,block);
        //check for errors
        if(resp != MMC_SUCCESS){
            report_error(ERR_LEV_ERROR,ERR_IMG,ERR_IMG_SD_CARD_WRITE,resp);
            //free buffer
            BUS_free_buffer();
            //error encountered, abort image store
            return 4;
        }
        //print progress in percent
        printf("\r%4i%%\r",(100*i)/jpglen);
    }
    BUS_free_buffer();
    printf("Done writing image to SD card.\r\n""Memory blocks used: %i\r\n",(blockAddr-1)-IMG_ADDR_START + writePic * IMG_SLOT_SIZE);
    return 0;
}

