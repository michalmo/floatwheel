#ifdef __CC_ARM
#define __AT(__ADDR__) at(__ADDR__),zero_init
#else
#define __AT(__ADDR__) section((".bss.ARM.__at_" __ADDR__))
#endif
