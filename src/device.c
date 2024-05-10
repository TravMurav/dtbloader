// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (c) 2024 Nikita Travkin <nikita@trvn.ru> */

#include <efi.h>
#include <efilib.h>
#include <libfdt.h>

#include <util.h>
#include <device.h>

#pragma section(".devs", read)

__declspec(allocate(".devs$a")) struct device *__start_dtbloader_dev = NULL;
__declspec(allocate(".devs$c")) struct device *__stop_dtbloader_dev = NULL;


struct device *match_device(void)
{
	struct device **dev = &__start_dtbloader_dev + 1;

	for (; dev < &__stop_dtbloader_dev; dev++) {
		return *dev;
	}

	return NULL;
}
