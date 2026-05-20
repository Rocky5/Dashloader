/*
  Xbox XBE bootloader
  
  Original code by Michael Steil & anonymous
  VESA Framebuffer code by Milosch Meriac
  Modified to boot shadow bioses by Phoenix
  
  Note that this is a very modified version, not created
  nor endorsed by the xbox-linux team.

  Improved version detector and support for kernels 5530+ by rmenhal.
*/

#include "printf.h"
#include "consts.h"
#include "xboxkrnl.h"
#include "xbox.h"
#include "boot.h"
#include "strh.h"
#include "cfgparser.h"
#include "config.h"
#include "video.h"
#include "ntfile.h"


unsigned char DebugFlag;

static int metoobfm_detect(void *rom, unsigned long rom_size);
static void *metoobfm_prepare(void *rom, unsigned long rom_size,
			      CONFIGENTRY *entry);
static int evoxrom_detect(void *rom, unsigned long rom_size);
static void *evoxrom_prepare(void *rom, unsigned long rom_size,
			     CONFIGENTRY *entry);
static void *std2bl_prepare(void *rom, unsigned long rom_size,
			    CONFIGENTRY *entry);
static void setup_video(CONFIGENTRY *entry);

static void mainthread(PVOID parm1, PVOID parm2);



// Useful for debugging
void die() {
    /* red-orange-green blink */
    HalWriteSMBusValue(0x20, 0x08, 0, 0x63);
    HalWriteSMBusValue(0x20, 0x07, 0, 0x01);
    while(1);
}


static long LoadShadowROM(PVOID Filename, long *lFileSize)
{
    HANDLE hFile;
    PBYTE Buffer = 0;
    ULONGLONG FileSize;

    if (!(hFile = OpenFile(NULL, Filename, -1, FILE_NON_DIRECTORY_FILE))) {
	dprintf("Error open file %s\n",Filename);
	die();
    }

    if (!GetFileSize(hFile, &FileSize)) {
	dprintf("Error getting file size %s\n",Filename);
	die();
    }

    if (FileSize != ROM_256K && FileSize != ROM_512K && FileSize != ROM_1024K)
    {
	dprintf("Invalid ROM Size %s\n",Filename);
	die();
    }
    
    Buffer = MmAllocateContiguousMemoryEx((ULONG)SHADOW_ROM_SIZE,
					  MIN_SHADOW_ROM, MAX_SHADOW_ROM,
					  0, PAGE_READWRITE);
    
    if (!Buffer) {
	dprintf("Error alloc memory for rom file %s\n",Filename);
	die();
    }

    if (!ReadFile(hFile, Buffer, FileSize)) {
	dprintf("Error loading file %s\n",Filename);
	die();
    }
    
    // Copy rom to fill 1MB if necessary
    switch ((ULONG)FileSize) {
    case ROM_256K:
	memcpy(Buffer+ROM_256K, Buffer, ROM_256K);
	// fall through
    case ROM_512K:
	memcpy(Buffer+ROM_512K, Buffer, ROM_512K);
	break;
    default:
	// No need to fill
	break;
    }
    
    NtClose(hFile);
    
    *lFileSize = FileSize;
    
    return (long)Buffer;
}



typedef enum {
    XBOXV_1_0 = 0,
    XBOXV_1_1 = 1,
    XBOXV_1_2 = 2,
    XBOXV_1_3 = 3,
    XBOXV_1_4 = 4,
    XBOXV_1_5 = 5,
    XBOXV_1_6 = 6,
    XBOXV_UNKNOWN = 10000
} xbox_version;


/*
 * A rudimentary version detector. Just what's needed to figure out
 * which EEPROM key should be used.
 */

static xbox_version get_xbox_version(void)
{
    static xbox_version ver = XBOXV_UNKNOWN;
    static int detected = 0;
    int verdetect;
    encoder_type encoder;

    if (detected)
	return ver;

    detected = 1;

    __asm__ __volatile__(
	"movw	$0xCF8,%%dx\n"
	"movl	$0x80000810,%%eax\n"
	"outl	%%eax,(%%dx)\n"
	"addb	$4,%%dl\n"
	"inl	(%%dx),%%eax\n"
	: "=a" (verdetect)
	: /* no input */
	: "%dx"
	);
    
    dprintf("Detecting xbox version:\n");
    dprintf(" * verdetect = %08x\n", verdetect);

    encoder = video_get_encoder_type();

    if (verdetect == 0x8001) {
	dprintf(" * Xbox v1.0\n");
	ver = XBOXV_1_0;
    }
    else if (encoder == ENCODER_CONEXANT) {
	dprintf(" * Xbox v1.1, v1.2 or v1.3\n");
	ver = XBOXV_1_1;
    }
    else if (encoder == ENCODER_FOCUS) {
	dprintf(" * Xbox v1.4 or v1.5\n");
	ver = XBOXV_1_4;
    }
    else if (encoder == ENCODER_XCALIBUR || encoder == ENCODER_UNKNOWN) {
	dprintf(" * Xbox v1.6+\n");
	ver = XBOXV_1_6;
    }
    else {
	/* Execution can't get here */
	dprintf(" * Version detector needs an update. :)\n");
	ver = XBOXV_UNKNOWN;
    }

    return ver;
}



