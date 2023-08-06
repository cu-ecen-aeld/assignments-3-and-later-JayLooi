#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

SCRIPT_DIR=$(dirname `readlink -f $0`)
OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}
    
    echo "Deep cleaning..."
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper

    echo "Configuring Kernel build..."
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig

    echo "Building Kernel image..."
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all

    echo "Building modules..."
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules

    echo "Building devicetree..."
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs
fi

echo "Adding the Image in outdir"
cp ${OUTDIR}/linux-stable/arch/arm64/boot/Image ${OUTDIR}/Image

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

echo "Creating necessary base directories..."
mkdir rootfs
cd rootfs
mkdir -p bin etc dev proc sys tmp sbin home lib lib64 usr var
mkdir -p usr/bin usr/sbin usr/lib
mkdir -p var/log

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
else
    cd busybox
fi

echo "Configuring and building busybox..."
make distclean
make defconfig
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

echo "Finding and adding library dependencies to rootfs..."
sysroot=$(${CROSS_COMPILE}gcc -print-sysroot)

lib+=(`${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "program interpreter" | grep -oP "lib/.+\.so.\d+"`)
cp $(find $sysroot -path */$lib) ${OUTDIR}/rootfs/${lib}

libs+=(`${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "Shared library" | grep -oP "(?<=\[).+\.so.\d+(?=\])"`)
for lib in "${libs[@]}"
do
    cp $(find $sysroot -path */$lib) ${OUTDIR}/rootfs/lib64/$(basename ${lib})
done

cd ${OUTDIR}/rootfs

echo "Making device nodes..."
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 666 dev/console c 5 1

echo "Building the writer utility..."
cd $SCRIPT_DIR
make clean
make CROSS_COMPILE=${CROSS_COMPILE}

echo "Copying the finder related scripts and executables to the /home directory on the target rootfs..."
cp ./writer ${OUTDIR}/rootfs/home/writer
cp ./finder.sh ${OUTDIR}/rootfs/home/finder.sh
cp ./writer.sh ${OUTDIR}/rootfs/home/writer.sh
mkdir -p ${OUTDIR}/rootfs/home/conf
cp ./conf/username.txt ${OUTDIR}/rootfs/home/conf/username.txt
cp ./finder-test.sh ${OUTDIR}/rootfs/home/finder-test.sh
cp ./autorun-qemu.sh ${OUTDIR}/rootfs/home/autorun-qemu.sh

# TODO: Chown the root directory
cd ${OUTDIR}/rootfs
chmod +x ${OUTDIR}/rootfs/bin/sh
chmod +x ${OUTDIR}/rootfs/bin/busybox
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio

# TODO: Create initramfs.cpio.gz
gzip -f ${OUTDIR}/initramfs.cpio

