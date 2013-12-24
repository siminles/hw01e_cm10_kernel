#!/bin/bash

# Kernel Build Script For HW-01E
# 2013-12-23 xuefy  : created

# set toolchain
export ARCH=arm
export CROSS_COMPILE=/media/cm/10/prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin/arm-eabi-

# make zImage
mkdir -p ./out/done/modules/
make O=./out hw01e_b101_defconfig
make -j8 O=./out

# Copy Kernel Image and Modules
if [ $? -eq 0 ]
then
	echo "==================================="
	echo "          Compile success!"
	echo "==================================="
	cd out
	cp -f ./arch/arm/boot/zImage ./done/
	cp -r `find -iname '*.ko'` ./done/modules/
	cd -
else
	echo "==================================="
	echo "          Compile failed!"
	echo "==================================="
fi
