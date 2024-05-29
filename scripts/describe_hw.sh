#!/bin/sh
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2024 Nikita Travkin <nikita@trvn.ru>

if [ $# -eq 1 ]
then
	hwids_data="$(cat "$1")"
else
	hwids_data="$(fwupdtool hwids)"
fi

get_field() {
	name="$1"
	echo "$hwids_data" | grep -m1 "$name" | sed 's/.*: //'
}

var_prefix="$(echo $(get_field "Manufacturer") $(get_field "Family") | tr '[:upper:]' '[:lower:]' | sed 's/\(\W\|-\)/_/g')"

echo "static EFI_GUID ""$var_prefix""_hwids[] = {"

# If it's stupid but it works...
echo "$hwids_data" \
	| sed -n '18,32 p' | sed \
		-e 's/^/\t/' \
		-e 's/   <-.*//' \
		-e 's/-/, 0x/g' \
		-e 's/{\(\w*\), \(.*\), 0x\(..\)\(..\), 0x\(.*\)}/{ 0x\1, \2, { 0x\3, 0x\4, _\5_ } },/' \
		-e 's/_\(..\)\(..\)\(..\)\(..\)\(..\)\(..\)_/0x\1, 0x\2, 0x\3, 0x\4, 0x\5, 0x\6/'

echo "	{ }"
echo "};"
echo
echo "static struct device ""$var_prefix""_dev = {"
echo "	.name  = L\"$(get_field "Manufacturer") $(get_field "Family")\","
echo "	.dtb   = L\"FIXME!\","
echo "	.hwids = ""$var_prefix""_hwids,"
echo "};"
echo "DEVICE_DESC(""$var_prefix""_dev);"

