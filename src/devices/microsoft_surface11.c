// SPDX-License-Identifier: BSD-3-Clause

#include <efi.h>
#include <device.h>

static EFI_GUID microsoft_corporation_microsoft_surface_pro__11th_edition_hwids[] = {
	{ 0x66f9d954, 0x5c66, 0x5577, { 0xb3, 0xe4, 0xe3, 0xf1, 0x4f, 0x87, 0xd2, 0xff } },
	{ 0x5a384f15, 0x464d, 0x5da8, { 0x93, 0x11, 0xa2, 0xc0, 0x21, 0x75, 0x9a, 0xfc } },
	{ 0x14b96570, 0x4bc4, 0x541a, { 0x9a, 0xef, 0x1b, 0x7e, 0x2b, 0x61, 0xd7, 0xcd } },
	{ 0xaca467c0, 0x5fc2, 0x59ad, { 0x8e, 0xd5, 0x1b, 0x7a, 0x09, 0x88, 0xd1, 0x1c } },
	{ 0x95971fb3, 0xd478, 0x591f, { 0x9e, 0xa3, 0xeb, 0x0a, 0xf0, 0xd1, 0xdf, 0xb5 } },
	{ 0xc9c14db9, 0x2b61, 0x597a, { 0xa4, 0xba, 0x84, 0x39, 0x7f, 0xe7, 0x5f, 0x63 } },
	{ 0x7cef06f5, 0xe7e6, 0x56d7, { 0xb1, 0x23, 0xa6, 0xd6, 0x40, 0xa5, 0xd3, 0x02 } },
	{ 0x48b86a5e, 0x1955, 0x5799, { 0x95, 0x77, 0x15, 0x0f, 0x9e, 0x1a, 0x69, 0xe4 } },
	{ 0x06128fee, 0x87dc, 0x50f6, { 0x8a, 0x3f, 0x97, 0xcd, 0x9a, 0x6d, 0x8b, 0xf6 } },
	{ 0x84b2e1d1, 0xe695, 0x5f41, { 0x8c, 0x41, 0xcf, 0x1f, 0x05, 0x9c, 0x61, 0x6a } },
	{ 0x16a47337, 0x1f8b, 0x5bd3, { 0xb3, 0xbd, 0x8e, 0x50, 0xb3, 0x1c, 0xb1, 0xc9 } },
	{ 0xca2e5189, 0x1d32, 0x509f, { 0x88, 0xa0, 0xd4, 0xeb, 0xcc, 0x72, 0x18, 0x99 } },
	{ 0xaca387a9, 0x183e, 0x5da9, { 0x8f, 0x9d, 0xf4, 0x60, 0xc3, 0xf5, 0x0f, 0x54 } },
	{ 0xfdef4ae0, 0x6bfb, 0x5706, { 0x8a, 0xae, 0xa5, 0x65, 0x63, 0x95, 0x05, 0xf5 } },
	{ 0xcc0aea32, 0xad2c, 0x5013, { 0x8b, 0xed, 0xce, 0xde, 0x6b, 0xe8, 0xc9, 0xf4 } },
	{ 0x01bf1e61, 0xd2e0, 0x518b, { 0xbb, 0x46, 0xeb, 0x4d, 0x1f, 0x2b, 0x1a, 0xf1 } },
	{ 0x584a5084, 0x15f2, 0x5d20, { 0x91, 0x7b, 0x57, 0xf2, 0x99, 0xe6, 0x1f, 0x7e } },
	{ 0x9914cecc, 0xaab7, 0x570e, { 0x8f, 0xce, 0xe8, 0x60, 0x09, 0xea, 0x6b, 0xbb } },
	{ }
};

static struct device microsoft_corporation_microsoft_surface_pro__11th_edition_dev = {
	.name  = L"Microsoft Corporation Microsoft Surface Pro, 11th Edition",
	.dtb   = L"qcom\\x1e80100-microsoft-denali.dtb", /* Tentative. */
	.hwids = microsoft_corporation_microsoft_surface_pro__11th_edition_hwids,

	.dt_fixup = qcom_dt_set_dpp_mac,
};
DEVICE_DESC(microsoft_corporation_microsoft_surface_pro__11th_edition_dev);
