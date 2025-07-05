#define _STRINGIFY(x) #x
#define STRINGIFY(x) _STRINGIFY(x)

#ifdef __CC_ARM
#define __AT(__ADDR__) at(STRINGIFY(__ADDR__))
#else
#define __AT(__ADDR__) section((".ARM.__at_" STRINGIFY(__ADDR__)))
#endif

#ifdef __CC_ARM
#define __AT__ZERO_INIT(__ADDR__) at(STRINGIFY(__ADDR__)),zero_init
#else
#define __AT__ZERO_INIT(__ADDR__) section((".bss.ARM.__at_" STRINGIFY(__ADDR__)))
#endif

