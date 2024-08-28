// SPDX-License-Identifier: BSD-3-Clause

#include <efi.h>
#include <device.h>

static EFI_GUID lenovo_miix_630_hwids[] = {
	{ 0xc4c9a6be, 0x5383, 0x5de7, { 0xaf, 0x35, 0xc2, 0xde, 0x50, 0x5e, 0xde, 0xc8 } },
	{ 0x14f581d2, 0xd059, 0x5cb2, { 0x9f, 0x8b, 0x56, 0xd8, 0xbe, 0x79, 0x32, 0xc9 } },
	{ 0xa51054fb, 0x5eef, 0x594a, { 0xa5, 0xa0, 0xcd, 0x87, 0x63, 0x2d, 0x0a, 0xea } },
	{ 0x307ab358, 0xed84, 0x57fe, { 0xbf, 0x05, 0xe9, 0x19, 0x5a, 0x28, 0x19, 0x8d } },
	{ 0x7e613574, 0x5445, 0x5797, { 0x95, 0x67, 0x2d, 0x0e, 0xd8, 0x6e, 0x6f, 0xfa } },
	{ 0xb0f4463c, 0xf851, 0x5ec3, { 0xb0, 0x31, 0x2c, 0xcb, 0x87, 0x3a, 0x60, 0x9a } },
	{ 0x08b75d1f, 0x6643, 0x52a1, { 0x9b, 0xdd, 0x07, 0x10, 0x52, 0x86, 0x0b, 0x33 } },
	{ 0xdacf4a59, 0x8e87, 0x55c5, { 0x8b, 0x93, 0x69, 0x12, 0xde, 0xd6, 0xbf, 0x7f } },
	{ 0xd0a8deb1, 0x4cb5, 0x50cd, { 0xbd, 0xda, 0x59, 0x5c, 0xfc, 0x13, 0x23, 0x0c } },
	{ 0x71d86d4d, 0x02f8, 0x5566, { 0xa7, 0xa1, 0x52, 0x9c, 0xef, 0x18, 0x4b, 0x7e } },
	{ 0x6de5d951, 0xd755, 0x576b, { 0xbd, 0x09, 0xc5, 0xcf, 0x66, 0xb2, 0x72, 0x34 } },
	{ 0x34df58d6, 0xb605, 0x50aa, { 0x93, 0x13, 0x9b, 0x34, 0xf5, 0xc4, 0xb6, 0xfc } },
	{ 0xe0a96696, 0xf0a6, 0x5466, { 0xa6, 0xdb, 0x20, 0x7f, 0xbe, 0x8b, 0xae, 0x3c } },
	{ }
};

static struct device lenovo_miix_630_dev = {
	.name  = L"LENOVO Miix 630",
	.dtb   = L"qcom\\msm8998-lenovo-miix-630.dtb",
	.hwids = lenovo_miix_630_hwids,
};
DEVICE_DESC(lenovo_miix_630_dev);
