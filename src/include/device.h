/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef DEVICE_H
#define DEVICE_H

#include <efi.h>

/**
 * struct device - Device description
 * @name:         Pretty marketing name of this device.
 * @dtb:          Name of the DTB file.
 * @hwids:        zero-terminated array of hwid values.
 * @extra_match:  Additional check to match the device.
 * @dt_fixup:     Board specific DTB fixups callback.
 */
struct device {
	CHAR16 *name;
	CHAR16 *dtb;
	EFI_GUID *hwids;

	EFI_STATUS (*extra_match)(struct device *dev);
	EFI_STATUS (*dt_fixup)(struct device *dev, void* dtb);
};

/*
 * NOTE: This should be static but apparently clang is
 * bugged and ignores "used" attribute here... ehhh :(
 */
#define DEVICE_DESC(dev) \
	__declspec(allocate(".devs$b")) \
	__attribute__((used)) \
	struct device *_dtbloader_dev_##dev = (&dev)

/*
 * Puts the description later in the array, useful for
 * "generic" match after more specific ones using
 * .extra_match callback.
 */
#define DEVICE_DESC_END(dev) \
	__declspec(allocate(".devs$c")) \
	__attribute__((used)) \
	struct device *_dtbloader_dev_##dev = (&dev)

struct device *match_device(void);

EFI_STATUS get_board_stable_hash(EFI_SHA1_HASH *hash, CHAR8 *key);

#define MAC_ADDR_SIZE		6

EFI_STATUS dt_set_default_mac(struct device *dev, void *dtb);
EFI_STATUS dt_update_mac(void *dtb, const char * const compatibles[], unsigned num,
			 const char *prop, UINT8 mac[MAC_ADDR_SIZE]);

/* qcom.c */
EFI_STATUS qcom_dt_set_dpp_mac(struct device *dev, void *dtb);

#endif