void boot(void)
{
    HANDLE hThread = 0;
    ULONG Id = 0;
    LARGE_INTEGER Timeout;
    ULONG Status = 0;
    
    Timeout.QuadPart = 0;
    
    
    if(!NT_SUCCESS(PsCreateSystemThreadEx(&hThread,
					  0,
					  65536,
					  0,
					  &Id,
					  NULL,
					  NULL,
					  FALSE,
					  FALSE, 
					  (PVOID)&mainthread))) {
	
	HalReturnToFirmware(2);
    }
    while(1) {
	Status = NtWaitForSingleObjectEx(hThread, 1 /* UserMode */ ,
					 FALSE, &Timeout);
	if (Status == STATUS_SUCCESS) {
	    NtClose(hThread);
	    HalReturnToFirmware(2);
	}
    }
}



static void mainthread(PVOID parm1, PVOID parm2)
{
    long ShadowRomPos;
    long RomSize;
    
    NTSTATUS Error;
    DWORD state = 0;
    CONFIGENTRY entry;

    PHYSICAL_ADDRESS PhysicalRomPos;
    PVOID EntryPoint2BL;
    
/*     __asm__ __volatile__( */
/* 	"push %eax\n" */
/* 	".byte 0xe8,0,0,0,0\n" */
/* 	"pop %eax\n" */
/* 	"addl $0xf,%eax\n" */
/* 	"movl %eax,0x10045\n" */
/* 	"pushl $0x10004\n" */
/* 	"ret\n" */
/* 	"pop %eax\n" */
/* 	); */

    memset(&entry, 0, sizeof(CONFIGENTRY));
    
    /* parse the configuration file */

    Error = cfgparser_get_config(&entry);

    if (!NT_SUCCESS(Error)) 
    {
/* 	dprintf("Error reading config!\n"); */
	die();
    }

    printf_init(entry.log, entry.path);
    dprintf("Phoenix Bios Loader - The Metoo Edition\n");

    if (entry.err_linenum > 0) {
	dprintf("Warning: there were errors processing config file!\n"
		" * First error at line %d\n", entry.err_linenum);
    }

    /* If led is specified, set it */

    if (entry.ledseq_set) {
	dprintf("Setting led: %02x\n", entry.ledseq);
	HalWriteSMBusValue(0x20, 0x08, FALSE, entry.ledseq);
	HalWriteSMBusValue(0x20, 0x07, FALSE, 0x01);
    }
    
    /* Check Tray State and load appropriate bios */

    state = 0;

    if (strlen(entry.altrom) > 0) {
	if (!NT_SUCCESS(HalReadSMBusValue(0x20, 0x03, 0, &state)))
	    state = 0;
    }

    if (state == 0x10) {
	dprintf("Loading Alternate Rom: %s\n", entry.altrom);
	ShadowRomPos = LoadShadowROM(entry.altrom, &RomSize);
    } else {
	dprintf("Loading Rom: %s\n", entry.rom);
	ShadowRomPos = LoadShadowROM(entry.rom, &RomSize);
    }
    
    dprintf("Setting video\n");
    setup_video(&entry);

    PhysicalRomPos = MmGetPhysicalAddress((PVOID)ShadowRomPos);

    if (metoobfm_detect((void *)ShadowRomPos, SHADOW_ROM_SIZE)) {
	dprintf("Metoo BFM 2bl footer detected\n");
	EntryPoint2BL = metoobfm_prepare((void *)ShadowRomPos, SHADOW_ROM_SIZE,
					 &entry);
    } else if (evoxrom_detect((void *)ShadowRomPos, SHADOW_ROM_SIZE)) {
	dprintf("EvoX M8 2bl type detected\n");
	EntryPoint2BL = evoxrom_prepare((void *)ShadowRomPos, SHADOW_ROM_SIZE,
					&entry);
    } else {
	dprintf("Assuming standard 2bl format\n");
	EntryPoint2BL = std2bl_prepare((void *)ShadowRomPos, SHADOW_ROM_SIZE,
				       &entry);
    }

    dprintf("Calling 2bl:\n"
	    " * EntryPoint2BL %08x\n"
	    " * PhysicalRomPos %08x\n",
	    (DWORD)EntryPoint2BL, (DWORD)PhysicalRomPos);
   


    I2CTransmitByteGetReturn(0x54, 0x58); /* "Fuck me gently with a chainsaw"*/

    __asm__ __volatile__(
	"push	%%eax\n"
	"push	%%eax\n"
	"push	%%eax\n"
	"push	%%eax\n"
	"sgdt	0x2(%%esp)\n"
	"pop	%%eax\n"
	"pop	%%eax\n"
	"mov	%%cs,%%edx\n"
	"add	%%edx,%%eax\n"
	"mov	%%cs,0x4(%%esp)\n"
	"cli\n"
	"movw	$0xffff,(%%eax)\n"	/* Code segment size enlargement is */
	"orb	$0xb,0x6(%%eax)\n"	/* required for 5530+ kernels */
	"ljmp	*(%%esp)\n"
	: /* no output */
	: "a" (EntryPoint2BL), "c" (PhysicalRomPos)
	);
}




