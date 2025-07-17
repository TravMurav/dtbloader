// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (c) 2024 Nikita Travkin <nikita@trvn.ru> */

#include <efi.h>
#include <efilib.h>
#include <libfdt.h>

#include <util.h>
#include <device.h>
#include <chid.h>

#define EFI_PARTITION_INFO_PROTOCOL_GUID \
  { 0x8cf2f62c, 0xbc9b, 0x4821, {0x80, 0x8d, 0xec, 0x9e, 0xc4, 0x21, 0xa1, 0xa0} }

typedef struct {
  EFI_GUID     PartitionTypeGUID;
  EFI_GUID     UniquePartitionGUID;
  EFI_LBA      StartingLBA;
  EFI_LBA      EndingLBA;
  UINT64       Attributes;
  CHAR16       PartitionName[36];
} __attribute__((packed)) EFI_PARTITION_ENTRY;

#define EFI_PARTITION_INFO_PROTOCOL_REVISION 0x0001000
#define PARTITION_TYPE_OTHER 0x00
#define PARTITION_TYPE_MBR 0x01
#define PARTITION_TYPE_GPT 0x02

typedef struct {

  UINT32         Revision;
  UINT32         Type;
  UINT8          System;
  UINT8          Reserved[7];
  union {
   MBR_PARTITION_RECORD Mbr;
   EFI_PARTITION_ENTRY Gpt;
  } Info;
} __attribute__((packed)) EFI_PARTITION_INFO_PROTOCOL;

/**
 * locate_gpt_partition() - Get a handle to a partition with specific GPT name.
 */
static EFI_STATUS locate_gpt_partition(CHAR16 *name, EFI_HANDLE *partition_handle)
{
	EFI_GUID gEfiPartitionInfoProtocol = EFI_PARTITION_INFO_PROTOCOL_GUID;
	EFI_STATUS status;
	EFI_HANDLE *disk_handles;
	UINTN disk_count;
	UINTN i;

	status = LibLocateHandle(ByProtocol, &gEfiDiskIoProtocolGuid, NULL, &disk_count, &disk_handles);
	if (EFI_ERROR(status))
		return status;

	for (i = 0; i < disk_count; ++i) {
		EFI_PARTITION_INFO_PROTOCOL *partition;
		status = uefi_call_wrapper(BS->HandleProtocol, 3, disk_handles[i], &gEfiPartitionInfoProtocol, (void*)&partition);
		if (EFI_ERROR(status))
			continue;

		if (partition->Type != PARTITION_TYPE_GPT)
			continue;

		if (!StrCmp(name, partition->Info.Gpt.PartitionName)) {
			*partition_handle = disk_handles[i];
			FreePool(disk_handles);
			return EFI_SUCCESS;
		}
	}

	FreePool(disk_handles);
	return EFI_NOT_FOUND;
}

/**
 * locate_partition_by_magic() - Find partition handle with first bytes matching magic.
 */
static EFI_STATUS locate_partition_by_magic(const UINT8 *magic, UINTN len, EFI_HANDLE *partition_handle)
{
	EFI_STATUS status;
	EFI_HANDLE *disk_handles;
	UINTN disk_count;
	UINTN i;
	UINT8 tmp[64];

	ASSERT(sizeof(tmp) >= len);

	status = LibLocateHandle(ByProtocol, &gEfiDiskIoProtocolGuid, NULL, &disk_count, &disk_handles);
	if (EFI_ERROR(status))
		return status;

	for (i = 0; i < disk_count; ++i) {
		EFI_BLOCK_IO_PROTOCOL *block_io;
		EFI_DISK_IO_PROTOCOL *disk_io;

		status = uefi_call_wrapper(BS->HandleProtocol, 3, disk_handles[i], &gEfiBlockIoProtocolGuid, (void*)&block_io);
		if (EFI_ERROR(status))
			continue;

		status = uefi_call_wrapper(BS->HandleProtocol, 3, disk_handles[i], &gEfiDiskIoProtocolGuid, (void*)&disk_io);
		if (EFI_ERROR(status))
			continue;

		status = uefi_call_wrapper(disk_io->ReadDisk, 5, disk_io, block_io->Media->MediaId, 0, len, tmp);
		if (EFI_ERROR(status))
			continue;

		if (!memcmp(magic, tmp, len)) {
			*partition_handle = disk_handles[i];
			FreePool(disk_handles);
			return EFI_SUCCESS;
		}
	}

	FreePool(disk_handles);
	return EFI_NOT_FOUND;
}

