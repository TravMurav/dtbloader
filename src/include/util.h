#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>
#include <efi.h>

#ifdef EFI_DEBUG
	#define Dbg Print
#else
	#define Dbg(...) DbgPrint(D_INFO, __VA_ARGS__)
#endif


EFI_FILE_HANDLE GetVolume(EFI_HANDLE image);
EFI_FILE_HANDLE FileOpen(EFI_FILE_HANDLE Volume, CHAR16 *FileName);
UINT64 FileSize(EFI_FILE_HANDLE FileHandle);
UINT64 FileRead(EFI_FILE_HANDLE FileHandle, UINT8 *Buffer, UINT64 ReadSize);
void FileClose(EFI_FILE_HANDLE FileHandle);

EFI_STATUS AllocateZeroPages(UINT64 page_count, EFI_PHYSICAL_ADDRESS *addr);
void FreePages(EFI_PHYSICAL_ADDRESS addr, UINT64 page_count);

CHAR16 *StrrChr(CHAR16 *str, CHAR16 ch);

EFI_STATUS Hash2Sha1(UINT8 *msg, UINTN size, EFI_SHA1_HASH *hash);
bool SecureBootEnabled(void);

#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))

static inline UINT16 SwapBytes16(UINT16 Value)
{
	return (UINT16) ((Value<< 8) | (Value>> 8));
}

static inline UINT32 SwapBytes32(UINT32 Value)
{
	UINT32  LowerBytes;
	UINT32  HigherBytes;

	LowerBytes  = (UINT32) SwapBytes16 ((UINT16) Value);
	HigherBytes = (UINT32) SwapBytes16 ((UINT16) (Value >> 16));
	return (LowerBytes << 16 | HigherBytes);
}

#endif
