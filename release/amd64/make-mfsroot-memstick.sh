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

echo '/dev/md0 / ufs ro,noatime 1 1' > ${1}/etc/fstab
echo 'root_rw_mount="NO"' > ${1}/etc/rc.conf.local

tar -C ${1} --exclude './mfsroot' -cf ${1}/mfsroot .

for f in $(find ${1} -depth 1 \! -name boot \! -name mfsroot); do
	chflags -R noschg ${f}
	rm -rf ${f}
done

gzip ${1}/mfsroot

echo 'autoboot_delay="3"' >> ${1}/boot/loader.conf
echo 'mfs_load="YES"' >> ${1}/boot/loader.conf
echo 'mfs_type="mfs_root"' >> ${1}/boot/loader.conf
echo 'mfs_name="/mfsroot"' >> ${1}/boot/loader.conf
echo 'vfs.root.mountfrom="ufs:/dev/md0"' >> ${1}/boot/loader.conf

makefs -B little -o label=pfSense_Install ${2}.part ${1}
if [ $? -ne 0 ]; then
	echo "makefs failed"
	exit 1
fi

mkimg -s gpt -b ${1}/boot/pmbr -p efi:=${1}/boot/boot1.efifat -p freebsd-boot:=${1}/boot/gptboot -p freebsd-ufs:=${2}.part -p freebsd-swap::1M -o ${2}
rm ${2}.part

