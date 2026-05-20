#include "ntfile.h"
#include "xbox.h"
#include "xboxkrnl.h"
#include "strh.h"
#include "printf.h"

int WriteFile(HANDLE Handle, PVOID Buffer, ULONG Size)
{
    IO_STATUS_BLOCK IoStatus;
    
    // Try to write the buffer
    if (!NT_SUCCESS(NtWriteFile(Handle, NULL, NULL, NULL, &IoStatus,
				Buffer, Size, NULL)))
	return 0;
    
    // Verify that the amount written is the correct size
    if (IoStatus.Information != Size)
	return 0;
    
    return 1;
}

int ReadFile(HANDLE Handle, PVOID Buffer, ULONG Size)
{
    IO_STATUS_BLOCK IoStatus;
    
    // Try to write the buffer
    if (!NT_SUCCESS(NtReadFile(Handle, NULL, NULL, NULL, &IoStatus,
			       Buffer, Size, NULL)))
	return 0;
    
    // Verify that the amount read is the correct size
    if (IoStatus.Information != Size)
	return 0;
    
    return 1;
}

int SaveFile(char *szFileName,PBYTE Buffer,ULONG Size)
{
    ANSI_STRING DestFileName;
    IO_STATUS_BLOCK IoStatus;
    OBJECT_ATTRIBUTES Attributes;
    HANDLE DestHandle = NULL;
    
    RtlInitAnsiString(&DestFileName,szFileName);
    Attributes.RootDirectory = NULL;
    Attributes.ObjectName = &DestFileName;
    Attributes.Attributes = OBJ_CASE_INSENSITIVE;
    
    if (!NT_SUCCESS(NtCreateFile(&DestHandle,
				 GENERIC_WRITE  | GENERIC_READ | SYNCHRONIZE,
				 &Attributes, &IoStatus,
				 NULL, FILE_RANDOM_ACCESS,
				 FILE_SHARE_READ, FILE_CREATE,
				 FILE_SYNCHRONOUS_IO_NONALERT |
				 FILE_NON_DIRECTORY_FILE))) {
	dprintf("Error saving File\n");
	return 0;
    }
    
    if(!WriteFile(DestHandle, Buffer, Size)) {
	dprintf("Error saving File\n");
	return 0;
    }
    
    NtClose(DestHandle);
    
    return 1;
}

int AppendFile(char *szFileName, PBYTE Buffer, ULONG Size)
{
    ANSI_STRING DestFileName;
    IO_STATUS_BLOCK IoStatus;
    OBJECT_ATTRIBUTES Attributes;
    HANDLE DestHandle = NULL;
    LONGLONG cursize;
    
    RtlInitAnsiString(&DestFileName,szFileName);
    Attributes.RootDirectory = NULL;
    Attributes.ObjectName = &DestFileName;
    Attributes.Attributes = OBJ_CASE_INSENSITIVE;
    
    if (!NT_SUCCESS(NtCreateFile(&DestHandle,
				 GENERIC_WRITE  | GENERIC_READ | SYNCHRONIZE,
				 &Attributes, &IoStatus,
				 NULL, FILE_RANDOM_ACCESS,
				 FILE_SHARE_READ, FILE_OPEN_IF,
				 FILE_SYNCHRONOUS_IO_NONALERT |
				 FILE_NON_DIRECTORY_FILE))) {
/* 	dprintf("Error opening file (AppendFile): %s\n", szFileName); */
	return 0;
    }

    if (!GetFileSize(DestHandle, &cursize) ||
	!NT_SUCCESS(NtSetInformationFile(DestHandle, &IoStatus, &cursize, 8,
					 FilePositionInformation)) ||
	!WriteFile(DestHandle, Buffer, Size)) {
	
/* 	dprintf("Error writing to file (AppendFile): %s\n", szFileName); */
	NtClose(DestHandle);
	return 0;
    }

    NtClose(DestHandle);
    
    return 1;
}

