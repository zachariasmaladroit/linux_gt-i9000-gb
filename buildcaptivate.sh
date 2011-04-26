#Script to build for the captivate and zip the package.
make aries_captivate_defconfig
make -j`grep 'processor' /proc/cpuinfo | wc -l`
cp /arch/arm/boot/zImage releasetools/updates
cd releasetools
zip * Talon
echo "bam. done."
