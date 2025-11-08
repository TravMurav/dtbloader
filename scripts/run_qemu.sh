#!/bin/bash

BASEDIR="$(dirname "$0")/../"
BUILDDIR="$BASEDIR/build-aarch64"

cat << EOF > "$BUILDDIR/startup.nsh"
@echo -off
fs0:
echo =====================================
load dtbloader.efi
echo =====================================
reset -s
EOF

# This is ugly but whatever...
if ! [ -e "$BUILDDIR/efi/boot/bootaa64.efi" ]
then
	mkdir -p "$BUILDDIR/efi/boot/"
	wget https://github.com/pbatard/UEFI-Shell/releases/download/24H2/shellaa64.efi -O "$BUILDDIR/efi/boot/bootaa64.efi"
fi

QEMU_SMBIOS_ARGS=(
	-smbios type=1,manufacturer='LENOVO',product='81F1',sku='LENOVO_MT_81F1_BU_idea_FM_Miix 630',family='Miix 630'
	-smbios type=2,manufacturer='LENOVO',product='LNVNB161216'
)

if [ $# -ge 1 ]
then
	case $1 in
		nosmbios)
			QEMU_SMBIOS_ARGS=()
			;;
		miix)
			# default
			;;
	esac
fi

timeout --foreground \
	--kill-after=120s \
	30s \
	qemu-system-aarch64 \
		-machine virt \
		-cpu cortex-a53 \
		-m 512 \
		-bios "/usr/share/AAVMF/AAVMF_CODE.fd" \
		-drive if=virtio,format=raw,file=fat:rw:"$BUILDDIR" \
		"${QEMU_SMBIOS_ARGS[@]}" \
		-nographic -no-reboot \
			| sed \
				-e 's/\x1b\[[0-9]\+;01H/\n/g' \
				-e 's/\x1b\[[0-9?]\+[^m0-9?]//g'

