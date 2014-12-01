/*************************************************** 
  This is a library for the Adafruit TTL JPEG Camera (VC0706 chipset)

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/products/397

  These displays use Serial to communicate, 2 pins are required to interface

  Adafruit invests time and resources providing this open source code, 
  please support Adafruit and open-source hardware by purchasing 
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.  
  BSD license, all text above must be included in any redistribution
 ****************************************************/


#include "Adafruit_VC0706.h"
#include <msp430.h>
#include <string.h>
#include <UCA1_uart.h>
#include <stdio.h>

  uint8_t  camerabuff[CAMERABUFFSIZ+1];
  uint8_t  bufferLen;
  uint8_t  serialNum;
  uint16_t frameptr;

void Adafruit_VC0706_init(void) {
  frameptr  = 0;
  bufferLen = 0;
  serialNum = 0;
  UCA1_init_UART();
  UCA1_BR38400();
}

boolean Adafruit_VC0706_reset(void) {
  uint8_t args[] = {0x0};

  return Adafruit_VC0706_runCommand(VC0706_RESET, args, 1, 5,true);
}

boolean Adafruit_VC0706_motionDetected(void) {
  if (Adafruit_VC0706_readResponse(4, 200) != 4) {
    return false;
  }
  if (! Adafruit_VC0706_verifyResponse(VC0706_COMM_MOTION_DETECTED))
    return false;

  return true;
}


boolean Adafruit_VC0706_setMotionStatus(uint8_t x, uint8_t d1, uint8_t d2) {
  uint8_t args[4];
  args[0] = 0x03;
  args[1] = x;
  args[2] = d1;
  args[3] = d2;

  return Adafruit_VC0706_runCommand(VC0706_MOTION_CTRL, args, sizeof(args), 5,true);
}


uint8_t Adafruit_VC0706_getMotionStatus(uint8_t x) {

  uint8_t args[2] = {0x01, 0};
  args[1] = x;

  return Adafruit_VC0706_runCommand(VC0706_MOTION_STATUS, args, sizeof(args), 5,true);
}


boolean Adafruit_VC0706_setMotionDetect(boolean flag) {
  uint8_t args[2] = {0x01, 0};
  args[1] = flag;

  if (! Adafruit_VC0706_setMotionStatus(VC0706_MOTIONCONTROL,VC0706_UARTMOTION, VC0706_ACTIVATEMOTION))
    return false;
  
  Adafruit_VC0706_runCommand(VC0706_COMM_MOTION_CTRL, args, sizeof(args), 5,true);
}



boolean Adafruit_VC0706_getMotionDetect(void) {
  uint8_t args[] = {0x0};

  if (! Adafruit_VC0706_runCommand(VC0706_COMM_MOTION_STATUS, args, 1, 6,true))
    return false;

  return camerabuff[5];
}

uint8_t Adafruit_VC0706_getImageSize(void) {
  uint8_t args[] = {0x4, 0x4, 0x1, 0x00, 0x19};
  if (! Adafruit_VC0706_runCommand(VC0706_READ_DATA, args, sizeof(args), 6,true))
    return -1;

  return camerabuff[5];
}

// 11940 bytes is normal image size;
boolean Adafruit_VC0706_setImageSize(uint8_t x) {
  uint8_t args[6] = {0x05, 0x04, 0x01, 0x00, 0x19, 0};
  args[5] = x;

  return Adafruit_VC0706_runCommand(VC0706_WRITE_DATA, args, sizeof(args), 5,true);
} 

/****************** downsize image control */

uint8_t Adafruit_VC0706_getDownsize(void) {
  uint8_t args[] = {0x0};
  if (! Adafruit_VC0706_runCommand(VC0706_DOWNSIZE_STATUS, args, 1, 6,true)) 
    return -1;

   return camerabuff[5];
}

boolean Adafruit_VC0706_setDownsize(uint8_t newsize) {
  uint8_t args[2] = {0x01, 0};
  args[1] = newsize;

  return Adafruit_VC0706_runCommand(VC0706_DOWNSIZE_CTRL, args, 2, 5,true);
}

/***************** other high level commands */

