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
#include <crc.h>

//Size in bytes of block to read from sensor
#define SENSOR_READ_BLOCK_SIZE          (64)

void sensor_init(void){

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
    // Let the camera boot up for a little bit...
    ctl_timeout_wait(ctl_get_current_time()+500);
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
    IMG_DAT *block;
    int j;
    unsigned long baseAddr;
    int blockIdx;
    unsigned char Num;
    unsigned char* buffer=NULL;
    int resp;
    int bytesToRead,blockspace,bytesToWrite;
    
    //generate info message 
    report_error(ERR_LEV_DEBUG,ERR_IMG,INFO_IMG_TAKE_PIC,writePic);
    
    //set image size
    Adafruit_VC0706_setImageSize(VC0706_640x480);
    //take the picture
    if(!Adafruit_VC0706_takePicture()){
        //take picture failed, report error
        report_error(ERR_LEV_ERROR,ERR_IMG,ERR_IMG_TAKEPIC, 0);
        //return error
        return IMG_RET_ERR_TAKEPIC;
    }
    
    //get frame length
    jpglen = Adafruit_VC0706_frameLength();
    //check if there is an image available
    if(jpglen == 0){
        //no image in buffer, report error
        report_error(ERR_LEV_ERROR,ERR_IMG,ERR_IMG_PICSIZE, 0);
        //return error
        return IMG_RET_ERR_PICSIZE;
    }
    
    //get buffer
    block = BUS_get_buffer(CTL_TIMEOUT_DELAY,1000);
    //check if timeout expired
    if(block==NULL){
        //buffer is locked, report error
        report_error(ERR_LEV_ERROR,ERR_IMG,ERR_IMG_BUFFER_BUSY, 0);
        //return error
        return IMG_RET_ERR_BUFFER_BUSY;
    }
    
    // Set block address
    baseAddr = IMG_ADDR_START + writePic * IMG_SLOT_SIZE;
    //advance picture slot
    writePic++;
    //check for wrap around
    if(writePic>=NUM_IMG_SLOTS){
        //wrap around to zero
        writePic=0;
    }
    //advance picture number
    Num=picNum++;
    
    for(i=0,bytesToRead=0,bytesToWrite=0,blockIdx=0;i<jpglen;blockIdx++){
        //check for unwritten bytes
        if(bytesToRead>bytesToWrite){
            //copy into buffer
            memcpy(block->dat, buffer+bytesToWrite, bytesToRead-bytesToWrite);
            //add bytes to image index
            i+=bytesToRead-bytesToWrite;
            //set block index
            j=bytesToRead-bytesToWrite;
        }else{
            j=0;
        }
        for(;j<sizeof(block->dat) && i<jpglen;){
            //check if there is more than SENSOR_READ_BLOCK_SIZE bytes to read
            if (jpglen-i > SENSOR_READ_BLOCK_SIZE){
                //read 64 bytes
                bytesToRead = SENSOR_READ_BLOCK_SIZE;
            }else{
                //calculate number of bytes remaining
                bytesToRead =jpglen-i;
            }
            //get data from sensor
            buffer = Adafruit_VC0706_readPicture(i,bytesToRead);
            //check for errors
            if(buffer==NULL){
                //error reading image data, report error
                report_error(ERR_LEV_ERROR,ERR_IMG,ERR_IMG_READPIC,0);
                //free buffer
                BUS_free_buffer();
                //return error
                return IMG_RET_ERR_READ_PIC_DAT;
            }
            //number of bytes to write
            bytesToWrite=bytesToRead;
            //calculate the number of bytes left in the block
            blockspace=sizeof(block->dat)-(j);
            //check available space in block
            if(bytesToWrite>blockspace){
                //only write enough to fill the block
                bytesToWrite=blockspace;
            }
            //copy into buffer
            memcpy(&(block->dat[j]), buffer, bytesToWrite);
            //add bytes to indexes
            j+=bytesToWrite;
            i+=bytesToWrite;
        }
        //write block fields
        //set block type
        block->magic=(blockIdx==0)?BT_IMG_START:BT_IMG_BODY;
        //set image number
        block->num=Num;
        //set block number
        block->block=(blockIdx==0)?(jpglen+sizeof(block->dat)-1)/sizeof(block->dat):blockIdx;
        //calculate CRC
        block->CRC=crc16(block,sizeof(*block)-sizeof(block->CRC));
        //write block to SD card
        resp = mmcWriteBlock(baseAddr+blockIdx,(unsigned char*)block);
        //check for errors
        if(resp != MMC_SUCCESS){
            //write failed, report error
            report_error(ERR_LEV_ERROR,ERR_IMG,ERR_IMG_SD_CARD_WRITE,resp);
            //free buffer
            BUS_free_buffer();
            //error encountered, abort image store
            return ERR_IMG_SD_CARD_WRITE;
        }
        //toggle LED for block write
        LED_toggle(IMG_RW_LED);
    }
    //free buffer
    BUS_free_buffer();
    //SUCCESS!!
    return IMG_RET_SUCCESS;
}


