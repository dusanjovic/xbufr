#!/bin/bash
set -eux

SECONDS=0

date

OS=$( uname -s )

VER6=6.1.0
VER6_DIR=${VER6%.*}

if [[ $OS == MSYS* || $OS == MINGW* ]]; then
  ext='zip'
else
  ext='tar.xz'
fi

curl -L    https://download.qt.io/official_releases/qt/${VER6_DIR}/${VER6}/submodules/md5sums.txt | grep qtbase-everywhere-src-${VER6}.${ext} > check.md5
curl -L -O https://download.qt.io/official_releases/qt/${VER6_DIR}/${VER6}/submodules/qtbase-everywhere-src-${VER6}.${ext}

md5sum -c check.md5

MYDIR=$(pwd)

INSTALL_DIR=${MYDIR}/qt-${VER6}
rm -rf "${INSTALL_DIR}"

cd "${MYDIR}"

QTSRC=qtbase-everywhere-src-${VER6}
rm -rf "${QTSRC}"
if [[ $OS == MSYS* || $OS == MINGW* ]]; then
  unzip -q "${QTSRC}".${ext}
else
  tar Jxf "${QTSRC}".${ext}
fi

rm -rf build
mkdir build
cd build

cmake ${MYDIR}/${QTSRC} -G Ninja \
                        -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} \
                        -DBUILD_SHARED_LIBS=OFF \
                        -DFEATURE_system_harfbuzz=OFF \
                        -DFEATURE_system_zlib=OFF \
                        -DFEATURE_dbus=OFF \
                        -DQT_BUILD_TESTS=OFF -DQT_BUILD_EXAMPLES=OFF

cmake --build .
cmake --install .

date
elapsed_time=$( printf '%02dh:%02dm:%02ds\n' $((SECONDS%86400/3600)) $((SECONDS%3600/60)) $((SECONDS%60)) )
echo "Elapsed time: ${elapsed_time}. Have a nice day!"
echo "End OK"
