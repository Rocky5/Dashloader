#ifndef _cfgparser_h_
#define _cfgparser_h_

#include "types.h"


#define CONFIG_FILE "boot.cfg"
#define CONFIG_FILE_BUFSIZE 4096


#define MAX_PATHNAME 256


typedef enum {
    CFG_SCREEN_KEEP = 0,	/* Keep current screen, default */
    CFG_SCREEN_BLANK,		/* Blank screen */
    CFG_SCREEN_OFF,		/* Turn video output off */
    CFG_SCREEN_NOTHING		/* Don't touch */
} cfg_screen;


typedef struct _CONFIGENTRY {
    unsigned int err_linenum;
    char path[MAX_PATHNAME];
    char rom[MAX_PATHNAME];
    char altrom[MAX_PATHNAME];
    char log[MAX_PATHNAME];
    UCHAR szRC4Key[16];
    UCHAR szEEPROMKey1_0[16];
    UCHAR szEEPROMKey1_1[16];
    UCHAR szEEPROMKey1_6[16];
    UCHAR ledseq_set;
    UCHAR ledseq;
    cfg_screen screen;
} CONFIGENTRY, *LPCONFIGENTRY;

typedef enum {
    VID_INVALID	= 0x00000000,
    NTSC_M	= 0x00400100,
    PAL_I	= 0x00800300
} VIDEO_STANDARD;



NTSTATUS cfgparser_get_config(CONFIGENTRY *entry);


#endif /* _cfgparser_h_ */
