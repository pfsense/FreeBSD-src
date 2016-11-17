#!/bin/sh
#
# This script generates a "memstick image with mfsroot" (image that can be copied to a
# USB memory stick or PXE booted) from a directory tree.  Note that the script does not
# clean up after itself very well for error conditions on purpose so the
# problem can be diagnosed (full filesystem most likely but ...).
#
# Usage: make-mfsroot-memstick.sh <directory tree> <image filename>
#
# $FreeBSD$
#

PATH=/bin:/usr/bin:/sbin:/usr/sbin
export PATH

if [ $# -ne 2 ]; then
	echo "make-memstick.sh /path/to/directory /path/to/image/file"
	exit 1
fi

if [ ! -d ${1} ]; then
	echo "${1} must be a directory"
	exit 1
fi

if [ -e ${2} ]; then
	echo "won't overwrite ${2}"
	exit 1
fi

if [ -e "${1}.tmp" ]; then
	rm -rf ${1}.tmp
fi

mkdir -p ${1}.tmp

echo '/dev/md0 / ufs ro,noatime 1 1' > ${1}/etc/fstab
echo 'root_rw_mount="NO"' > ${1}/etc/rc.conf.local

cp -rp ${1}/boot ${1}.tmp
makefs -o version=2 ${1}.tmp/mfsroot ${1}
gzip ${1}.tmp/mfsroot

rm ${1}/etc/fstab
rm ${1}/etc/rc.conf.local

echo 'autoboot_delay="3"' >> ${1}.tmp/boot/loader.conf
echo 'mfs_load="YES"' >> ${1}.tmp/boot/loader.conf
echo 'mfs_type="mfs_root"' >> ${1}.tmp/boot/loader.conf
echo 'mfs_name="/mfsroot"' >> ${1}.tmp/boot/loader.conf
echo 'vfs.root.mountfrom="ufs:/dev/md0"' >> ${1}.tmp/boot/loader.conf

makefs -B little -o label=pfSense_Install ${2}.part ${1}.tmp
if [ $? -ne 0 ]; then
	echo "makefs failed"
	exit 1
fi

rm -rf ${1}.tmp

mkimg -s gpt -b ${1}/boot/pmbr -p efi:=${1}/boot/boot1.efifat -p freebsd-boot:=${1}/boot/gptboot -p freebsd-ufs:=${2}.part -p freebsd-swap::1M -o ${2}
rm ${2}.part

