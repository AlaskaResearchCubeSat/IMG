#ifndef __IMG_H
#define __IMG_H
    #include <Error.h>
    
    //define number of image slots
    #define NUM_IMG_SLOTS           (5)
    //define the size of a image slot
    #define IMG_SLOT_SIZE           (100)
    //define address ranges for image data
    enum{IMG_ADDR_START=ERR_ADDR_END+1,IMG_ADDR_END=IMG_ADDR_START+NUM_IMG_SLOTS*IMG_SLOT_SIZE};

    void cmd_parse(void *p);
    void sub_events(void *p);
    void img_events(void *p0);  

#endif
    