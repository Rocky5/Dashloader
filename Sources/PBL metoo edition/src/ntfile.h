#ifndef _NTFILE_H_
#define _NTFILE_H_

#include "types.h"

int ReadFile(HANDLE Handle, PVOID Buffer, ULONG Size);
int WriteFile(HANDLE Handle, PVOID Buffer, ULONG Size);
int SaveFile(char *szFileName,PBYTE Buffer,ULONG Size);
int AppendFile(char *szFileName,PBYTE Buffer,ULONG Size);
void DismountFileSystems(void);
int RemapDrive(char *szDrive);
HANDLE OpenFile(HANDLE Root, LPCSTR Filename, LONG Length, ULONG Mode);
BOOL GetFileSize(HANDLE File, LONGLONG *Size);

#endif
