// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (c) 2024 Nikita Travkin <nikita@trvn.ru> */

/*
 * Based on Linaro dtbloader implementation.
 * Copyright (c) 2019, Linaro. All rights reserved.
 *
 * https://github.com/aarch64-laptops/edk2/blob/dtbloader-app/EmbeddedPkg/Application/ConfigTableLoader/CHID.c
 */

#include <stdbool.h>
#include <stdarg.h>
#include <efi.h>
#include <efilib.h>
#include <sha1.h>

#include <util.h>
#include <device.h>
#include <chid.h>

/**
 * hash_strings() - Hash a list of strings after concatenating them.
 * @alg:      Algorighm to use from HASH_PROTOCOL.
 * @hash:     Pointer to the result buffer.
 * @seed:     Arbitrary data to prepend to payload.
 * @seed_len: Size of seed.
 * @count:    Amount of strings.
 * @...:  One or more CHAR16 strings to use as message.
 */
static EFI_STATUS hash_strings_sha1(EFI_SHA1_HASH *hash, UINT8 *seed, int seed_len, int count, ...)
{
	SHA1_CTX sha1;
	va_list args;
	int i;

	SHA1Init(&sha1);

	if (seed)
		SHA1Update(&sha1, (void*)seed, seed_len);

	va_start(args, count);

	for (i = 0; i < count; ++i) {
		UINT8 *str = va_arg(args, UINT8*);
		size_t len = StrLen((CHAR16*)str) * sizeof(CHAR16);

		if (len == 0)
			continue;

		SHA1Update(&sha1, (void*)str, len);
	}

	_Static_assert(sizeof(*hash) == 20, "");
	SHA1Final((void*)hash, &sha1);

	va_end(args);
	return EFI_SUCCESS;
}

#define SMBIOS_TYPE_SYSTEM_INFORMATION                   1
#define SMBIOS_TYPE_BASEBOARD_INFORMATION                2

/*
 * gnu-efi doesn't have the full definition for this sadly...
 */
#pragma pack(1)
typedef struct {
	SMBIOS_HEADER   Hdr;
	SMBIOS_STRING   Manufacturer;
	SMBIOS_STRING   ProductName;
	SMBIOS_STRING   Version;
	SMBIOS_STRING   SerialNumber;
	EFI_GUID        Uuid;
	UINT8           WakeUpType;
	SMBIOS_STRING   SKUNumber;
	SMBIOS_STRING   Family;
} SMBIOS_TYPE1_X;
#pragma pack()

struct raw_smbios_info {
	CHAR8 *Manufacturer;
	CHAR8 *ProductName;
	CHAR8 *ProductSku;
	CHAR8 *Family;
	CHAR8 *BaseboardProduct;
	CHAR8 *BaseboardManufacturer;
};

static EFI_STATUS populate_raw_smbios_info(struct raw_smbios_info *info)
{
	EFI_STATUS status;

	SMBIOS_STRUCTURE_TABLE      *SmbiosTable = NULL;
	SMBIOS3_STRUCTURE_TABLE     *Smbios3Table = NULL;
	SMBIOS_STRUCTURE_POINTER    Smbios;

	status = LibGetSystemConfigurationTable(&SMBIOSTableGuid, (VOID**)&SmbiosTable);
	if (EFI_ERROR(status)) {

		status = LibGetSystemConfigurationTable(&SMBIOS3TableGuid, (VOID**)&Smbios3Table);
		if (EFI_ERROR(status))
			return EFI_NOT_FOUND;
	}

	if (SmbiosTable)
		Smbios.Hdr = (SMBIOS_HEADER *)SmbiosTable->TableAddress;
	else if (Smbios3Table)
		Smbios.Hdr = (SMBIOS_HEADER *)Smbios3Table->TableAddress;
	else
		return EFI_NOT_FOUND;

	while (Smbios.Hdr->Type != 127) {

		switch (Smbios.Hdr->Type) {
		case SMBIOS_TYPE_SYSTEM_INFORMATION:
			info->Manufacturer = LibGetSmbiosString(&Smbios, Smbios.Type1->Manufacturer);
			info->ProductName = LibGetSmbiosString(&Smbios, Smbios.Type1->ProductName);
			info->ProductSku = LibGetSmbiosString(&Smbios, ((SMBIOS_TYPE1_X*)Smbios.Raw)->SKUNumber);
			info->Family = LibGetSmbiosString(&Smbios, ((SMBIOS_TYPE1_X*)Smbios.Raw)->Family);
			break;

		case SMBIOS_TYPE_BASEBOARD_INFORMATION:
			info->BaseboardManufacturer = LibGetSmbiosString(&Smbios, Smbios.Type2->Manufacturer);
			info->BaseboardProduct = LibGetSmbiosString(&Smbios, Smbios.Type2->ProductName);
			break;
		}

		LibGetSmbiosString(&Smbios, -1);
	}

	return EFI_SUCCESS;
}

