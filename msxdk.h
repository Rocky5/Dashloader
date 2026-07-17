#ifndef MSXDK_H
#define MSXDK_H
#include "xtl.h"
#ifdef __cplusplus
extern "C" {
#endif
	void XReboot();
	int  XGetTickCount();
	void XSleep(int milliseconds);
	void XLaunchXBE(const char* path);
	long XUnmount(const char* szDrive);
	long XMount(const char* szDrive, const char* szDevice);
	void XMountRunningXBEDir();
	void XGetXBEName(char* dst, int maxlen);
	void debugPrint(const char*, ...);
	void ShutdownD3D();
	typedef struct _ANSI_STRING
	{
		unsigned short Length;
		unsigned short MaximumLength;
		char* Buffer;
	} ANSI_STRING, *PANSI_STRING;
	extern PANSI_STRING XeImageFileName;
	typedef struct _XBOX_KRNL_VERSION
	{
		unsigned short VersionMajor;
		unsigned short VersionMinor;
		unsigned short Build;
		unsigned short Qfe;
	} XBOX_KRNL_VERSION;
	extern XBOX_KRNL_VERSION* XboxKrnlVersion;
#ifdef __cplusplus
}
#endif
#endif