char * Adafruit_VC0706_getVersion(void) {
  uint8_t args[] = {0x01};
  
 Adafruit_VC0706_sendCommand(VC0706_GEN_VERSION, args, 1);
  // get reply
  if (!Adafruit_VC0706_readResponse(CAMERABUFFSIZ, 200)) 
    return 0;
  camerabuff[bufferLen] = 0;  // end it!
  return (char *)camerabuff;  // return it!
}


/****************** high level photo comamnds */

void Adafruit_VC0706_OSD(uint8_t x, uint8_t y, char *str) {
  int i;
  uint8_t args[17];// = {strlen(str), strlen(str)-1, (y & 0xF) | ((x & 0x3) << 4)};
  args[0] = strlen(str);
  args[1] = strlen(str) -1;
  args[2] = (y & 0xF) | ((x & 0x3) << 4);
  if (strlen(str) > 14) { str[13] = 0; }


  for (i=0; i<strlen(str); i++) {
    char c = str[i];
    if ((c >= '0') && (c <= '9')) {
      str[i] -= '0';
    } else if ((c >= 'A') && (c <= 'Z')) {
      str[i] -= 'A';
      str[i] += 10;
    } else if ((c >= 'a') && (c <= 'z')) {
      str[i] -= 'a';
      str[i] += 36;
    }

    args[3+i] = str[i];
  }

   Adafruit_VC0706_runCommand(VC0706_OSD_ADD_CHAR, args, strlen(str)+3, 5, true);
   Adafruit_VC0706_printBuff();
}

boolean Adafruit_VC0706_setCompression(uint8_t c) {
  uint8_t args[6] = {0x5, 0x1, 0x1, 0x12, 0x04, 0};
  args[5] = c;
  return Adafruit_VC0706_runCommand(VC0706_WRITE_DATA, args, sizeof(args), 5, true);
}

uint8_t Adafruit_VC0706_getCompression(void) {
  uint8_t args[] = {0x4, 0x1, 0x1, 0x12, 0x04};
  Adafruit_VC0706_runCommand(VC0706_READ_DATA, args, sizeof(args), 6, true);
  Adafruit_VC0706_printBuff();
  return camerabuff[5];
}

boolean Adafruit_VC0706_setPTZ(uint16_t wz, uint16_t hz, uint16_t pan, uint16_t tilt) {
  uint8_t args[9];
  args[0] = 0x08;
  args[1] = wz >> 8;
  args[2] = wz;
  args[3] = hz >> 8;
  args[4] = wz;
  args[5] = pan >> 8;
  args[6] = pan;
  args[7] = tilt >> 8;
  args[8] = tilt;


  return (! Adafruit_VC0706_runCommand(VC0706_SET_ZOOM, args, sizeof(args), 5, true));
}


boolean Adafruit_VC0706_getPTZ(uint16_t *w, uint16_t *h, uint16_t *wz, uint16_t *hz, uint16_t *pan, uint16_t *tilt) {
  uint8_t args[] = {0x0};
  
  if (! Adafruit_VC0706_runCommand(VC0706_GET_ZOOM, args, sizeof(args), 16,true))
    return false;
  Adafruit_VC0706_printBuff();

  *w = camerabuff[5];
  *w <<= 8;
  *w |= camerabuff[6];

  *h = camerabuff[7];
  *h <<= 8;
  *h |= camerabuff[8];

  *wz = camerabuff[9];
  *wz <<= 8;
  *wz |= camerabuff[10];

  *hz = camerabuff[11];
  *hz <<= 8;
  *hz |= camerabuff[12];

  *pan = camerabuff[13];
  *pan <<= 8;
  *pan |= camerabuff[14];

  *tilt = camerabuff[15];
  *tilt <<= 8;
  *tilt |= camerabuff[16];

  return true;
}


boolean Adafruit_VC0706_takePicture(void) {
  frameptr = 0;
  return Adafruit_VC0706_cameraFrameBuffCtrl(VC0706_STOPCURRENTFRAME); 
}

boolean Adafruit_VC0706_resumeVideo(void) {
  return Adafruit_VC0706_cameraFrameBuffCtrl(VC0706_RESUMEFRAME); 
}