#define METOOBFM_MAGIC1	0x314d4642

struct metoobfm_footer {
    u32 reserved;
    u16 loader_ofs;
    u16 kernel_param_size;
    u16 kernel_param_ofs;
    u16 size_2bl;
    u32 base_2bl;
    u32 magic;		/* BFM1 */
};




static int metoobfm_detect(void *rom, unsigned long rom_size)
{
    if (*(u32 *)(rom + rom_size - 4) == METOOBFM_MAGIC1)
	return TRUE;

    return FALSE;
}

static void *metoobfm_prepare(void *rom, unsigned long rom_size,
			      CONFIGENTRY *entry)
{
    struct metoobfm_footer *ft;
    void *base;
    void *copyptr;
    char *param;

    ft = (struct metoobfm_footer *)(rom + rom_size
				    - sizeof(struct metoobfm_footer));

    dprintf("Allocate 2bl mem\n");

    base = MmAllocateContiguousMemoryEx(ft->size_2bl,
					ft->base_2bl,
					ft->base_2bl + ft->size_2bl - 1,
					0, PAGE_READWRITE);
    
    if (!base) {
	dprintf("No memory for 2BL!\n");
	die();
    }

    /* Copy the 2bl to the appropriate location */

    copyptr = (void *)(rom + rom_size - ft->size_2bl);
    dprintf("Copying 2bl\n");
    memcpy(base, copyptr, ft->size_2bl);

    if (ft->kernel_param_size != 0) {
	dprintf("Patching kernel param string\n");
	param = (char *)(ft->kernel_param_ofs + base);
	strh_dnzcpy(param, " /SHADOW /HDBOOT", ft->kernel_param_size);
	
	if (entry->screen == CFG_SCREEN_KEEP)
	    video_add_avsave_param(param, ft->kernel_param_size);
	
	param[ft->kernel_param_size - 1] = 0;

	dprintf("Parameter string: \"%s\"\n", param);
    }

    dprintf("Calculating 2bl entry point\n");

    return (void *)(ft->loader_ofs + base);
}



/* evoxrom 2bl type here means the 2bl format for original M8 (not plus) */


static int evoxrom_detect(void *rom, unsigned long rom_size)
{
    if (strncmp(rom + rom_size - 0x3000 + 0x2e58, "$EvoxRom$", 10) != 0)
	return FALSE;

    return TRUE;
}

static void *evoxrom_prepare(void *rom, unsigned long rom_size,
			     CONFIGENTRY *entry)
{
    void *virt2bl;
    void *copyptr;

    dprintf("Allocate 2bl mem\n");

    virt2bl = MmAllocateContiguousMemoryEx((ULONG) 0x3000,
					   0x400000, 0x400000 + 0x3000 - 1,
					   0, PAGE_READWRITE);
    
    if (!virt2bl) {
	dprintf("No memory for 2BL!\n");
	die();
    }

    /* Copy the 2bl to the appropriate location */

    copyptr = (void *)(rom + (rom_size - 0x3000));
    dprintf("Copying 2bl\n");
    memcpy(virt2bl, copyptr, 0x3000);

    dprintf("Calculating 2bl entry point\n");

    return (void *)(*(DWORD *)virt2bl - 0x90000 + 0x80400000);
}


