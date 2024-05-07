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