int loadpic(void){
    int writeCount = 0;
    int count = 0;
    int nextBlock = 0;
    int i,j;
    int img_size;
    int resp;
    unsigned short check;
    IMG_DAT *block;
    unsigned char *buffer=NULL;
    
    //reserve buffer
    buffer=BUS_get_buffer(CTL_TIMEOUT_DELAY,1000);
    //check for errors
    if(buffer==NULL){
        //report error
        report_error(ERR_LEV_ERROR,ERR_IMG,ERR_IMG_LOADPIC_BUFFER,0);
        //return error
        return IMG_RET_ERR_BUFFER_BUSY;
    }
    block=(IMG_DAT*)(buffer+2);
    //Send blocks from image
    for(i = 0,img_size=IMG_SLOT_SIZE; i < img_size; i++){
        //read from SD card
        resp=mmcReadBlock(IMG_ADDR_START+readPic*IMG_SLOT_SIZE+i,buffer);
        //check for errors
        if(resp != MMC_SUCCESS){
            //report error
            report_error(ERR_LEV_ERROR,ERR_IMG,ERR_IMG_SD_CARD_READ, resp);
            //done with buffer, free it
            BUS_free_buffer();
            //return error
            return IMG_RET_ERR_SD_READ;
        }else{
            //check if block type is correct
            if(block->magic==((i==0)?BT_IMG_START:BT_IMG_BODY)){
                //calculate CRC
                check=crc16(block,sizeof(*block)-sizeof(block->CRC));
                //check if CRC's match
                if(check==block->CRC){
                    // Transmit this block across SPI
                    resp = BUS_SPI_txrx(BUS_ADDR_COMM,buffer,NULL,512 + BUS_SPI_CRC_LEN + 3);
                    //check for errors
                    if(resp != RET_SUCCESS){
                        //report error
                        report_error(ERR_LEV_ERROR,ERR_IMG,ERR_IMG_TX, resp);
                        //done with buffer, free it
                        BUS_free_buffer();
                        //return error
                        return IMG_RET_ERR_IMG_TX;
                    }
                    //Toggle read LED
                    LED_toggle(IMG_READ_LED);
                    //get image size in blocks from first block
                    if(i==0){
                        //read image size from first block
                        img_size=block->num;
                    }
                }else{
                    //bad CRC, report error
                    report_error(ERR_LEV_ERROR,ERR_IMG,ERR_IMG_READ_INVALID_CRC,block->CRC);
                    //done with buffer, free it
                    BUS_free_buffer();
                    //return error
                    return IMG_RET_BAD_CRC;
                }
            }else{
                //report error
                report_error(ERR_LEV_ERROR,ERR_IMG,(i==0)?ERR_IMG_READ_START_BLOCK_ID:ERR_IMG_READ_BLOCK_ID,block->magic);
                //done with buffer, free it
                BUS_free_buffer();
                //return error
                return IMG_RET_BAD_BLK_HEADER;
            }
        }
    }
    //done with buffer, free it
    BUS_free_buffer();
    //SUCCESS!!
    return IMG_RET_SUCCESS;
}