struct smbios_info {
	CHAR16 *Manufacturer;
	CHAR16 *ProductName;
	CHAR16 *ProductSku;
	CHAR16 *Family;
	CHAR16 *BaseboardProduct;
	CHAR16 *BaseboardManufacturer;
};

/**
 * smbios_to_hashable_string() - Convert ascii smbios string to stripped CHAR16.
 */
static CHAR16 *smbios_to_hashable_string(CHAR8* str)
{
	int len, i;
	CHAR16 *ret;

	if (!str) {
		/* User of this function is expected to free the result. */
		ret = AllocateZeroPool(sizeof(*ret));
		return ret;
	}

	/*
	 * We need to strip leading and trailing spaces, leading zeroes.
	 * See fwupd/libfwupdplugin/fu-hwids-smbios.c
	 */
	while (*str == ' ')
		str++;

	while (*str == '0')
		str++;

	len = strlena(str);

	while (len && str[len-1] == ' ')
		len--;

	ret = AllocateZeroPool((len+1) * sizeof(*ret));
	if (!ret)
		return NULL;

	for (i = 0; i < len; ++i)
		ret[i] = str[i];

	return ret;
}

static EFI_STATUS populate_smbios_info(struct smbios_info *info)
{
	struct raw_smbios_info raw;
	EFI_STATUS status;

	status = populate_raw_smbios_info(&raw);
	if (EFI_ERROR(status))
		return status;

	info->Manufacturer		= smbios_to_hashable_string(raw.Manufacturer);
	info->ProductName		= smbios_to_hashable_string(raw.ProductName);
	info->ProductSku		= smbios_to_hashable_string(raw.ProductSku);
	info->Family			= smbios_to_hashable_string(raw.Family);
	info->BaseboardProduct		= smbios_to_hashable_string(raw.BaseboardProduct);
	info->BaseboardManufacturer	= smbios_to_hashable_string(raw.BaseboardManufacturer);

	return EFI_SUCCESS;
}

static void free_smbios_info(struct smbios_info *info)
{
	if (info->Manufacturer)
		FreePool(info->Manufacturer);
	if (info->ProductName)
		FreePool(info->ProductName);
	if (info->ProductSku)
		FreePool(info->ProductSku);
	if (info->Family)
		FreePool(info->Family);
	if (info->BaseboardProduct)
		FreePool(info->BaseboardProduct);
	if (info->BaseboardManufacturer)
		FreePool(info->BaseboardManufacturer);
}