static void *std2bl_prepare(void *rom, unsigned long rom_size,
			    CONFIGENTRY *entry)
{
    PVOID Virt2BL = 0;
    PVOID CopyPtr = 0;
    PVOID TempPtr = 0;
    PUCHAR KernelParamPtr = 0;
    UCHAR *eeprom_key;
    xbox_version version;
    int i;

    /* Allocate memory for the 2bl.  Has to be at 0x00400000 */

    dprintf("Allocate 2bl mem\n");
    Virt2BL = MmAllocateContiguousMemoryEx((ULONG) 0x6000,
					   0x400000, 0x400000 + 0x6000 - 1,
					   0, PAGE_READWRITE);
    
    if (!Virt2BL) {
	dprintf("\nNo memory for 2BL!\n");
	die();
    }
    
    /* Decrypt 2bl */

    CopyPtr = (PVOID)(rom + (rom_size - 0x6000 - 0x200));
    
    for (i = 0; i < 16; i++)
	if (entry->szRC4Key[i] != 0)
	    break;

    if (i < 16) { /* If the RC4 key wasn't blank */
	RC4_SBOX RC4State;

	dprintf("Using RC4 key:");

	for (i = 0; i < 16; i++) {
	    dprintf(" 0x%02x", entry->szRC4Key[i]);
	}
	
	dprintf("\nDecrypting 2bl\n");
	XcRC4Key(&RC4State, 16, entry->szRC4Key);
	XcRC4Crypt(&RC4State, 0x6000, CopyPtr);
    } else {
	dprintf("RC4 key is blank in boot.cfg! IS THAT REALLY CORRECT?\n");
    }
    
    /* Copy the 2bl to the appropriate location */

    dprintf("Copying 2bl\n");
    memcpy(Virt2BL, CopyPtr, 0x6000);

    /* Check the 2bl magic */

    if (*(DWORD *)(Virt2BL + THE_2BL_SIZE - 0x2C) != 0x7854794a) {
	dprintf("Trouble with 2bl: bad magic. Possible reasons:\n"
		" - WRONG RC4 KEY\n"
		" - Non-BFM BIOS image (BIOS MUST be Bootable From Media)\n"
		" - Faulty BIOS image\n"
		"Trying to continue anyway.\n");
    }
    
    /* Pick appropriate EEprom key from config if specified */

    version = get_xbox_version();

    if (version == 0) {
	dprintf("Selecting EEPROM 1.0 key.\n");
	eeprom_key = entry->szEEPROMKey1_0;
    } else if (version < 6) {
	dprintf("Selecting EEPROM 1.1 key.\n");
	eeprom_key = entry->szEEPROMKey1_1;
    } else {
	dprintf("Selecting EEPROM 1.6 key.\n");
	eeprom_key = entry->szEEPROMKey1_6;
    }

    /* Patch in the EEprom key if it was specified */

    for (i = 0; i < 16; i++)
	if (eeprom_key[i] != 0)
	    break;

    if (i < 16) {
	dprintf("Patching EEPROM key:");

	for (i = 0; i < 16; i++) {
	    dprintf(" 0x%02x", eeprom_key[i]);
	}
	dprintf("\n");

	memcpy((PVOID)((rom + (rom_size - 0x6000 - 0x200)) + 0x64),
	       eeprom_key, 16);
	memcpy((PVOID)(Virt2BL + 0x64), eeprom_key, 16);
    } else {
	dprintf("EEPROM key is blank! PUT IT IN YOUR BOOT.CFG!\n");
    }

    
    dprintf("Patching kernel param string\n");
    KernelParamPtr = (PUCHAR)0x80400004;
    memcpy(KernelParamPtr, (PUCHAR)" /SHADOW /HDBOOT", 16);
    
    dprintf("Calculating 2bl entry point\n");
    TempPtr = (PVOID)(*(PULONG)Virt2BL + 0x8036FFFC);

    return ((PVOID)(*(PULONG)TempPtr) + 0x80000000);
}




static void setup_video(CONFIGENTRY *entry)
{
    switch (entry->screen) {
    case CFG_SCREEN_KEEP:
	dprintf(" * Keeping current screen\n");
	video_keep();
	break;
    case CFG_SCREEN_BLANK:
	dprintf(" * Blanking screen\n");
	video_blank();
	break;
    case CFG_SCREEN_OFF:
	dprintf(" * Turning video output off\n");
	video_off();
	break;

    case CFG_SCREEN_NOTHING:
    default:
	break;
    }
}
