// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (c) 2024 Nikita Travkin <nikita@trvn.ru> */

#include <efi.h>
#include <efilib.h>
#include <libfdt.h>

#include <util.h>
#include <device.h>

static struct device aspire1_dev = {
	.name = L"Acer Aspire 1",
	.dtb  = L"qcom\\sc7180-acer-aspire1.dtb",
};

struct device *match_device(void)
{
	return &aspire1_dev;
}
