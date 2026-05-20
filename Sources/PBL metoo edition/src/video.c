#include "types.h"
#include "xboxkrnl.h"
#include "boot.h"
#include "video.h"
#include "xbox.h"
#include "strh.h"
#include "printf.h"



/*
 * Detect video encoder. Similar to xbox-linux project's detector in kernel.
 */

encoder_type video_get_encoder_type(void)
{
    static encoder_type type = ENCODER_UNKNOWN;
    static int detected = 0;
    DWORD value;

    if (detected)
	return type;

    detected = 1;
    
    if (NT_SUCCESS(HalReadSMBusValue(0x8a, 0x00, 0, &value))) {
	dprintf(" * Conexant video encoder\n");
	type = ENCODER_CONEXANT;
    }
    else if (NT_SUCCESS(HalReadSMBusValue(0xd4, 0x00, 0, &value))) {
	dprintf(" * Focus video encoder\n");
	type = ENCODER_FOCUS;
    }
    else if (NT_SUCCESS(HalReadSMBusValue(0xe0, 0x01, 0, &value))) {
	dprintf(" * Xcalibur video encoder\n");
	type = ENCODER_XCALIBUR;
    }
    else {
	dprintf(" * Unknown video encoder\n");
	type = ENCODER_UNKNOWN;
    }

    return type;
}



void video_off(void)
{
    switch (video_get_encoder_type()) {
    case ENCODER_CONEXANT:
        IoOutputByte(0x80d3, 0x05);
	HalWriteSMBusValue(0x8a, 0xba, 0, 0x3f);
	break;

    case ENCODER_FOCUS:
	HalWriteSMBusValue(0xd4, 0xa0, 1, 0x020f);
	break;

    case ENCODER_XCALIBUR:
	HalWriteSMBusValue(0xe0, 0x04, 1, 0x0000000f);
	break;

    case ENCODER_UNKNOWN:
    default:
	break;
    }
}



void video_blank(void)
{
    AvSendTVEncoderOption(0, 9, 1, NULL);
}


/*
 * Move current framebuffer to a location where it won't get corrupted by
 * the loaded 2bl and kernel.
 */

void video_keep(void)
{
    void *saved;
    DWORD phys_saved;
    unsigned long saved_size;
    void *ns;
    DWORD phys_ns;
    DWORD phys_frame_data;

    saved = AvGetSavedDataAddress();
    if (!saved) {
	dprintf(" * No saved data, so blanking screen instead.\n");
	video_blank();
	return;
    }

    saved_size = MmQueryAllocationSize(saved);

    ns = MmAllocateContiguousMemoryEx(saved_size,
				      MIN_SHADOW_ROM, MAX_SHADOW_ROM,
				      0, 0x404);
    if (!ns) {
	dprintf(" * Memory allocation failed for new framebuffer.\n");
	return;
    }

    phys_saved = MmGetPhysicalAddress(saved);
    phys_ns = MmGetPhysicalAddress(ns);

    if ((phys_saved + saved_size) > phys_ns &&
	(phys_ns + saved_size) > phys_saved) {
	/* Overlap. Current frame buffer position good enough. */
	MmFreeContiguousMemory(ns);
	return;
    }

    memcpy(ns, saved, saved_size);

    phys_frame_data = *(DWORD *)(saved + 4) - phys_saved + phys_ns;
    *(DWORD *)(ns + 4) = phys_frame_data;

    __asm__ __volatile__(
	"sfence\n"
	"movl	%%eax,0xfd600800\n"
	"1:jmp	1f\n" /* uh, waits needed? */
	"1:jmp	1f\n"
	"1:jmp	1f\n"
	"1:jmp	1f\n"
	"1:\n"
	"movl	$1,0xfd600100\n"
	: /* no output */
	: "a" (phys_frame_data));

    MmPersistContiguousMemory(ns, saved_size, 1); /* serves waits too =) */

    __asm__ __volatile__(
	"1:\n"
	"testb	$1,0xfd600100\n"
	"jz	1b\n"
	);

    AvSetSavedDataAddress(ns);
    MmFreeContiguousMemory(saved);
}



/*
 * Add AVSAVE parameter to a kernel parameter string
 */

int video_add_avsave_param(char *param, unsigned long param_size)
{
    void *saved;
    u32 saved_phys;
    unsigned long saved_size;
    u32 mode;
    char datastr[17];
    u32 *datenc;
    int i;

    if ((strlen(param) + 8 + 16 + 1) > param_size) {
	dprintf("Not enough space for AVSAVE parameter\n");
	return FALSE;
    }
    
    saved = AvGetSavedDataAddress();
    if (!saved)
	return FALSE;
    
    saved_phys = MmGetPhysicalAddress(saved);
    saved_size = MmQueryAllocationSize(saved);
    AvSendTVEncoderOption(0, 13, 0, &mode);
    
    dprintf("saved_phys: 0x%08x, saved_size: 0x%08x, mode: 0x%08x\n",
	    saved_phys, saved_size, mode);
    
    saved_phys >>= 12;
    saved_size >>= 12;
    
    datenc = (u32 *)datastr;
    
    datenc[0] = (((saved_phys & 0xf0f0) >> 4) << 16) +
	((saved_size & 0xf0f0) >> 4);
    datenc[1] = ((saved_phys & 0x0f0f) << 16) +
	(saved_size & 0x0f0f);
    datenc[2] = (mode & 0xf0f0f0f0) >> 4;
    datenc[3] = mode & 0x0f0f0f0f;
    
    for (i = 0; i < 4; i++)
	datenc[i] += 0x61616161;
    
    datastr[16] = 0;
    
    strh_dnzcat(param, " /AVSAVE", param_size);
    strh_dnzcat(param, datastr, param_size);

    return TRUE;
}
