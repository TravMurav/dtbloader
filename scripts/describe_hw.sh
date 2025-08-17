#!/bin/sh
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2024 Nikita Travkin <nikita@trvn.ru>

dtb_name="FIXME!"
dtb_compatible="FIXME!"
hwids_file=""
name_field="Family"

if [ -e /sys/firmware/devicetree/base/compatible ]
then
	dtb_compatible="$(cat /sys/firmware/devicetree/base/compatible | tr '\0' '\n' | head -n1)"
fi

usage() {
	echo "Usage: $0 [-h] [-d DTB_NAME] [-f FILE] [-n NAME]"
	echo "Generate hardware description struct for dtbloader."
	echo
	echo "  -d DTB	Set the name of the DTB to use."
	echo "  -c COMP	Set the compatible value to use."
	echo "  -f FILE	Read FILE as fwupdtool hwids output."
	echo "  -n NAME	Field to use as device name. See below. (default: $name_field)"
	echo "  -h		This help."
	echo
	echo "Possible NAME fields are: 'Family', 'ProductName', 'BaseboardProduct'."
	echo "By default this tool attempts to survey the device it was run on."
	echo
}

while getopts ":d:c:f:n:h" opt
do
	case $opt in
		d)
			dtb_name="$OPTARG"
			;;
		c)
			dtb_compatible="$OPTARG"
			;;
		f)
			hwids_file="$OPTARG"
			;;
		n)
			name_field="$OPTARG"
			;;
		h)
			usage
			exit 0 ;;
		*)
			usage
			echo "Unkown option: -$OPTARG"
			echo
			exit 1 ;;
	esac
done
shift $((OPTIND-1))

if [ -z "$hwids_file" ]
then
	hwids_data="$(fwupdtool hwids)"
else
	hwids_data="$(cat "$hwids_file")"
fi

get_field() {
	name="$1"
	echo "$hwids_data" | grep -m1 "$name" | sed 's/.*: //'
}

var_prefix="$(echo $(get_field "Manufacturer") $(get_field "$name_field") | tr '[:upper:]' '[:lower:]' | sed 's/\(\W\|-\)/_/g')"

echo "// SPDX-License-Identifier: BSD-3-Clause"
echo
echo "#include <efi.h>"
echo "#include <device.h>"
echo
echo "static EFI_GUID ""$var_prefix""_hwids[] = {"

# If it's stupid but it works...
echo "$hwids_data" \
	| grep -E "^{" \
	| sed \
		-e 's/^/\t/' \
		-e 's/   <-.*//' \
		-e 's/-/, 0x/g' \
		-e 's/{\(\w*\), \(.*\), 0x\(..\)\(..\), 0x\(.*\)}/{ 0x\1, \2, { 0x\3, 0x\4, _\5_ } },/' \
		-e 's/_\(..\)\(..\)\(..\)\(..\)\(..\)\(..\)_/0x\1, 0x\2, 0x\3, 0x\4, 0x\5, 0x\6/' | grep -v unknown

echo "	{ }"
echo "};"
echo
echo "static struct device ""$var_prefix""_dev = {"
echo "	.name  = L\"$(get_field "Manufacturer") $(get_field "$name_field")\","
echo "	.dtb   = L\"$(echo "$dtb_name" | sed 's_[/\\]_\\\\_')\","
echo "	.compatible = \"$dtb_compatible\","
echo "	.hwids = ""$var_prefix""_hwids,"
echo "};"
echo "DEVICE_DESC(""$var_prefix""_dev);"

