#!/bin/sh

BASEDIR="$(dirname "$0")/../"
BUILDDIR="$BASEDIR/build-aarch64"

cat << EOF > "$BUILDDIR/startup.nsh"
@echo -off
fs0:
echo =====================================
load dtbloader.efi
echo =====================================
dmpstore -guid 96176C01-5F3F-47C9-9F85-5632F602F611 DtbName
echo _SCRIPT_DONE_
reset -s
EOF

# This is ugly but whatever...
if ! [ -e "$BUILDDIR/efi/boot/bootaa64.efi" ]
then
	mkdir -p "$BUILDDIR/efi/boot/"
	wget https://github.com/pbatard/UEFI-Shell/releases/download/24H2/shellaa64.efi -O "$BUILDDIR/efi/boot/bootaa64.efi"
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
		-smbios type=1,manufacturer="LENOVO",product="81F1",sku="LENOVO_MT_81F1_BU_idea_FM_Miix 630",family="Miix 630" \
		-smbios type=2,manufacturer="LENOVO",product="LNVNB161216" \
		-nographic -no-reboot \
			| sed \
				-e 's/\x1b\[[0-9]\+;01H/\n/g' \
				-e 's/\x1b\[[0-9?]\+[^m0-9?]//g'

