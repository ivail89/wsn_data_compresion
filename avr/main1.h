#ifndef __USERDEF__
#define __USERDEF__


#ifdef F_CPU
#undeif F_CPU
#endif
#define F_CPU 16000000UL

#define TIME_INIT_MS 1000
#define TCNT16_TIME_INIT (uint16_t) (~((uint64_t) TIME_INIT_MS*(uint64_t)/1024000) +1 )

#define set_bit(port,mask) ((port) |=_BV(mask))
#define clear_bit(port,mask) ((port) &= ~_BV(mask))
#define invert_bit(port,mask) ((port) ^=_BV(mask))
#endif