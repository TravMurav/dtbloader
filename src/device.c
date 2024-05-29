// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (c) 2024 Nikita Travkin <nikita@trvn.ru> */

#include <efi.h>
#include <efilib.h>
#include <libfdt.h>

#include <util.h>
#include <device.h>
#include <chid.h>

#pragma section(".devs", read)

__declspec(allocate(".devs$a")) struct device *__start_dtbloader_dev = NULL;
__declspec(allocate(".devs$c")) struct device *__stop_dtbloader_dev = NULL;


struct device *match_device(void)
{
	EFI_STATUS status;
	struct device **dev;
	EFI_GUID hwids[15] = {0};
	int priority[] = {3, 6, 8, 10, 4, 5 , 7, 9, 11}; /* From most to least specific. */
	int i, j;

	status = populate_board_hwids(hwids);
	if (EFI_ERROR(status)) {
		Print(L"Failed to populate board hwids: %r\n", status);
		return NULL;
	}

	for (i = 0; i < ARRAY_SIZE(priority); ++i) {
		for (dev = &__start_dtbloader_dev + 1; dev < &__stop_dtbloader_dev; dev++) {
			for (j = 0; (*dev)->hwids[j].Data1; ++j) {
				if (!CompareGuid(&hwids[i], &(*dev)->hwids[j])) {
					return *dev;
				}
			}
		}
	}

	return NULL;
}
