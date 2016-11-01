
#include "gpio.h"


//for Banana Pi
#ifdef P_A20
#include "gpio/a20.c"
#endif

 //debugging mode (for machine with no gpio pins)
#ifdef P_ALL
#include "gpio/all.c"
#endif


