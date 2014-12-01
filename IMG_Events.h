#include <ctl.h>

extern CTL_EVENT_SET_t IMG_events;

//flags for events handled by the imager
enum{IMG_EV_TAKEPIC=(1<<0),IMG_EV_SAVEPIC=(1<<1),IMG_EV_LOADPIC=(1<<2)};

#define IMG_EV_ALL (IMG_EV_TAKEPIC|IMG_EV_SAVEPIC|IMG_EV_LOADPIC)

