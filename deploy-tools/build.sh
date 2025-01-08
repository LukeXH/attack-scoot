CURRDIR=$(dirname $(readlink -f "$0"))

echo $CURRDIR

cd $CURRDIR/..
mkdir -p build
cd build
cmake ..
cmake --build .