static const UINT8 dpp_magic[] = { 0x52, 0x57, 0x46, 0x53 }; /* 'RWFS' */

struct rwfs_header {
    UINT8  magic[4];	/* RWFS */
    UINT32 unk1;
    UINT32 hdr2_offt;	/* Points to rwfs_second */
    UINT32 unk2;
    UINT32 unk3;
    UINT32 unk4;
    UINT32 data_start;	/* this + offt in table entry to get blob start */
    UINT32 header_size; /* whole header inc 9 blobs */
    UINT32 full_len;	/* seems like full partition len */
    UINT8  pad[32];
} __attribute__((packed));

struct rwfs_second {
    UINT8  magic[4];	/* RWFS */
    UINT32 unk1;
    UINT32 table_offt;	/* Points to first blob_entry */
    UINT32 table_size;	/* size of all blob_entry together, some are empty */
    UINT32 unk2;
    UINT32 data_start;	/* same as in first hdr? */
    UINT32 funk3;
    UINT8  pad[40];
} __attribute__((packed));

struct rwfs_blob {
    CHAR16 name[49];   /* file name */
    CHAR16 vendor[49]; /* QCOM or OEM */
    UINT32 region_len; /* Full, 0x100 aligned region size */
    UINT32 data_len;   /* Actual meaningful data size */
    UINT32 present;    /* Seems to always be 0x1 for filled entries. */
    UINT32 offt;       /* Offset from data_start to the start of the region with data */
    UINT8  pad[32];
} __attribute__((packed));


/**
 * qcom_dpp_read_file() - Read file data from DPP.
 * This allocates a pool with file data that should be freed.
 */
static EFI_STATUS qcom_dpp_read_file(EFI_HANDLE dpp_partition, CHAR16 *file_name, UINT8 **buf_ret, UINTN *len_ret)
{
	EFI_STATUS status;
	UINT8 *buf;
	UINTN len;
	EFI_BLOCK_IO_PROTOCOL *block_io;
	EFI_DISK_IO_PROTOCOL *disk_io;
	struct rwfs_header header;
	struct rwfs_second second_hdr;
	struct rwfs_blob   blob;
	UINTN offt;

	status = uefi_call_wrapper(BS->HandleProtocol, 3, dpp_partition, &gEfiBlockIoProtocolGuid, (void*)&block_io);
	if (EFI_ERROR(status))
		return status;

	status = uefi_call_wrapper(BS->HandleProtocol, 3, dpp_partition, &gEfiDiskIoProtocolGuid, (void*)&disk_io);
	if (EFI_ERROR(status))
		return status;

	status = uefi_call_wrapper(disk_io->ReadDisk, 5, disk_io, block_io->Media->MediaId, 0, sizeof(header), &header);
	if (EFI_ERROR(status))
		return status;

	if (!!memcmp(dpp_magic, header.magic, sizeof(dpp_magic))) {
		return EFI_UNSUPPORTED;
	}

	status = uefi_call_wrapper(disk_io->ReadDisk, 5, disk_io, block_io->Media->MediaId, header.hdr2_offt, sizeof(second_hdr), &second_hdr);
	if (EFI_ERROR(status))
		return status;

	for (offt = 0; offt < second_hdr.table_size; offt += sizeof(blob)) {
		status = uefi_call_wrapper(disk_io->ReadDisk, 5, disk_io, block_io->Media->MediaId, second_hdr.table_offt + offt, sizeof(blob), &blob);
		if (EFI_ERROR(status))
			return status;

		if (!blob.present)
			break;

		if (!StriCmp(file_name, blob.name)) {
			len = blob.data_len;
			buf = AllocatePool(len);

			status = uefi_call_wrapper(disk_io->ReadDisk, 5, disk_io, block_io->Media->MediaId, second_hdr.data_start + blob.offt, len, buf);
			if (EFI_ERROR(status))
				return status;

			Dbg(L"DPP: Found %s/%s with %d bytes.\n", blob.vendor, blob.name, blob.data_len);
			*buf_ret = buf;
			*len_ret = len;

			return EFI_SUCCESS;
		}
	}

	return EFI_NOT_FOUND;
}

