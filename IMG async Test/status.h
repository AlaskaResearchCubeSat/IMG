#ifndef __STATUS_H
#define __STATUS_H

    //image beacon structure
    typedef struct{
        int sd_stat;
        ticker img_time;
        unsigned char num,flags;
    }IMG_BEACON;
        
    //flags for beacon data    
    enum{IMG_BEACON_FLAGS_HAVE_PIC=1<<0,IMG_BEACON_FLAGS_PIC_SCH=1<<1,IMG_BEACON_FLAGS_PIC_IN_PROGRESS=1<<2};


    unsigned int img_make_beacon(IMG_BEACON *dest);
    void status_init(void);
    void status_refresh(void);
    void stat_pic_complete(unsigned char num);
    void stat_pic_abort(void);
    void stat_pic_start(void);

#endif
    