// Dismount all file systems
void DismountFileSystems(void) {
    
    ANSI_STRING String;
    
    RtlInitAnsiString(&String, "\\Device\\Harddisk0\\Partition1");
    IoDismountVolumeByName(&String);
    RtlInitAnsiString(&String, "\\Device\\Harddisk0\\Partition2");
    IoDismountVolumeByName(&String);
    RtlInitAnsiString(&String, "\\Device\\Harddisk0\\Partition3");
    IoDismountVolumeByName(&String);
    RtlInitAnsiString(&String, "\\Device\\Harddisk0\\Partition4");
    IoDismountVolumeByName(&String);
    RtlInitAnsiString(&String, "\\Device\\Harddisk0\\Partition5");
    IoDismountVolumeByName(&String);
    RtlInitAnsiString(&String, "\\Device\\Harddisk0\\Partition6");
    IoDismountVolumeByName(&String);
}

// Remap the drive to the XBE directory, even if the drive already mapped
int RemapDrive(char *szDrive)
{
    ANSI_STRING LinkName, TargetName;
    char *temp = 0;
    char DDrivePath[256];

    if (XeImageFileName->Length > 255) {
	dprintf("RemapDrive: XeImageFileName too long\n");
	return 0;
    }
   
    // Copy the XBE filename for now
    memcpy(DDrivePath, XeImageFileName->Buffer, XeImageFileName->Length);
    DDrivePath[XeImageFileName->Length] = 0;
    
    // Delete the trailing backslash, chopping off the XBE name, and make it
    // into an ANSI_STRING
    if (!(temp = strrchr(DDrivePath, '\\')))
	return 0;
    *temp = 0;
    
    RtlInitAnsiString(&TargetName, DDrivePath);
    
    // Set up the link
    RtlInitAnsiString(&LinkName, szDrive);
    IoDeleteSymbolicLink(&LinkName);
    
    if(!NT_SUCCESS(IoCreateSymbolicLink(&LinkName, &TargetName))) {
	dprintf("Error IoCreateSymbolicLink\n");
	return 0;
    }
    
    return 1;
}

// Opens a file or directory for read-only access
// Length parameter is negative means use strlen()
// This was originally designed to open directories, but it turned out to be
// too much of a hassle and was scrapped.  Use only for files with the
// FILE_NON_DIRECTORY_FILE mode.
HANDLE OpenFile(HANDLE Root, LPCSTR Filename, LONG Length, ULONG Mode)
{
    ANSI_STRING FilenameString;
    OBJECT_ATTRIBUTES Attributes;
    IO_STATUS_BLOCK IoStatus;
    HANDLE Handle;

    // Initialize the filename string
    // If a length is specified, set up the string manually
    if (Length >= 0)
    {
	FilenameString.Length = (USHORT) Length;
	FilenameString.MaxLength = (USHORT) Length;
	FilenameString.Buffer = (PSTR) Filename;
    }
    // Use RtlInitAnsiString to do it for us
    else
	RtlInitAnsiString(&FilenameString, Filename);
    
    // Initialize the object attributes
    Attributes.Attributes = OBJ_CASE_INSENSITIVE;
    Attributes.RootDirectory = Root;
    Attributes.ObjectName = &FilenameString;

    // Try to open the file or directory
    if (!NT_SUCCESS(NtCreateFile(&Handle, GENERIC_READ | SYNCHRONIZE,
				 &Attributes, &IoStatus, NULL, 0,
				 FILE_SHARE_READ | FILE_SHARE_WRITE
				 | FILE_SHARE_DELETE, FILE_OPEN,
				 FILE_SYNCHRONOUS_IO_NONALERT | Mode))) {
	
	return NULL;
    }
    
    return Handle;
}

// Gets the size of a file
BOOL GetFileSize(HANDLE File, LONGLONG *Size)
{
    FILE_NETWORK_OPEN_INFORMATION SizeInformation;
    IO_STATUS_BLOCK IoStatus;
    
    // Try to retrieve the file size
    if (!NT_SUCCESS(NtQueryInformationFile(File, &IoStatus,
					   &SizeInformation,
					   sizeof(SizeInformation),
					   FileNetworkOpenInformation)))
	return FALSE;
    
    *Size = SizeInformation.EndOfFile.QuadPart;
    return TRUE;
}
