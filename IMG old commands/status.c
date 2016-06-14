#include <msp430.h>
#include <string.h>
#include <ctype.h>
#include <SDlib.h>
#include <crc.h>
#include "Error.h"
#include "IMG_errors.h"
#include "LED.h"
#include "sensor.h"
#include "IMG.h"
#include "status.h"


IMG_BEACON status;
CTL_MUTEX_t stat_mutex;

//update status for start of picture
void stat_pic_start(void){
    //set picture in progress flag
    //only updating one thing so don't lock mutex
    status.flags|=IMG_BEACON_FLAGS_PIC_IN_PROGRESS;
}

//update status if picture aborted
void stat_pic_abort(void){
    //clear picture in progress flag
    //only updating one thing so don't lock mutex
    status.flags&=~IMG_BEACON_FLAGS_PIC_IN_PROGRESS;
}

//clear in progress flag and update to newest picture number
void stat_pic_complete(unsigned char num){
    //lcok status info
    ctl_mutex_lock(&stat_mutex,CTL_TIMEOUT_NONE,0);
    //set number
    status.num=num;
    //set image present flag
    status.flags|=IMG_BEACON_FLAGS_HAVE_PIC;
    //clear picture in progress flag
    status.flags&=~IMG_BEACON_FLAGS_PIC_IN_PROGRESS;
    //done with status information
    ctl_mutex_unlock(&stat_mutex);
}

//find pictures on SD card
void status_refresh(void){
    int i,res;
    unsigned short check;
    unsigned short Num;
    unsigned char *buffer=NULL;
    IMG_DAT *block;
    int slot;
    //lcok status info
    ctl_mutex_lock(&stat_mutex,CTL_TIMEOUT_NONE,0);
    //check if card is initialized
    if(mmc_is_init() != MMC_SUCCESS){
        //attempt to reinitialize the card
        status.sd_stat = mmcReInit_card();
        //can't read from card if card is not initialized
        return;
    }else{
        //everything is awesome
        status.sd_stat = MMC_SUCCESS;
    }
    //clear have pic flag
    status.flags&=~IMG_BEACON_FLAGS_HAVE_PIC;
    // count pictures on SD card
    buffer=BUS_get_buffer(CTL_TIMEOUT_DELAY,1000);
    if(buffer!=NULL){
        for(i = 0,Num=0,slot=0; i < NUM_IMG_SLOTS; i++){
            //read from SD card
            res=mmcReadBlock(IMG_ADDR_START+i*IMG_SLOT_SIZE,buffer);
            if(res==MMC_SUCCESS){
                block=(IMG_DAT*)buffer;
                if(block->magic==BT_IMG_START){
                    //calculate CRC
                    check=crc16(block,sizeof(*block)-sizeof(block->CRC));
                    //check if CRC's match
                    if(check==block->CRC){
                        status.flags|=IMG_BEACON_FLAGS_HAVE_PIC;
                        //check if picture number is greater than Num
                        if(block->num>Num){
                            Num=block->num;
                            slot=i;
                        }
                    }
                }
            }else{
                report_error(ERR_LEV_ERROR,ERR_IMG,ERR_IMG_BEACON_SD_READ,res);
            }
        }
        BUS_free_buffer();
        //set image number
        status.num = Num;
        picNum=Num;
        writePic=slot;
    }else{
        status.num=-1;
    }
    //done with status information
    ctl_mutex_unlock(&stat_mutex);  
}

void status_init(void){
    //init mutex
    ctl_mutex_init(&stat_mutex);
    //lock card so that we can search uninterrupted
    //also allows for other tasks to read the SD card on startup
    mmcLock(CTL_TIMEOUT_NONE,0);
    //Search for pictures on card
    status_refresh();
    //done using card, unlock
    mmcUnlock();
}

unsigned int img_make_beacon(IMG_BEACON *dest){
    unsigned int locked;
    //attempt to lock status info
    locked=ctl_mutex_lock(&stat_mutex,CTL_TIMEOUT_DELAY,10);
    //status needs to be generated so don't wait if it is not ready
    //check if card is initialized
    if(mmc_is_init() != MMC_SUCCESS){
        //TODO: schedule this in the future
        //attempt to reinitialize the card
        status.sd_stat = mmcReInit_card();
    }else{
        //everything is awesome
        status.sd_stat = MMC_SUCCESS;
    }

    //get picture time
    status.img_time=BUS_get_alarm_time(BUS_ALARM_0);
    
    //check if picture is scheduled
    if(ERR_BUSY==BUS_alarm_is_free(BUS_ALARM_0)){
        //A picture is scheduled
        status.flags|=IMG_BEACON_FLAGS_PIC_SCH;
    }else{
        //no picture is scheduled
        status.flags&=~IMG_BEACON_FLAGS_PIC_SCH;
    }

    //copy data into structure
    memcpy(dest,&status,sizeof(IMG_BEACON));
    //if mutex locked, unlock it
    if(locked){
        //done with status information
        ctl_mutex_unlock(&stat_mutex); 
    }
    
    return sizeof(IMG_BEACON);
}
