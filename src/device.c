// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (c) 2024 Nikita Travkin <nikita@trvn.ru> */

#include <efi.h>
#include <efilib.h>
#include <libfdt.h>

#include <protocol/Hash2.h>

#include <util.h>
#include <device.h>
#include <chid.h>

#pragma section(".devs", read)

__declspec(allocate(".devs$a")) struct device *__start_dtbloader_dev = NULL;
__declspec(allocate(".devs$d")) struct device *__stop_dtbloader_dev = NULL;


/**
 * match_device() - Detect the device.
 *
 * This function attempts to find the device structure for the
 * machine that dtbloader is running on.
 *
 * Returns: Pointer to the device structure or NULL on failure.
 */
struct device *match_device(void)
{
	EFI_STATUS status;
	struct device **dev;
	static struct device *cached_dev = NULL;
	EFI_GUID hwids[15] = {0};
	int priority[] = { /* From most to least specific. */
		3,  /* Manufacturer + Family + ProductName + ProductSku + BaseboardManufacturer + BaseboardProduct */
		6,  /* Manufacturer +                        ProductSku + BaseboardManufacturer + BaseboardProduct */
		8,  /* Manufacturer +          ProductName +              BaseboardManufacturer + BaseboardProduct */
		10, /* Manufacturer + Family +                            BaseboardManufacturer + BaseboardProduct */
		4,  /* Manufacturer + Family + ProductName + ProductSku */
		5,  /* Manufacturer + Family + ProductName */
		7,  /* Manufacturer +                        ProductSku */
		9,  /* Manufacturer +          ProductName */
		11, /* Manufacturer + Family */
	};
	int i, j;

	if (cached_dev)
		return cached_dev;

	status = populate_board_hwids(hwids);
	if (EFI_ERROR(status)) {
		Print(L"Failed to populate board hwids: %r\n", status);
		return NULL;
	}

	for (i = 0; i < ARRAY_SIZE(priority); ++i) {
		for (dev = &__start_dtbloader_dev + 1; dev < &__stop_dtbloader_dev; dev++) {
			for (j = 0; (*dev)->hwids[j].Data1; ++j) {
				if (!CompareGuid(&hwids[i], &(*dev)->hwids[j])) {
					if ((*dev)->extra_match && (*dev)->extra_match(*dev) != EFI_SUCCESS)
						continue;

					cached_dev = *dev;
					return *dev;
				}
			}
		}
	}

	return NULL;
}

/**
 * get_board_stable_hash() - Generate SHA1 hash stable for this board.
 * @hash:  Output hash.
 * @key:   Purpose of the hash.
 *
 * This function takes board-specific information and generates a
 * hash of that + @key that should not change on subsequent reboots.
 */
EFI_STATUS get_board_stable_hash(EFI_SHA1_HASH *hash, CHAR8 *key)
{
	EFI_GUID alg = EFI_HASH_ALGORITHM_SHA1_GUID;
	EFI_GUID EfiHash2ProtocolGuid = EFI_HASH2_PROTOCOL_GUID;
	EFI_HASH2_PROTOCOL *prot;
	EFI_GUID board_guid;
	CHAR8 *board_serial;
	EFI_STATUS status;

	if (!key)
		return EFI_INVALID_PARAMETER;

	status = LibLocateProtocol(&EfiHash2ProtocolGuid, (void**)&prot);
	if (EFI_ERROR(status))
		return status;

	status = uefi_call_wrapper(prot->HashInit, 2, prot, &alg);
	if (EFI_ERROR(status))
		return status;

	status = LibGetSmbiosSystemGuidAndSerialNumber(&board_guid, &board_serial);
	if (EFI_ERROR(status))
		return status;

	status = uefi_call_wrapper(prot->HashUpdate, 3, prot, (UINT8*)&board_guid, sizeof(board_guid));
	if (EFI_ERROR(status))
		return status;

	if (board_serial) {
		status = uefi_call_wrapper(prot->HashUpdate, 3, prot, (UINT8*)board_serial, strlena(board_serial));
		if (EFI_ERROR(status))
			return status;
	}

	status = uefi_call_wrapper(prot->HashUpdate, 3, prot, (UINT8*)key, strlena(key));
	if (EFI_ERROR(status))
		return status;

	status = uefi_call_wrapper(prot->HashFinal, 2, prot, (EFI_HASH2_OUTPUT*)hash);
	if (EFI_ERROR(status))
		return status;

	return status;
}

