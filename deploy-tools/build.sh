PKG_CONFIG_PATH=/usr/lib/arm-linux-gnueabihf/pkgconfig

CURRDIR=$(dirname $(readlink -f "$0"))

echo $CURRDIR

cd $CURRDIR/..
meson build
cd build
ninja
# mkdir -p build
# cd build
# cmake ..
# cmake --build .
