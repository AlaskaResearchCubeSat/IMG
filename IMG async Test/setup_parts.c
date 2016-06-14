

 //initialize the MSP430 Clocks
void initCLK(void){
  //set XT1 load caps, do this first so XT1 starts up sooner
  UCSCTL6=XCAP_0|XT2OFF|XT1DRIVE_3;
  //stop watchdog
  //WDT_STOP();
  //kick watchdog
  WDT_KICK();
  //set higher core voltage
  if(!PMM_setVCore(PMM_CORE_LEVEL_3)){
    //Voltage changed succeeded, set frequency
    //setup clocks
    //set frequency range
    UCSCTL1=DCORSEL_5;
    //setup FLL for 19.99 MHz operation
    UCSCTL2=FLLD__4|(609);
    UCSCTL3=SELREF__XT1CLK|FLLREFDIV__4;
  }else{
    //core voltage could not be set, report error
    _record_error(ERR_LEV_CRITICAL,BUS_ERR_SRC_STARTUP,STARTUP_ERR_PMM_VCORE,PMMCTL0,0);
  }
  //use XT1 for ACLK and DCO for MCLK and SMCLK
  UCSCTL4=SELA_0|SELS_3|SELM_3;
  
  //TODO: Maybe wait for LFXT to startup?
}

//=============================================================

//setup Supply voltage supervisor and monitor levels and interrupts
void initSVS(void){
  //unlock PMM
  PMMCTL0_H=PMMPW_H;
  //check voltage level
  switch(PMMCTL0&PMMCOREV_3){
    //settings for highest core voltage settings
    case PMMCOREV_3:
      //setup high side supervisor and monitor
      SVSMHCTL=SVMHE|SVSHE|SVSHRVL_3|SVSMHRRL_7;
    break;
    //settings for lowest core voltage settings
    case PMMCOREV_0:
      //setup high side supervisor and monitor
      //TODO: are these correct?
      SVSMHCTL=SVMHE|SVSHE|SVSHRVL_0|SVSMHRRL_1;
    break;
    default :
      //unexpected core voltage, did not set SVM
      _record_error(ERR_LEV_CRITICAL,BUS_ERR_SRC_STARTUP,STARTUP_ERR_SVM_UNEXPECTED_VCORE,PMMCTL0,0);
    break;
  }
  //clear interrupt flags
  PMMIFG&=~(SVMLIFG|SVMHIFG|SVMHVLRIFG|SVMLVLRIFG);
  //setup interrupts
  PMMRIE|=SVMLIE|SVMHIE|SVMHVLRIE|SVMLVLRIE;
  //lock PMM
  PMMCTL0_H=0;
}
//=============================================================

//low level setup code
void ARC_setup(void)
{
  extern ticker ticker_time;
  //setup error reporting library
  error_init();
  //record reset error first so that it appears first in error log
  //check for reset error
  if(saved_error.magic==RESET_MAGIC_POST)
  {
    _record_error(saved_error.level,saved_error.source,saved_error.err,saved_error.argument,0);
    //clear magic so we are not confused in the future
    saved_error.magic=RESET_MAGIC_EMPTY;
  }
  else
  {
    //for some reason there is no error
    _record_error(ERR_LEV_CRITICAL,BUS_ERR_SRC_STARTUP,STARTUP_ERR_NO_ERROR,0,0);
    //clear magic so we are not confused in the future
    saved_error.magic=RESET_MAGIC_EMPTY;
  }
  //setup clocks
  initCLK();
  //set time ticker to zero
  ticker_time=0;
  //setup SVS
  initSVS();
  //setup timerA
  init_timerA();
  //set timer to increment by 1
  ctl_time_increment=1;  
  
  //setup error handler
  err_register_handler(BUS_MIN_ERR,BUS_MAX_ERR,err_decode_arcbus,ERR_FLAGS_LIB);

  //init buffer
  BUS_init_buffer();
  //========[setup AUX supplies]=======
  if(AUXCTL0&LOCKAUX)
  {
    //unlock AUX registers
    AUXCTL0_H=AUXKEY_H;
    //disable all supplies but VCC
    AUXCTL1=AUX2MD|AUX1MD|AUX0MD|AUX0OK;
    //clear LOCKAUX bit
    AUXCTL0=AUXKEY;
    //lock AUX registers
    AUXCTL0_H=0;
  }

  //TODO: determine if ctl_timeslice_period should be set to allow preemptive rescheduling
  
  //kick watchdog
  WDT_KICK();
}
