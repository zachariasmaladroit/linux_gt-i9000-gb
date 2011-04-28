# Script to build for the captivate and zip the package.
# Written by Evan alias ytt3r
make aries_talon_defconfig
export LOCALVERSION="-I9000XWJVB-CL118186"
make -j`grep 'processor' /proc/cpuinfo | wc -l`
cp arch/arm/boot/zImage releasetools
cd releasetools
zip * Talon
echo "Finished."
