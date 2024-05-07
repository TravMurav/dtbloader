// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (c) 2024 Nikita Travkin <nikita@trvn.ru> */

#include <stdbool.h>
#include <efi.h>
#include <efilib.h>
#include <libfdt.h>

#include <util.h>
#include <device.h>

EFI_STATUS load_dtb(EFI_HANDLE ImageHandle, struct device *dev, UINT8 **dtb_ret)
{
	EFI_STATUS status;
	int ret;

	CHAR16 dtb_name[128] = L"\\dtbs\\";
	StrnCat(dtb_name, dev->dtb, 128 - 17);


	Dbg(L"Installing DTB: %s\n", dtb_name);

	EFI_FILE_HANDLE volume = GetVolume(ImageHandle);
	if (!volume) {
		Print(L"Cant open volume\n");
		return EFI_INVALID_PARAMETER;
	}

	EFI_FILE_HANDLE dtb_file = FileOpen(volume, dtb_name);
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
		Print(L"Failed to allocate memory: %d\n", status);
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

EFI_STATUS finalize_dtb(UINT8 *dtb)
{
	int ret;

	ret = fdt_pack(dtb);
	if (ret) {
		Print(L"fdt pack failed: %d\n", ret);
		return EFI_LOAD_ERROR;
	}

	return EFI_SUCCESS;
}

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


	status = finalize_dtb(dtb);
	if (EFI_ERROR(status))
		return status;

	EFI_GUID EfiDtbTableGuid = EFI_DTB_TABLE_GUID;

	status = uefi_call_wrapper(BS->InstallConfigurationTable, 2, &EfiDtbTableGuid, dtb);
	if (EFI_ERROR(status)) {
		Print(L"Failed to install dtb: %d\n", status);
		return status;
	}

	return EFI_SUCCESS;
}
