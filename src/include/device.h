/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef DEVICE_H
#define DEVICE_H

#include <efi.h>

/**
 * struct device - Device description
 * @name:	Pretty marketing name of this device.
 * @dtb:	Name of the DTB file.
 * @hwids:	zero-terminated array of hwid values.
 * @dt_fixup:	Board specific DTB fixups callback.
 */
struct device {
	CHAR16 *name;
	CHAR16 *dtb;
	EFI_GUID *hwids;

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

struct device *match_device(void);

EFI_STATUS get_board_stable_hash(EFI_SHA1_HASH *hash, CHAR8 *key);
EFI_STATUS dt_set_default_mac(struct device *dev, void *dtb);

#endif
