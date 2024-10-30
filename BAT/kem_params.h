#ifndef KEM_PARAMS_H
#define KEM_PARAMS_H

// BAT_257_512

#define Q          257
#define N          512
#define LOGN       9
#define LVLBYTES   16

#define XCAT(x, y)    XCAT_(x, y)
#define XCAT_(x, y)   x ## y
#define XSTR(x)       XSTR_(x)
#define XSTR_(x)      #x

// #define Zn(name)   XCAT(XCAT(XCAT(bat_, Q), XCAT(_, N)), XCAT(_, name))
#define Zn(name) XCAT(kem_, name)
#define ZN(name)   XCAT(XCAT(XCAT(BAT_, Q), XCAT(_, N)), XCAT(_, name))

#endif

