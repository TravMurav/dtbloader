// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (c) 2024 Nikita Travkin <nikita@trvn.ru> */

#include <efi.h>
#include <efilib.h>
#include <libfdt.h>

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
EFI_STATUS dt_update_mac(void *dtb, const char * const compatibles[], unsigned num,
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

