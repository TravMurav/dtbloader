#!/bin/sh
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2024 Nikita Travkin <nikita@trvn.ru>

cd "$(dirname $0)/.." 

grep -h ".dtb" src/devices/* | sed -e 's/.*L"\(.*\)",/\1/' -e 's_\\\\_/_'
