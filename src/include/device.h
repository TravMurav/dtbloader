/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef DEVICE_H
#define DEVICE_H

#include <efi.h>

/**
 * struct device - Device description
 * @name:	Pretty marketing name of this device.
 * @dtb:	Name of the DTB file.
 */
struct device {
	CHAR16 *name;
	CHAR16 *dtb;
};

/*
 * NOTE: This should be static but apparently clang is
 * bugged and ignores "used" attribute here... ehhh :(
 */
#define DEVICE_DESC(dev) \
	__declspec(allocate(".devs$b")) \
	__attribute__((used)) \
	struct device *_dtbloader_dev_##dev = (&dev)

/**
 * match_device() - Detect the device.
 *
 * This function attempts to find the device structure for the
 * machine that dtbloader is running on.
 *
 * Returns: Pointer to the device structure or NULL on failure.
 */
struct device *match_device(void);

#endif
