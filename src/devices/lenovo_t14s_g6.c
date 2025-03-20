// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (c) 2025 Nikita Travkin <nikita@trvn.ru> */

#include <efi.h>
#include <efilib.h>
#include <device.h>

/*
 * https://github.com/aarch64-laptops/build/blob/master/misc/lenovo-thinkpad-t14s-120hz-64gb/acpi/dsdt.dsl
 *
 * Scope (\_SB)
 * {
 *     OperationRegion (MNVS, SystemMemory, 0xD6CF5018, 0x6000)
 *     (...)
 * }
 */
#define T14S_MVNS_BASE      0xD6CF5018
#define T14S_MVNS_SIZE      0x6000

#pragma pack(1)
struct t14s_mvns {
	uint8_t  und0[0xf14];
	uint8_t  und1[7];
	uint8_t  vedx[128];
	uint8_t  shdw;
	uint16_t tpid;
	uint8_t  tpad;
	uint16_t tdvi; /* touchpad vid */
	uint16_t tdpi; /* touchpad pid */
	uint16_t tlvi; /* touchscreen vid */
	uint16_t tlpi; /* touchscreen pid */
	uint8_t  epao;
	uint8_t  tlas;
	uint8_t  fadm;
	uint8_t  vpid; /* panel id (oled=4) */
};
#pragma options align=reset

_Static_assert(T14S_MVNS_BASE + offsetof(struct t14s_mvns, und1) == 0xd6cf5f2c, "");
_Static_assert(T14S_MVNS_BASE + offsetof(struct t14s_mvns, vedx) == 0xd6cf5f2c + 7, "");
_Static_assert(T14S_MVNS_BASE + offsetof(struct t14s_mvns, shdw) == 0xd6cf5fac + 7, "");
_Static_assert(T14S_MVNS_BASE + offsetof(struct t14s_mvns, tpid) == 0xd6cf5fac + 8, "");
_Static_assert(T14S_MVNS_BASE + offsetof(struct t14s_mvns, tpad) == 0xd6cf5fac + 10, "");
_Static_assert(T14S_MVNS_BASE + offsetof(struct t14s_mvns, tdvi) == 0xd6cf5fac + 11, "");
_Static_assert(T14S_MVNS_BASE + offsetof(struct t14s_mvns, tlpi) == 0xd6cf5fbc + 1, "");
_Static_assert(T14S_MVNS_BASE + offsetof(struct t14s_mvns, fadm) == 0xd6cf5fbc + 5, "");


static EFI_STATUS t14s_is_oled(struct device *dev)
{
	struct t14s_mvns *mvns;

	/*
	 * This is theoretically not very safe and nice as we blatantly
	 * hardcode some random address we found in acpi tables and read
	 * from it... The hope here is Lenovo will be too lazy to ever
	 * change this struct and relevant acpi structure layout.
	 */
	mvns = (void*)(uintptr_t)T14S_MVNS_BASE;

	/* Panel XML with index 4 has Backlight Type = 5 and not 1(pmic pwm). */
	return (mvns->vpid == 4 ? EFI_SUCCESS : EFI_UNSUPPORTED);
}

static EFI_GUID lenovo_thinkpad_t14s_gen_6_hwids[] = {
	{ 0xc81fee2f, 0xcf41, 0x5d5a, { 0x8c, 0x7b, 0xaf, 0xd6, 0x58, 0x5b, 0x1d, 0x81 } },
	{ 0xa9b59fea, 0xe841, 0x508a, { 0xa2, 0x45, 0x3a, 0x2d, 0x8d, 0x28, 0x02, 0xde } },
	{ 0x74593764, 0xb6b9, 0x58e9, { 0xbe, 0xdc, 0x93, 0xeb, 0xbb, 0x1e, 0xb0, 0x57 } },
	{ 0xe5d83424, 0x0ecb, 0x5632, { 0xb7, 0xb1, 0x50, 0x0f, 0x04, 0xe8, 0x27, 0x25 } },
	{ 0x76032e78, 0x67a8, 0x5dab, { 0x85, 0x12, 0x15, 0x7b, 0xfc, 0xfb, 0x8f, 0x75 } },
	{ 0xdd83478e, 0xe01b, 0x5631, { 0xae, 0x74, 0x92, 0xae, 0x27, 0x5a, 0x9b, 0x4e } },
	{ 0x791ecd9d, 0x1547, 0x58e6, { 0xb7, 0x2a, 0x5c, 0xe4, 0x17, 0xb7, 0x29, 0xdd } },
	{ 0x8c602147, 0x5363, 0x5374, { 0x85, 0x9e, 0x8b, 0x7f, 0xe2, 0xd4, 0xd3, 0xce } },
	{ 0x498d60ae, 0x9b1d, 0x5b67, { 0x8a, 0xbd, 0xaf, 0x57, 0x1b, 0xab, 0xfa, 0x94 } },
	{ 0xacbac5af, 0xaa6a, 0x5690, { 0x88, 0xf3, 0xe9, 0x10, 0xf0, 0x4a, 0x7e, 0xad } },
	{ 0x5180bc01, 0x5d18, 0x5870, { 0xb9, 0x55, 0x96, 0x9d, 0xa3, 0x8b, 0x26, 0x47 } },
	{ 0x431ff9e9, 0xcd92, 0x51c1, { 0x89, 0x17, 0x46, 0xb0, 0xa0, 0xef, 0x14, 0x7c } },
	{ 0xe093d715, 0x70f7, 0x51f4, { 0xb6, 0xc8, 0xb4, 0xa7, 0xe3, 0x1d, 0xef, 0x85 } },
	{ 0xc124cecf, 0xe6dc, 0x5d35, { 0xa3, 0x20, 0x71, 0x29, 0x80, 0xcf, 0xb6, 0x8d } },
	{ 0x6de5d951, 0xd755, 0x576b, { 0xbd, 0x09, 0xc5, 0xcf, 0x66, 0xb2, 0x72, 0x34 } },
	{ }
};

static struct device lenovo_thinkpad_t14s_gen_6_oled_dev = {
	.name  = L"LENOVO ThinkPad T14s Gen 6 (OLED)",
	.dtb   = L"qcom\\x1e78100-lenovo-thinkpad-t14s-oled.dtb",
	.hwids = lenovo_thinkpad_t14s_gen_6_hwids,

	.extra_match = t14s_is_oled,
};
DEVICE_DESC(lenovo_thinkpad_t14s_gen_6_oled_dev);

static struct device lenovo_thinkpad_t14s_gen_6_dev = {
	.name  = L"LENOVO ThinkPad T14s Gen 6 (IPS)",
	.dtb   = L"qcom\\x1e78100-lenovo-thinkpad-t14s.dtb",
	.hwids = lenovo_thinkpad_t14s_gen_6_hwids,
};
DEVICE_DESC_END(lenovo_thinkpad_t14s_gen_6_dev);
