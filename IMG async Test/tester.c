#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <msp430.h>
#include <ctl.h>
#include <terminal.h>
#include <ARCbus.h>
#include <UCA1_uart.h>
#include <Error.h>
#include <commandLib.h>
#include <SDlib.h>
#include <crc.h>
#include "IMG_errors.h"
#include "Adafruit_VC0706.h"
#include "sensor.h"
#include "IMG.h"
#include "status.h"

int cmdPic=0;

int tvOffCmd(char ** argv, unsigned short argc)
{
  printf("Turning video off");
  Adafruit_VC0706_TVoff();
  printf("Video off");
  return 0;
}

int tvOnCmd(char ** argv, unsigned short argc)
{
  printf("Turning video on");
  Adafruit_VC0706_TVon();
  printf("Video on");
}

const CMD_SPEC cmd_tbl[]={{"help"," [command]\r\n\t""get a list of commands or help on a spesific command.",helpCmd},
                         //CTL_COMMANDS,
                         //ARC_COMMANDS,ERROR_COMMANDS,MMC_COMMANDS,
                         {"tvoff","\r\n\t""Turn the Composite video output off",tvOffCmd},
                         {"tvon","\r\n\t""Turn the Composite video output on",tvOnCmd},
                         {NULL,NULL,NULL}};