#ifndef __IMG_H
#define __IMG_H
    #include <Error.h>
    #include <ctl.h>

    //flags for events handled by the imager
    enum{IMG_EV_TAKEPIC=(1<<0),IMG_EV_LOADPIC=(1<<1),IMG_EV_INPROGRESS=(1<<2),IMG_EV_PIC_TAKEN=(1<<3)};

    
    #define IMG_EV_ALL (IMG_EV_TAKEPIC|IMG_EV_LOADPIC)
    
    //define number of image slots
    #define NUM_IMG_SLOTS           (5)
    //define the size of a image slot
    #define IMG_SLOT_SIZE           (150)
    //define address ranges for image data
    enum{IMG_ADDR_START=ERR_ADDR_END+10,IMG_ADDR_END=IMG_ADDR_START+NUM_IMG_SLOTS*IMG_SLOT_SIZE};

    //read and write slots for picture
    extern int readPic,writePic;
    
    //Event set for imager events
    extern CTL_EVENT_SET_t IMG_events;

    //Block ID's for image blocks
    enum{BT_IMG_START=0x990F,BT_IMG_BODY=0x99F0};

    //image data block structure
    typedef struct{
        unsigned short magic;
        unsigned char  num;
        unsigned char  block;
        unsigned char dat[506];
        unsigned short CRC;
    }IMG_DAT;
        
    //image beacon structure
    typedef struct{
        int sd_stat;
        ticker img_time;
        unsigned char num,flags;
    }IMG_BEACON;

    void cmd_parse(void *p);
    void sub_events(void *p);
    void img_events(void *p0);  
    unsigned int img_make_beacon(IMG_BEACON *dest);

#endif
    