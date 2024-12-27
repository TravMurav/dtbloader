// SPDX-License-Identifier: BSD-3-Clause

#include <efi.h>
#include <device.h>

static EFI_GUID qualcomm_snapdragon_devkit_hwids[] = {
	{ 0xbaa7a649, 0x12d8, 0x56c7, { 0x93, 0xc5, 0xa4, 0xe1, 0x0f, 0x48, 0x52, 0xbe } },
	{ 0xc8e75ab8, 0x555c, 0x5952, { 0xa3, 0xe3, 0x5b, 0x60, 0x7b, 0xea, 0x03, 0x1d } },
	{ 0x4bb05d50, 0x6c4f, 0x525d, { 0xa9, 0xec, 0x89, 0x24, 0xaf, 0xd6, 0xed, 0xea } },
	{ 0x830bd4a2, 0x2498, 0x55cf, { 0xb5, 0x61, 0x48, 0xf7, 0xdc, 0x5f, 0x48, 0x20 } },
	{ 0xb36a40fa, 0x4640, 0x5b1b, { 0x8f, 0xa1, 0x6d, 0xbd, 0xe1, 0x03, 0xc8, 0x0d } },
	{ 0x0d601876, 0x0ac6, 0x533e, { 0x83, 0x86, 0x3a, 0x58, 0x20, 0x3d, 0x8c, 0x33 } },
	{ 0xf37dc44b, 0x0be4, 0x5a70, { 0x86, 0xbd, 0x81, 0xf3, 0xda, 0xcf, 0xf2, 0xe9 } },
	{ 0x9cba20d0, 0x17ad, 0x559f, { 0x94, 0xcd, 0xcf, 0xcb, 0xbf, 0x5f, 0x71, 0xf5 } },
	{ 0xd86bea02, 0x5d71, 0x5ee5, { 0x98, 0xdc, 0x4f, 0x74, 0xd5, 0x77, 0x7d, 0xde } },
	{ }
};

static struct device qualcomm_snapdragon_devkit_dev = {
	.name  = L"Qualcomm Snapdragon-Devkit",
	.dtb   = L"qcom\\x1e001de-devkit.dtb",
	.hwids = qualcomm_snapdragon_devkit_hwids,
};
DEVICE_DESC(qualcomm_snapdragon_devkit_dev);