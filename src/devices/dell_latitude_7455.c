// SPDX-License-Identifier: BSD-3-Clause

#include <efi.h>
#include <device.h>

static EFI_GUID dell_latitude_7455_hwids[] = {
	{ 0xb05fb751, 0x0fcd, 0x578a, { 0x81, 0x67, 0x72, 0x40, 0xf5, 0x27, 0xbf, 0x8e } },
	{ 0x8bbe868f, 0x51ec, 0x5147, { 0x9f, 0x89, 0xaa, 0x74, 0xad, 0xc2, 0xab, 0x57 } },
	{ 0x4b6ff7fb, 0xb48e, 0x5a4b, { 0x9c, 0xc0, 0x3c, 0xb2, 0xe3, 0xd9, 0x52, 0x85 } },
	{ 0x71aa7285, 0x85cc, 0x5981, { 0x98, 0x0c, 0x89, 0x97, 0xbb, 0x8f, 0x33, 0x5f } },
	{ 0x366393b3, 0x317d, 0x50aa, { 0x8b, 0x56, 0xd4, 0xc0, 0x41, 0xcc, 0x95, 0x16 } },
	{ 0x2b9277cd, 0x85b1, 0x51ee, { 0x9a, 0x38, 0xd4, 0x77, 0x63, 0x25, 0x32, 0xda } },
	{ 0x2fd2f214, 0xa724, 0x5a61, { 0xa4, 0xb0, 0xb1, 0x43, 0x44, 0x4c, 0x55, 0xc9 } },
	{ 0x351c2902, 0x6e75, 0x5441, { 0x9b, 0x90, 0x31, 0xfa, 0x56, 0x1f, 0x39, 0x55 } },
	{ 0x0ede04d5, 0xf7f4, 0x5738, { 0x89, 0xc5, 0x47, 0x92, 0x43, 0x83, 0x9c, 0x0b } },
	{ 0x4f73f73b, 0xe639, 0x5353, { 0xbb, 0xf7, 0xd8, 0x51, 0xe4, 0x8f, 0x18, 0xfc } },
	{ 0x47d034a3, 0x8b0c, 0x5a59, { 0x8f, 0xbf, 0x61, 0x20, 0x4d, 0xcb, 0x2a, 0x27 } },
	{ 0x41b0b5d7, 0xad12, 0x5d86, { 0x9a, 0x3d, 0x82, 0x6f, 0x9b, 0x20, 0xad, 0xcc } },
	{ 0x5c163113, 0x9296, 0x5c8d, { 0xa9, 0x2d, 0xae, 0x04, 0xeb, 0x59, 0xa0, 0xf7 } },
	{ 0x5455944b, 0x1a7d, 0x5678, { 0x9c, 0xae, 0x0f, 0xfd, 0xa6, 0xff, 0xa2, 0x56 } },
	{ 0x85d38fda, 0xfc0e, 0x5c6f, { 0x80, 0x8f, 0x07, 0x69, 0x84, 0xae, 0x79, 0x78 } },
	{ 0xad4ce233, 0xb03c, 0x5b5d, { 0xbf, 0x42, 0xa4, 0x4b, 0xe5, 0xb7, 0xee, 0x32 } },
	{ 0xfb7493ec, 0x9634, 0x5c5a, { 0x9f, 0x26, 0x69, 0xcb, 0xf9, 0xb9, 0x24, 0x60 } },
	{ 0x36cb5c6e, 0xfb91, 0x55e9, { 0x80, 0x77, 0xe0, 0x04, 0xf2, 0xb1, 0xdd, 0xad } },
	{ }
};

static struct device dell_latitude_7455_dev = {
	.name  = L"Dell Inc. Latitude 7455",
	.dtb   = L"qcom\\x1e80100-dell-latitude-7455.dtb", /* Tentative */
	.hwids = dell_latitude_7455_hwids,
};
DEVICE_DESC(dell_latitude_7455_dev);
