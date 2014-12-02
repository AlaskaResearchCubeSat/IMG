#ifndef __IMG_ERRORS_H
  #define __IMG_ERRORS_H
  #include <commandLib.h>
  
  //error sources for IMAGER
  enum{ERR_IMG=ERR_SRC_CMD+1};
  
  enum{ERR_IMG_SD_CARD_WRITE,ERR_IMG_SD_CARD_READ,ERR_IMG_SD_CARD_INIT,ERR_IMG_TX,ERR_IMG_TAKEPIC};

#endif
  