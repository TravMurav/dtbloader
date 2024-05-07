/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef FIXUP_H
#define FIXUP_H

#include <efi.h>

/*
 * EFI_DT_FIXUP_PROTOCOL
 *
 * Documented in https://github.com/U-Boot-EFI/EFI_DT_FIXUP_PROTOCOL
 */

#define EFI_DT_FIXUP_PROTOCOL_GUID \
	{ 0xe617d64c, 0xfe08, 0x46da, { 0xf4, 0xdc, 0xbb, 0xd5, 0x87, 0x0c, 0x73, 0x00 } }
#define EFI_DT_FIXUP_PROTOCOL_REVISION 0x00010000

typedef EFI_STATUS
(EFIAPI *EFI_DT_FIXUP) (
		IN EFI_DT_FIXUP_PROTOCOL *This,
		IN VOID                  *Fdt,
		IN OUT UINTN             *BufferSize,
		IN UINT32                Flags
		);

typedef struct _EFI_DT_FIXUP_PROTOCOL {
	UINT64         Revision;
	EFI_DT_FIXUP   Fixup;
} EFI_DT_FIXUP_PROTOCOL;

/* Add nodes and update properties */
#define EFI_DT_APPLY_FIXUPS    0x00000001
/*
 * Reserve memory according to the /reserved-memory node
 * and the memory reservation block
 */
#define EFI_DT_RESERVE_MEMORY  0x00000002

#endif
