#!/bin/sh
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2024 Nikita Travkin <nikita@trvn.ru>

sign_everything() {
	old_dir=$PWD
	cd $1
	sha256sum -- * > sha256sums.txt
	gpg --yes --detach-sign sha256sums.txt
	cd $old_dir
}

TAG=$(git describe)

echo "Generating \'$TAG\'"

BUILD_DIR="$PWD/build-$TAG"
BLOB_DIR="$PWD/release/$TAG"

make -j$(nproc) O="$BUILD_DIR"
mkdir -p "$BLOB_DIR"

cp $BUILD_DIR/*.efi "$BLOB_DIR"

git ls-files --recurse-submodules \
	| tar 	--sort=name --mtime="@0" \
		--owner=0 --group=0 --numeric-owner \
		-caf "$BLOB_DIR/dtbloader-$TAG.tar.gz" -T-

sign_everything "$BLOB_DIR"

echo ==========================================================
echo Done building!
echo Now you can push the tag: "git push origin $TAG"
echo and upload the artifacts from "$BLOB_DIR"
echo ==========================================================