boolean Adafruit_VC0706_TVon(void) {
  uint8_t args[] = {0x1, 0x1};
  return Adafruit_VC0706_runCommand(VC0706_TVOUT_CTRL, args, sizeof(args), 5, true);
}
boolean Adafruit_VC0706_TVoff(void) {
  uint8_t args[] = {0x1, 0x0};
  return Adafruit_VC0706_runCommand(VC0706_TVOUT_CTRL, args, sizeof(args), 5, true);
}

boolean Adafruit_VC0706_cameraFrameBuffCtrl(uint8_t command) {
  uint8_t args[2] = {0x1, 0};
  args[1] = command;
  return Adafruit_VC0706_runCommand(VC0706_FBUF_CTRL, args, sizeof(args), 5, true);
}

uint32_t Adafruit_VC0706_frameLength(void) {
  uint8_t args[] = {0x01, 0x00};
  uint32_t len;
  if (!Adafruit_VC0706_runCommand(VC0706_GET_FBUF_LEN, args, sizeof(args), 9, true))
  {
    Adafruit_VC0706_printBuff();
    return 0;
  }

  len = camerabuff[5];
  len <<= 8;
  len |= camerabuff[6];
  len <<= 8;
  len |= camerabuff[7];
  len <<= 8;
  len |= camerabuff[8];

  return len;
}


uint8_t Adafruit_VC0706_available(void) {
  return bufferLen;
}


uint8_t * Adafruit_VC0706_readPicture(uint8_t n) {
  uint8_t args[13] = {0x0C, 0x0, 0x0A, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  args[5] = frameptr >> 8;
  args[6] = frameptr & 0xFF;
  args[10] = n;
  args[11] = CAMERADELAY >> 8;
  args[12] = CAMERADELAY & 0xFF;

  if (! Adafruit_VC0706_runCommand(VC0706_READ_FBUF, args, sizeof(args), 5, false))
    return 0;


  // read into the buffer PACKETLEN!
  if (Adafruit_VC0706_readResponse(n+5, CAMERADELAY) == 0) 
      return 0;


  frameptr += n;

  return camerabuff;
}

/**************** low level commands */


boolean Adafruit_VC0706_runCommand(uint8_t cmd, uint8_t *args, uint8_t argn, 
			   uint8_t resplen, boolean flushflag) {
  // flush out anything in the buffer?
  if (flushflag) {
    Adafruit_VC0706_readResponse(100, 10); 
  }

  Adafruit_VC0706_sendCommand(cmd, args, argn);
  if (Adafruit_VC0706_readResponse(resplen, 200) != resplen) 
    return false;
  if (! Adafruit_VC0706_verifyResponse(cmd))
    return false;
  return true;
}

void Adafruit_VC0706_sendCommand(uint8_t cmd, uint8_t args[], uint8_t argn) {
    int i;
    UCA1_TxChar(0x56);
    UCA1_TxChar(serialNum);
    UCA1_TxChar(cmd);

    for(i=0; i<argn; i++) {
      UCA1_TxChar(args[i]);
      //Serial.print(" 0x");
      //Serial.print(args[i], HEX);
    }
}

uint8_t Adafruit_VC0706_readResponse(uint8_t numbytes, uint8_t timeout) {
  uint8_t counter = 0;
  int c;
  bufferLen = 0;
 
  while ((timeout != counter) && (bufferLen != numbytes)){
    c=UCA1_CheckKey();
    if (c == EOF) {
      ctl_timeout_wait(ctl_get_current_time()+4);
      counter++;
      continue;
    }
    counter = 0;
    // there's a byte!
    camerabuff[bufferLen++] = c;
  }
  return bufferLen;
}

boolean Adafruit_VC0706_verifyResponse(uint8_t command) {
     if((camerabuff[0] != 0x76) ||
      (camerabuff[1] != serialNum) ||
      (camerabuff[2] != command) ||
      (camerabuff[3] != 0x0))
      return false;
  return true;
  
}

void Adafruit_VC0706_printBuff(void) {
uint8_t i;
  for (i = 0; i< bufferLen; i++) {
    printf(" 0x%02X",camerabuff[i]); 
  }
  printf("\r\n");
}