static EFI_STATUS get_chid(struct smbios_info *info, int id, EFI_GUID *chid)
{
	EFI_STATUS status;
	EFI_GUID namespace = { 0x12d8ff70, 0x7f4c, 0x7d4c, { 0 } }; /* Swapped to BE */
	EFI_SHA1_HASH hash = {0};

	switch (id) {
	case 3:
		status = hash_strings_sha1(&hash, (UINT8*)&namespace, sizeof(namespace), 11,
				info->Manufacturer, L"&",
				info->Family, L"&",
				info->ProductName, L"&",
				info->ProductSku, L"&",
				info->BaseboardManufacturer, L"&",
				info->BaseboardProduct);
		break;
	case 4:
		status = hash_strings_sha1(&hash, (UINT8*)&namespace, sizeof(namespace), 7,
				info->Manufacturer, L"&",
				info->Family, L"&",
				info->ProductName, L"&",
				info->ProductSku);
		break;
	case 5:
		status = hash_strings_sha1(&hash, (UINT8*)&namespace, sizeof(namespace), 5,
				info->Manufacturer, L"&",
				info->Family, L"&",
				info->ProductName);
		break;
	case 6:
		status = hash_strings_sha1(&hash, (UINT8*)&namespace, sizeof(namespace), 7,
				info->Manufacturer, L"&",
				info->ProductSku, L"&",
				info->BaseboardManufacturer, L"&",
				info->BaseboardProduct);
		break;
	case 7:
		status = hash_strings_sha1(&hash, (UINT8*)&namespace, sizeof(namespace), 3,
				info->Manufacturer, L"&",
				info->ProductSku);
		break;
	case 8:
		status = hash_strings_sha1(&hash, (UINT8*)&namespace, sizeof(namespace), 7,
				info->Manufacturer, L"&",
				info->ProductName, L"&",
				info->BaseboardManufacturer, L"&",
				info->BaseboardProduct);
		break;
	case 9:
		status = hash_strings_sha1(&hash, (UINT8*)&namespace, sizeof(namespace), 3,
				info->Manufacturer, L"&",
				info->ProductName);
		break;
	case 10:
		status = hash_strings_sha1(&hash, (UINT8*)&namespace, sizeof(namespace), 7,
				info->Manufacturer, L"&",
				info->Family, L"&",
				info->BaseboardManufacturer, L"&",
				info->BaseboardProduct);
		break;
	case 11:
		status = hash_strings_sha1(&hash, (UINT8*)&namespace, sizeof(namespace), 3,
				info->Manufacturer, L"&",
				info->Family);
		break;
	case 13:
		status = hash_strings_sha1(&hash, (UINT8*)&namespace, sizeof(namespace), 5,
				info->Manufacturer, L"&",
				info->BaseboardManufacturer, L"&",
				info->BaseboardProduct);
		break;
	case 14:
		status = hash_strings_sha1(&hash, (UINT8*)&namespace, sizeof(namespace), 1,
				info->Manufacturer);
		break;
	default:
		return EFI_SUCCESS; /* Just keep empty to prevent match. */
	}
	if (EFI_ERROR(status))
		return status;

	CopyMem(chid, hash, sizeof(*chid));

	/* Convert the resulting CHID back to little-endian: */
	chid->Data1 = SwapBytes32(chid->Data1);
	chid->Data2 = SwapBytes16(chid->Data2);
	chid->Data3 = SwapBytes16(chid->Data3);

	/* set specific bits according to RFC4122 Section 4.1.3 */
	chid->Data3    = (chid->Data3 & 0x0fff) | (5 << 12);
	chid->Data4[0] = (chid->Data4[0] & 0x3f) | 0x80;

	return EFI_SUCCESS;
}

/**
 * populate_board_hwids() - Read board SMBIOS and produce an array of CHID values.
 * @hwids:  Pointer to an array of 12 chids to be filled.
 */
EFI_STATUS populate_board_hwids(EFI_GUID *hwids)
{
	EFI_STATUS status;
	struct smbios_info info;
	int i;

	if (!hwids)
		return EFI_INVALID_PARAMETER;

	status = populate_smbios_info(&info);
	if (EFI_ERROR(status))
		return status;

	for (i = 0; i < 15; ++i) {
		status = get_chid(&info, i, &hwids[i]);
		if (EFI_ERROR(status))
			goto exit;
	}

exit:
	free_smbios_info(&info);
	return status;
}
