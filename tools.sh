#!/bin/bash
###############################################
#NAME: tools.sh
#FUNCTION: compile uboot Image rootfile system
#DATE: Create by zlh@uteK,2017-08-1
###############################################

export chip=hurricane2
export LOADADDR=0x61008000

# make $CHIP-initramfs_defconfig

cd buildroot

# Building the Default UCLIBC Toolchain
make iproc-tools_defconfig
make

# Build U-Boot Code
make $CHIP-uboot_defconfig
# make hurricane2-uboot_defconfig
make

# Building the Images
make $CHIP-flash_defconfig
# make hurricane2-flash_defconfig
make menuconfig
make

# Convert Image to uImage format for U-Boot
host/usr/bin/mkimage -A arm -O linux -T kernel -n Image -a $LOADADDR -C none -d output/images/Image output/images/uImage_flash.img

