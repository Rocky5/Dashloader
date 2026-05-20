#ifndef _XBOX_H_
#define _XBOX_H_

/* you can change this */
#define DEBUG

#define BUFFERSIZE 256

/* a retail Xbox has 64 MB of RAM */
#define RAMSIZE (64 * 1024*1024)

#define MIN_2BL 0x400000
#define MAX_2BL 0x405FFF
#define THE_2BL_SIZE 0x6000

#define MIN_SHADOW_ROM 0x0000000
#define MAX_SHADOW_ROM 0x3000000
#define SHADOW_ROM_SIZE 0x100000

#define ROM_256K 0x40000
#define ROM_512K 0x80000
#define ROM_1024K 0x100000


#endif // _XBOX_H_
