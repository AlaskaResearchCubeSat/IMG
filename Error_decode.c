#include <Error.h>
#include "Proxy_errors.h"
#include <stdio.h>
#include <string.h>

//decode errors from ACDS system
char *err_decode(char buf[150], unsigned short source,int err, unsigned short argument){
  char str [256];
  switch(source){
    case ERR_SRC_CMD:
      switch(err){
        case CMD_ERR_RESET:
          return "Command Line : Reset Command reset";  
      }
    break;

    case ERR_IMG:
      switch(err){
        case ERR_IMG_SD_CARD_WRITE:
          return "IMG : Error writing to SD card.";
        case ERR_IMG_SD_CARD_READ:
          return "IMG : Error reading from SD card";
        case ERR_IMG_SD_CARD_INIT:
          return "IMG : SD card not initialized";
        case ERR_IMG_TX:
          
          sprintf(str, "IMG : Error transmitting packet, %i", argument);
          return str;
        case ERR_IMG_TAKEPIC:
          return "IMG : Error taking picture";
      }
    break; 
  }
  sprintf(buf,"source = %i, error = %i, argument = %i",source,err,argument);
  return buf;
}

char* itoa(int i, char b[]){
    char const digit[] = "0123456789";
    int shifter;
    char* p = b;
    if(i<0){
        *p++ = '-';
        i *= -1;
    }
    shifter = i;
    do{ //Move to where representation ends
        ++p;
        shifter = shifter/10;
    }while(shifter);
    *p = '\0';
    do{ //Move back, inserting digits as u go
        *--p = digit[i%10];
        i = i/10;
    }while(i);
    return b;
}