#define MAC_ADDR_SIZE		6

static bool dt_check_existing_mac_prop(void *dtb, int node, const char *prop)
{
	const uint8_t *val;
	int len, i;

	val = fdt_getprop(dtb, node, prop, &len);
	if (!val)
		return false;

	if (len != MAC_ADDR_SIZE)
		return false;

	/* Check if the prop is all zero as placeholder */
	for (i = 0; i < MAC_ADDR_SIZE; ++i)
		if (val[i])
			return true;

	return false;
}

/**
 * dt_update_mac() - Insert mac property if not present.
 * @dtb:	 DT blob.
 * @compatibles: Array of compatibles to add the mac to.
 * @num:	 Count of compatibles.
 * @prop:	 Name of the prop to add (i.e. "local-mac-address")
 * @mac:	 Raw MAC value to add.
 */
static EFI_STATUS dt_update_mac(void *dtb, const char * const compatibles[], unsigned num,
				const char *prop, UINT8 mac[MAC_ADDR_SIZE])
{
	unsigned i;

	for (i = 0; i < num; ++i) {
		int node, ret;

		node = fdt_node_offset_by_compatible(dtb, -1, compatibles[i]);
		if (node == -FDT_ERR_NOTFOUND)
			continue;

		if (dt_check_existing_mac_prop(dtb, node, prop))
			continue;

		ret = fdt_setprop(dtb, node, prop, mac, MAC_ADDR_SIZE);
		if (ret < 0) {
			return EFI_INVALID_PARAMETER;
		}
	}

	return EFI_SUCCESS;
}

/**
 * dt_set_default_mac() - Set generic wifif/bt MAC for this device.
 * @dev:    This device.
 * @dtb:    FDT to fixup.
 *
 * This function updates the @dtb to include board-stable random
 * MAC for BT and WiFi.
 */
EFI_STATUS dt_set_default_mac(struct device *dev, void *dtb)
{
	static UINT8 mac_addr[MAC_ADDR_SIZE];
	static UINT8 bd_addr[MAC_ADDR_SIZE];
	EFI_SHA1_HASH board_hash;
	EFI_STATUS status;
	UINT32 sn;

	const char * const mac_compatibles[] = {
		"qcom,wcnss-wlan",
		"qcom,wcn3990-wifi",
	};
	const char * const bd_compatibles[] = {
		"qcom,wcnss-bt",
		"qcom,wcn3991-bt",
		"qcom,wcn6855-bt",
	};

	if (!mac_addr[0]) {
		status = get_board_stable_hash(&board_hash, "mac");
		if (EFI_ERROR(status))
			return status;

		sn = *(UINT32*)board_hash;

		/* locally administrated pool */
		mac_addr[0] = 0x02;
		mac_addr[1] = 0x00;
		mac_addr[2] = sn >> 24;
		mac_addr[3] = sn >> 16;
		mac_addr[4] = sn >> 8;
		mac_addr[5] = sn;

		/*
		 * BD address is encoded in little endian format (reversed),
		 * with least significant bit flipped.
		 */
		bd_addr[5] = 0x02;
		bd_addr[4] = 0x00;
		bd_addr[3] = sn >> 24;
		bd_addr[2] = sn >> 16;
		bd_addr[1] = sn >> 8;
		bd_addr[0] = sn ^ 1;

		Dbg(L"Generated MAC address %02x:%02x:%02x:%02x:%02x:%02x, "
				"BD address %02x:%02x:%02x:%02x:%02x:%02x\n",
				mac_addr[0], mac_addr[1], mac_addr[2],
				mac_addr[3], mac_addr[4], mac_addr[5],
				bd_addr[5], bd_addr[4], bd_addr[3],
				bd_addr[2], bd_addr[1], bd_addr[0]);
	}

	status = dt_update_mac(dtb, mac_compatibles, ARRAY_SIZE(mac_compatibles), "local-mac-address", mac_addr);
	if (EFI_ERROR(status))
		return status;

	status = dt_update_mac(dtb, bd_compatibles, ARRAY_SIZE(bd_compatibles), "local-bd-address", bd_addr);
	if (EFI_ERROR(status))
		return status;

	return EFI_SUCCESS;
}
