#ifndef __BUS_ERRORS_H
  #define __BUS_ERRORS_H
  #include <commandLib.h>
  
  //error sources for IMAGER
  enum{ERR_IMG=ERR_SRC_CMD+1};
  enum{ERR_IMG_SD_CARD_WRITE=0};
  enum{ERR_IMG_SD_CARD_READ=1};
  enum{ERR_IMG_SD_CARD_INIT=2};

  enum{ERR_IMG_TX=3};

  enum{ERR_IMG_TAKEPIC=4};

   
    
  //subsystem errors
  enum{SUB_ERR_SPI_CRC};
#endif
  