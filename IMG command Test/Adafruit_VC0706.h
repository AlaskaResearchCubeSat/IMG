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

#include <msp430.h>

#define VC0706_RESET  0x26
#define VC0706_GEN_VERSION 0x11
#define VC0706_READ_FBUF 0x32
#define VC0706_GET_FBUF_LEN 0x34
#define VC0706_FBUF_CTRL 0x36
#define VC0706_DOWNSIZE_CTRL 0x54
#define VC0706_DOWNSIZE_STATUS 0x55
#define VC0706_READ_DATA 0x30
#define VC0706_WRITE_DATA 0x31
#define VC0706_COMM_MOTION_CTRL 0x37
#define VC0706_COMM_MOTION_STATUS 0x38
#define VC0706_COMM_MOTION_DETECTED 0x39
#define VC0706_MOTION_CTRL 0x42
#define VC0706_MOTION_STATUS 0x43
#define VC0706_TVOUT_CTRL 0x44
#define VC0706_OSD_ADD_CHAR 0x45

#define VC0706_STOPCURRENTFRAME 0x0
#define VC0706_STOPNEXTFRAME 0x1
#define VC0706_RESUMEFRAME 0x3
#define VC0706_STEPFRAME 0x2

#define VC0706_640x480 0x00
#define VC0706_320x240 0x11
#define VC0706_160x120 0x22

#define VC0706_MOTIONCONTROL 0x0
#define VC0706_UARTMOTION 0x01
#define VC0706_ACTIVATEMOTION 0x01

#define VC0706_SET_ZOOM 0x52
#define VC0706_GET_ZOOM 0x53

#define CAMERABUFFSIZ 100
#define CAMERADELAY 10



typedef short boolean;
#define false 0
#define true 1

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;

  boolean Adafruit_VC0706_reset(void);
  boolean Adafruit_VC0706_TVon(void);
  boolean Adafruit_VC0706_TVoff(void);
  boolean Adafruit_VC0706_takePicture(void);
  uint8_t *Adafruit_VC0706_readPicture(uint16_t frameptr,uint8_t n);
  boolean Adafruit_VC0706_resumeVideo(void);
  uint32_t Adafruit_VC0706_frameLength(void);
  char *Adafruit_VC0706_getVersion(void);
  uint8_t Adafruit_VC0706_available(void);
  uint8_t Adafruit_VC0706_getDownsize(void);
  boolean Adafruit_VC0706_setDownsize(uint8_t);
  uint8_t Adafruit_VC0706_getImageSize(void);
  boolean Adafruit_VC0706_setImageSize(uint8_t);
  boolean Adafruit_VC0706_getMotionDetect(void);
  uint8_t Adafruit_VC0706_getMotionStatus(uint8_t);
  boolean Adafruit_VC0706_motionDetected(void);
  boolean Adafruit_VC0706_setMotionDetect(boolean f);
  boolean Adafruit_VC0706_setMotionStatus(uint8_t x, uint8_t d1, uint8_t d2);
  boolean Adafruit_VC0706_cameraFrameBuffCtrl(uint8_t command);
  uint8_t Adafruit_VC0706_getCompression(void);
  boolean Adafruit_VC0706_setCompression(uint8_t c);
  
  boolean Adafruit_VC0706_getPTZ(uint16_t *w, uint16_t *h, uint16_t *wz, uint16_t *hz, uint16_t *pan, uint16_t *tilt);
  boolean Adafruit_VC0706_setPTZ(uint16_t wz, uint16_t hz, uint16_t pan, uint16_t tilt);

  void Adafruit_VC0706_OSD(uint8_t x, uint8_t y, char *s); // isnt supported by the chip :(
  void Adafruit_VC0706_init(void);

  boolean Adafruit_VC0706_runCommand(uint8_t cmdconst,const uint8_t args[], uint8_t argn, uint8_t resp, boolean flushflag); 
  void Adafruit_VC0706_sendCommand(uint8_t cmd,const uint8_t args[], uint8_t argn); 
  uint8_t Adafruit_VC0706_readResponse(uint8_t numbytes, uint8_t timeout);
  boolean Adafruit_VC0706_verifyResponse(uint8_t command);
  void Adafruit_VC0706_printBuff(void);

