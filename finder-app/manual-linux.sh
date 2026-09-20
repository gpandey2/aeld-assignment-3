#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git
KERNEL_VERSION=v5.15.163
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-
SYSROOT=$(${CROSS_COMPILE}gcc --print-sysroot)
OLD_DIR=$PWD

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux/arch/${ARCH}/boot/Image ]; then
    cd linux
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # Deep clean, including the .config
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper

    # Generate the defconfig for QEMU target
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig

    # Generate the vmlinux image for QEMU
    make -j8 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all

    # Build the modules
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules

    # Build the devicetree
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs
fi

# Copy the linux kernel image to outdir 
echo "Adding the Image in outdir"
cp ${OUTDIR}/linux/arch/${ARCH}/boot/Image ${OUTDIR}/ 

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# Create necessary directories
mkdir ${OUTDIR}/rootfs
cd ${OUTDIR}/rootfs
mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
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

# Configure busybox
make distclean
make defconfig
# Make and install busybox
make -j8 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

echo "Library dependencies"
INTERPRETER=$(${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "program interpreter" | sed -n 's/.*Requesting program interpreter: \(.*\)\].*/\1/p')
LIBS=$(${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "Shared library" | awk -F'[][]' '{print $2}')

# Add Interpreter to rootfs
SRC=$(find "$SYSROOT" -name $(basename ${INTERPRETER}) -type f -print -quit)
if [ -z "$SRC" ]; then
    echo "ERROR: ${INTERPRETER} not found"
    exit 1
fi
cp -av "$SRC" "${OUTDIR}/rootfs/lib/"

# Add library dependencies to rootfs
for lib in $LIBS; do
    SRC=$(find "$SYSROOT" -name "$lib" -type f -print -quit)

    if [ -z "$SRC" ]; then
        echo "ERROR: $lib not found"
        exit 1
    fi
    cp -av "$SRC" "${OUTDIR}/rootfs/lib64/"
done
  

# Make device nodes
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/null c 1 3
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/character c 5 1

# Clean and build the writer utility
cd ${OLD_DIR}
make clean
make CROSS_COMPILE=${CROSS_COMPILE}

# Copy the finder related scripts and executables to the /home directory
# on the target rootfs
cp -r writer ${OUTDIR}/rootfs/home/
cp -r ../conf/ ${OUTDIR}/rootfs/home/
cp finder.sh ${OUTDIR}/rootfs/home/
cp finder-test.sh ${OUTDIR}/rootfs/home/
cp autorun-qemu.sh ${OUTDIR}/rootfs/home/

# Modify the finder-test.sh script to reference conf/assignment.txt instead of ../conf/assignment.txt
sed -i 's#assignment=`cat ../conf/assignment.txt`#assignment=`cat conf/assignment.txt`#' ${OUTDIR}/rootfs/home/finder-test.sh

# Fix the test scripts by removing the make lines and fixing the shebang
sed -i '1s|^#!.*|#!/bin/sh|' ${OUTDIR}/rootfs/home/finder.sh ${OUTDIR}/rootfs/home/finder-test.sh
sed -i '/make/d' ${OUTDIR}/rootfs/home/finder-test.sh

# Chown the root directory
sudo chown 777 ${OUTDIR}/rootfs/home/ -R

# Create initramfs.cpio.gz
cd ${OUTDIR}/rootfs/
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
gzip -f ${OUTDIR}/initramfs.cpio
