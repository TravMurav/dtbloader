// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (c) 2024 Nikita Travkin <nikita@trvn.ru> */

#include <stdbool.h>
#include <efi.h>
#include <efilib.h>
#include <libfdt.h>

#include <util.h>
#include <device.h>

#include <protocol/dt_fixup.h>

static const CHAR16 *dtb_locations[] = {
	L"\\dtbloader\\dtbs\\",
	L"\\dtbs\\",
	L"\\",
};

static CHAR16 *basename(CHAR16 *name)
{
	CHAR16 *ret = StrrChr(name, L'\\');

	if (!ret)
		return name;

	return ret + 1;
}

static EFI_FILE_HANDLE open_dtb(EFI_FILE_HANDLE volume, CHAR16 *name)
{
	EFI_FILE_HANDLE dtb_file = NULL;
	CHAR16 dtb_name[128];
	int i;

	for (i = 0; i < ARRAY_SIZE(dtb_locations); ++i) {
		StrnCpy(dtb_name, dtb_locations[i], 32);
		StrnCat(dtb_name, name, ARRAY_SIZE(dtb_name) - 33);

		dtb_file = FileOpen(volume, dtb_name);
		if (dtb_file)
			break;

		/*
		 * Try to be robust and strip vendor dir from the name.
		 * This convention is used by x13s as well as some tools like boot-deploy.
		 */
		StrnCpy(dtb_name, dtb_locations[i], 32);
		StrnCat(dtb_name, basename(name), ARRAY_SIZE(dtb_name) - 33);

		dtb_file = FileOpen(volume, dtb_name);
		if (dtb_file)
			break;
	}

	if (dtb_file)
		Dbg(L"  Found %s\n", dtb_name);

	return dtb_file;
}

static EFI_STATUS load_dtb(EFI_HANDLE ImageHandle, struct device *dev, UINT8 **dtb_ret)
{
	EFI_STATUS status;
	int ret;

	Dbg(L"Installing DTB: %s\n", dev->dtb);

	EFI_FILE_HANDLE volume = GetVolume(ImageHandle);
	if (!volume) {
		Print(L"Cant open volume\n");
		return EFI_INVALID_PARAMETER;
	}

	EFI_FILE_HANDLE dtb_file = open_dtb(volume, dev->dtb);
	if (!dtb_file) {
		Print(L"Cant open the file\n");
		return EFI_INVALID_PARAMETER;
	}

	EFI_PHYSICAL_ADDRESS dtb_phys;
	UINT64 dtb_max_sz = 1 * 1024 * 1024;
	UINT64 dtb_pages  = dtb_max_sz / 4096;

	/* The spec mandates using "ACPI" memory type for any configuration tables like dtb */
	status = uefi_call_wrapper(BS->AllocatePages, 4, AllocateAnyPages, EfiACPIReclaimMemory, dtb_pages, &dtb_phys);
	if (EFI_ERROR(status)) {
		Print(L"Failed to allocate memory: %r\n", status);
		return status;
	}

	UINT8 *dtb    = (UINT8 *)(dtb_phys);
	UINT64 dtb_sz = FileSize(dtb_file);

	if (dtb_sz > 1 * 1024 * 1024) {
		Print(L"File too big!\n");
		status = EFI_BUFFER_TOO_SMALL;
		goto error;
	}

	FileRead(dtb_file, dtb, dtb_sz);
	FileClose(dtb_file);

	ret = fdt_check_header(dtb);
	if (ret) {
		Print(L"fdt header check failed: %d\n", ret);
		status = EFI_LOAD_ERROR;
		goto error;
	}

	ret = fdt_open_into(dtb, dtb, dtb_max_sz);
	if (ret) {
		Print(L"fdt open failed: %d\n", ret);
		status = EFI_LOAD_ERROR;
		goto error;
	}

	*dtb_ret = dtb;

	return EFI_SUCCESS;

error:
	FreePages(dtb_phys, dtb_pages);
	return status;

}

static EFI_STATUS finalize_dtb(UINT8 *dtb)
{
	int ret;

	ret = fdt_pack(dtb);
	if (ret) {
		Print(L"fdt pack failed: %d\n", ret);
		return EFI_LOAD_ERROR;
	}

	return EFI_SUCCESS;
}

static EFI_STATUS apply_dt_fixups(struct device *dev, void *dtb)
{
	EFI_STATUS status;

	if (dev->dt_fixup) {
		status = dev->dt_fixup(dev, dtb);
		if (EFI_ERROR(status)) {
			return status;
		}
	}

	return EFI_SUCCESS;
}

EFI_STATUS efi_dt_fixup(EFI_DT_FIXUP_PROTOCOL *this, void *dtb, UINTN *size, UINT32 flags)
{
	struct device *dev = match_device();
	UINTN extra_space = 4096 * 4;
	EFI_STATUS status;
	int ret;

	if (!dev)
		return EFI_UNSUPPORTED;

	if (fdt_check_header(dtb))
		return EFI_INVALID_PARAMETER;

	if (*size < fdt_totalsize(dtb) + extra_space) {
		*size = fdt_totalsize(dtb) + extra_space;
		return EFI_BUFFER_TOO_SMALL;
	}

	ret = fdt_open_into(dtb, dtb, *size);
	if (ret) {
		Print(L"(dtbloader) fdt open failed: %d\n", ret);
		return EFI_LOAD_ERROR;
	}

	if (flags & EFI_DT_APPLY_FIXUPS) {
		status = apply_dt_fixups(dev, dtb);
		if (EFI_ERROR(status)) {
			Print(L"(dtbloader) Failed to fixup dtb: %r\n", status);
			return status;
		}
	}

	return finalize_dtb(dtb);
}

EFI_DT_FIXUP_PROTOCOL fixup_prot = {
	.Revision = EFI_DT_FIXUP_PROTOCOL_REVISION,
	.Fixup = efi_dt_fixup,
};

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
	EFI_STATUS status;

	InitializeLib(ImageHandle, SystemTable);
	Dbg(L"dtbloader!\n");

	struct device *dev = match_device();

	if (!dev) {
		Print(L"Failed to detect this device!\n");
		return EFI_UNSUPPORTED;
	}

	Print(L"Detected device: %s\n", dev->name);

	UINT8 *dtb = NULL;

	status = load_dtb(ImageHandle, dev, &dtb);
	if (EFI_ERROR(status))
		return status;

	status = apply_dt_fixups(dev, dtb);
	if (EFI_ERROR(status)) {
		Print(L"Failed to fixup dtb: %r\n", status);
		return status;
	}

	status = finalize_dtb(dtb);
	if (EFI_ERROR(status))
		return status;

	EFI_GUID efi_dt_fixup_prot_guid = EFI_DT_FIXUP_PROTOCOL_GUID;
	EFI_HANDLE fixup_handle = NULL;

	status = uefi_call_wrapper(BS->InstallProtocolInterface, 4, &fixup_handle, &efi_dt_fixup_prot_guid,
			EFI_NATIVE_INTERFACE, &fixup_prot);
	if (EFI_ERROR(status)) {
		Print(L"Failed to install fixup protocol: %r\n", status);
		return status;
	}

	EFI_GUID EfiDtbTableGuid = EFI_DTB_TABLE_GUID;

	status = uefi_call_wrapper(BS->InstallConfigurationTable, 2, &EfiDtbTableGuid, dtb);
	if (EFI_ERROR(status)) {
		Print(L"Failed to install dtb: %r\n", status);
		return status;
	}

	return EFI_SUCCESS;
}