/**
 * locate_dpp() - Locate DPP partition on qcom devices.
 */
static EFI_STATUS locate_dpp(EFI_HANDLE *partition_handle)
{
	EFI_STATUS status;
	EFI_HANDLE dpp_partition;

	status = locate_gpt_partition(L"DPP", &dpp_partition);
	if (EFI_ERROR(status) && status != EFI_NOT_FOUND)
		return status;

	if (status == EFI_NOT_FOUND) {
		status = locate_partition_by_magic(dpp_magic, sizeof(dpp_magic), &dpp_partition);
		if (EFI_ERROR(status))
			return status;
	}

	*partition_handle = dpp_partition;
	return EFI_SUCCESS;
}

struct wlan_provision {
	UINT8 unk[3];
	UINT8 mac[6];
} __attribute__((packed));

struct bt_provision {
	UINT8 unk[2];
	UINT8 mac[6];
} __attribute__((packed));

/**
 * qcom_dt_set_dpp_mac() - Set wifif/bt MAC for this device.
 * @dev:    This device.
 * @dtb:    FDT to fixup.
 *
 * This function updates the @dtb to include MAC for BT and WiFi
 * as found on the DPP partition.
 */
EFI_STATUS qcom_dt_set_dpp_mac(struct device *dev, void *dtb)
{
	EFI_STATUS status;
	EFI_HANDLE dpp_partition;
	struct wlan_provision *wlan_file;
	UINTN wlan_file_len;
	struct bt_provision *bt_file;
	UINTN bt_file_len;
	UINT8 bd_addr[6];

	const char * const mac_compatibles[] = {
		"qcom,wcnss-wlan",
		"qcom,wcn3990-wifi",
		"pci17cb,1103",
	};
	const char * const bd_compatibles[] = {
		"qcom,wcnss-bt",
		"qcom,wcn3991-bt",
		"qcom,wcn6855-bt",
	};

	status = locate_dpp(&dpp_partition);
	if (EFI_ERROR(status))
		return status;

	status = qcom_dpp_read_file(dpp_partition, L"WLAN.PROVISION", (UINT8**)&wlan_file, &wlan_file_len);
	if (EFI_ERROR(status))
		return status;

	status = qcom_dpp_read_file(dpp_partition, L"BT.PROVISION", (UINT8**)&bt_file, &bt_file_len);
	if (EFI_ERROR(status)) {
		FreePool(wlan_file);
		return status;
	}

	if (wlan_file_len != sizeof(struct wlan_provision) || bt_file_len != sizeof(struct bt_provision)) {
		Print(L"DPP mac format is not supported.");
		FreePool(wlan_file);
		FreePool(bt_file);
		return EFI_UNSUPPORTED;
	}

	/*
	 * BD address is encoded in little endian format (reversed),
	 * with least significant bit flipped.
	 */
	bd_addr[5] = bt_file->mac[0];
	bd_addr[4] = bt_file->mac[1];
	bd_addr[3] = bt_file->mac[2];
	bd_addr[2] = bt_file->mac[3];
	bd_addr[1] = bt_file->mac[4];
	bd_addr[0] = bt_file->mac[5];

	Dbg(L"DPP MAC address %02x:%02x:%02x:%02x:%02x:%02x, "
		"BD address %02x:%02x:%02x:%02x:%02x:%02x\n",
		wlan_file->mac[0], wlan_file->mac[1], wlan_file->mac[2],
		wlan_file->mac[3], wlan_file->mac[4], wlan_file->mac[5],
		bd_addr[5], bd_addr[4], bd_addr[3],
		bd_addr[2], bd_addr[1], bd_addr[0]);

	status = dt_update_mac(dtb, mac_compatibles, ARRAY_SIZE(mac_compatibles), "local-mac-address", wlan_file->mac);
	if (EFI_ERROR(status))
		return status;

	status = dt_update_mac(dtb, bd_compatibles, ARRAY_SIZE(bd_compatibles), "local-bd-address", bd_addr);
	if (EFI_ERROR(status))
		return status;

	FreePool(wlan_file);
	FreePool(bt_file);
	return EFI_SUCCESS;
}
