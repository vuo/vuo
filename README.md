These instructions are for building Vuo Base, Vuo Compiler, Vuo Renderer, Vuo Runtime, and the built-in Vuo Nodes and Types from source code.

Vuo's source code is available so you can learn about how Vuo works, tinker with it, and maybe even help develop Vuo.

You ***do not*** need to build Vuo from source if you want to:

   - **run the Vuo Editor application**.  The Vuo Editor application is not included in this source code archive — it is available as a [separate download](https://vuo.org/download).
   - **develop an application that uses Vuo**.  Instead, [download the Vuo SDK](https://vuo.org/download), and follow the instructions on [api.vuo.org](http://api.vuo.org) under the section "Developing Applications that use Vuo".
   - **develop nodes and types for Vuo**.  Instead, [download the Vuo SDK](https://vuo.org/download), and follow the instructions on [api.vuo.org](http://api.vuo.org) under the section "Developing Node Classes and Port Types".



# Building Vuo on Mac OS X


## Install required dependencies

### Xcode

Install a recent version of [Xcode](https://developer.apple.com/xcode/) (version 3.2 or later).

Accept the Xcode license by opening Xcode.app or running:

    xcodebuild -license

Install Command Line Tools for Xcode:

   - Launch Xcode
   - Select the Xcode menu > Preferences > Downloads
   - Next to "Command line tools", click "Install"

### Homebrew

Install Homebrew:

    ruby -e "$(curl -fsSL https://raw.github.com/mxcl/homebrew/go/install)"
    brew doctor

Review the results of `brew doctor` and fix any problems it finds.

### LLVM and Clang

If you're running Mac OS 10.8 or earlier:

    export CC=gcc
    export CXX=g++

If you're running Mac OS 10.9:

    brew install https://raw.github.com/Homebrew/homebrew-dupes/ad6f252e3dceb0ca26748816de57c87e0c4630f0/apple-gcc42.rb
    export CC=gcc-4.2
    export CXX=g++-4.2

If you're running any version of Mac OS X:

    cd /tmp
    curl -OL http://llvm.org/releases/3.2/llvm-3.2.src.tar.gz
    curl -OL http://llvm.org/releases/3.2/clang-3.2.src.tar.gz
    tar zxf llvm-3.2.src.tar.gz
    cd llvm-3.2.src/tools
    tar zxf ../../clang-3.2.src.tar.gz
    mv clang-3.2.src clang
    cd ..
    curl -OL https://b33p.net/sites/default/files/llvm-disable-unused-intrinsics_1.patch
    patch -p0 < llvm-disable-unused-intrinsics_1.patch
    CFLAGS="-march=x86-64" CXXFLAGS="-march=x86-64" LDFLAGS="-Wl,-macosx_version_min,10.6" ./configure --prefix=/usr/local/Cellar/llvm/3.2 --enable-optimized --with-optimize-option="-Oz" --disable-bindings --enable-targets=host --enable-shared
    make install -j9
    cd tools/clang
    make install -j9

If building LLVM and Clang for redistribution, also build and execute UPX (see the UPX section below).

### Graphviz

Install Graphviz 2.28.0:

    curl -OL http://www.graphviz.org/pub/graphviz/stable/SOURCES/graphviz-2.28.0.tar.gz
    tar zxf graphviz-2.28.0.tar.gz
    cd graphviz-2.28.0

If you're running Mac OS 10.6:

    export CFLAGS="-mmacosx-version-min=10.6 -mno-sse4 -mno-sse4.1 -mno-sse4.2"

If you're running Mac OS 10.7 or later:

    export CFLAGS="-mmacosx-version-min=10.6 -mno-avx -mno-sse4 -mno-sse4.1 -mno-sse4.2"

If you're running any version of Mac OS X:

    export LDFLAGS='-Wl,-headerpad_max_install_names'
    ./configure --prefix=/usr/local/Cellar/graphviz/2.28.0 --disable-static --enable-shared --disable-debug --disable-dependency-tracking --without-expat --without-x --disable-ltdl --disable-swig --disable-sharp --disable-guile --disable-java --disable-lua --disable-ocaml --disable-perl --disable-php --disable-python --disable-r --disable-ruby --disable-tcl --with-qt=no --with-quartz --disable-swig --without-pangocairo --without-digcola --without-fontconfig --without-freetype2 --without-ortho --without-png --without-jpeg --without-sfdp --without-gdk-pixbuf --without-quartz
    make -j9
    (cd lib ; make install)
    (cd plugin ; make install)

### Qt

Install Qt 5.3.1:

    unset CC CXX
    cd /tmp
    curl -OL http://download.qt-project.org/official_releases/qt/5.3/5.3.1/single/qt-everywhere-opensource-src-5.3.1.tar.gz
    tar xzf qt-everywhere-opensource-src-5.3.1.tar.gz
    cd qt-everywhere-opensource-src-5.3.1
    cd qtbase

    # https://bugreports.qt-project.org/browse/QTBUG-36575
    curl -OL https://bugreports.qt-project.org/secure/attachment/37834/qmake-objcxx-cxxflags.patch
    patch -p1 < qmake-objcxx-cxxflags.patch

    # https://bugreports.qt-project.org/browse/QTBUG-37926
    curl -OL https://b33p.net/sites/default/files/mousebuttonstate.patch
    patch -p1 < mousebuttonstate.patch

    # https://bugreports.qt-project.org/browse/QTBUG-26795
    curl -OL https://bugreports.qt-project.org/secure/attachment/42769/DevicePixelsRatioImageItemCache.diff
    patch -p1 < DevicePixelsRatioImageItemCache.diff

    # https://bugreports.qt-project.org/browse/QTBUG-36383
    curl -OL https://b33p.net/sites/default/files/fixloading2ximagesinrichtextdocuments.patch
    patch -p1 < fixloading2ximagesinrichtextdocuments.patch

    cd ..
    ./configure -prefix /usr/local/Cellar/qt/5.3.1/ -opensource -confirm-license -release -no-c++11 -no-ssse3 -no-sse4.1 -no-sse4.2 -no-avx -no-avx2 -qt-zlib -qt-libpng -qt-libjpeg -qt-pcre -qt-xcb -optimized-qmake -no-xcb -no-eglfs -no-directfb -no-linuxfb -no-kms -no-glib -nomake tools -nomake examples -skip qtquick1 -skip qtquickcontrols -skip qtdeclarative -skip qtscript -skip qtsvg -skip qtxmlpatterns -skip qtwebkit -skip qtmultimedia
    make -j9
    make install
    ln -s /usr/local/Cellar/qt/5.3.1/bin/qmake /usr/local/bin/qmake

### JSON-C

Install JSON-C 0.10:

    cd /tmp
    curl -OL https://github.com/downloads/json-c/json-c/json-c-0.10.tar.gz
    tar zxf json-c-0.10.tar.gz
    cd json-c-0.10
    CFLAGS="-Wno-error" ./configure --prefix=/usr/local/Cellar/json-c/0.10
    make install -j9
    cp json_object_iterator.h /usr/local/Cellar/json-c/0.10/include/json/
    make clean
    CFLAGS="-Wno-error -m32" ./configure --prefix=/usr/local/Cellar/json-c/0.10-32
    make install -j9
    cp json_object_iterator.h /usr/local/Cellar/json-c/0.10-32/include/json/

### ØMQ

If you're running Mac OS 10.9:

    export CC=gcc-4.2
    export CXX=g++-4.2

If you're running any version of Mac OS X:

    cd /tmp
    curl -OL http://download.zeromq.org/zeromq-2.2.0.tar.gz
    tar zxf zeromq-2.2.0.tar.gz
    cd zeromq-2.2.0
    curl -OL https://b33p.net/sites/default/files/zeromq-skip-abort.patch
    patch -p0 < zeromq-skip-abort.patch
    ./configure --prefix=/usr/local/Cellar/zeromq/2.2.0
    make -j9
    make install
    ln -s . /usr/local/Cellar/zeromq/2.2.0/include/zmq
    make clean
    CXXFLAGS="-m32" ./configure --prefix=/usr/local/Cellar/zeromq/2.2.0-32
    make -j9
    make install
    ln -s . /usr/local/Cellar/zeromq/2.2.0-32/include/zmq

### libffi

Install libffi 3.0.11:

    (cd /usr/local && git checkout d1319df Library/Formula/libffi.rb)
    brew install libffi

### zlib

Install zlib 1.2.8: 

    brew install https://raw.github.com/Homebrew/homebrew-dupes/44fc867849f309ff5266e8a0092fac7a1ce81c5e/zlib.rb

### muParser

If you're running Mac OS 10.9:

    export CC=gcc-4.2
    export CXX=g++-4.2

If you're running any version of Mac OS X:

    cd /tmp
    curl -OL http://downloads.sourceforge.net/project/muparser/muparser/Version%202.2.3/muparser_v2_2_3.zip
    unzip muparser_v2_2_3.zip
    cd muparser_v2_2_3
    ./configure --prefix=/usr/local/Cellar/muparser/2.2.3 --enable-shared=no --enable-samples=no
    make -j9
    make install

### FreeImage

    cd /tmp
    curl -OL http://downloads.sourceforge.net/freeimage/FreeImage3154.zip
    unzip FreeImage3154.zip
    cd FreeImage

If you're running Mac OS 10.8 or earlier:

    curl -OL http://sourceforge.net/p/freeimage/bugs/228/attachment/Makefile.osx2
    make -f Makefile.osx2

If you're running Mac OS 10.9:

    curl -OL http://sourceforge.net/p/freeimage/bugs/_discuss/thread/33613606/8561/attachment/Makefile.osx-10.9
    make -f Makefile.osx-10.9

If you're running any version of Mac OS X:

    mkdir -p /usr/local/Cellar/freeimage/3.15.4/{lib,include}
    cp libfreeimage.a-x86_64 /usr/local/Cellar/freeimage/3.15.4/lib/libfreeimage.a
    cp Source/FreeImage.h /usr/local/Cellar/freeimage/3.15.4/include/

### cURL

    cd /tmp
    curl -OL http://curl.haxx.se/download/curl-7.30.0.tar.gz
    tar zxf curl-7.30.0.tar.gz
    cd curl-7.30.0
    ./configure --prefix=/usr/local/Cellar/curl/7.30.0 --enable-shared=no --disable-ldap --without-libidn
    make -j9
    make install

### RtMidi

If you're running Mac OS 10.9:

    export CC=gcc-4.2
    export CXX=g++-4.2

If you're running any version of Mac OS X:

    cd /tmp
    curl -OL http://www.music.mcgill.ca/~gary/rtmidi/release/rtmidi-2.0.1.tar.gz
    tar zxf rtmidi-2.0.1.tar.gz
    cd rtmidi-2.0.1
    ./configure
    make
    mkdir -p /usr/local/Cellar/rtmidi/2.0.1/{lib,include}
    cp librtmidi.a /usr/local/Cellar/rtmidi/2.0.1/lib
    cp *.h /usr/local/Cellar/rtmidi/2.0.1/include

### RtAudio

If you're running Mac OS 10.9:

    export CC=gcc-4.2
    export CXX=g++-4.2

If you're running any version of Mac OS X:

    cd /tmp
    curl -OL http://www.music.mcgill.ca/~gary/rtaudio/release/rtaudio-4.0.12.tar.gz
    tar zxf rtaudio-4.0.12.tar.gz
    cd rtaudio-4.0.12
    ./configure
    make
    mkdir -p /usr/local/Cellar/rtaudio/4.0.12/{lib,include}
    cp librtaudio.a /usr/local/Cellar/rtaudio/4.0.12/lib
    cp *.h /usr/local/Cellar/rtaudio/4.0.12/include

### Gamma

If you're running Mac OS 10.9:

    export CC=gcc-4.2
    export CXX=g++-4.2

If you're running any version of Mac OS X:

    cd /tmp
    curl -OL http://mat.ucsb.edu/gamma/dl/gamma-0.9.5.tar.gz
    mkdir gamma-0.9.5
    cd gamma-0.9.5
    tar zxf ../gamma-0.9.5.tar.gz
    
If you're running Mac OS 10.6, add the following line to Makefile.config in the "Default variables" section:

    EXT_CPPFLAGS = -m64

If you're running any version of Mac OS X:

    make install DESTDIR=/usr/local/Cellar/gamma/0.9.5

### CMake

(Required to build Open Asset Import, below.)

    cd /tmp
    curl -OL http://www.cmake.org/files/v2.8/cmake-2.8.11.2.tar.gz
    tar zxf cmake-2.8.11.2.tar.gz
    cd cmake-2.8.11.2
    ./bootstrap
    make -j9
    make install

### Open Asset Import

If you're running Mac OS 10.9:

    export CC=gcc-4.2
    export CXX=g++-4.2

If you're running any version of Mac OS X:

    cd /tmp
    curl -OL http://downloads.sourceforge.net/project/assimp/assimp-3.0/assimp--3.0.1270-source-only.zip
    tar zxf assimp--3.0.1270-source-only.zip
    cd assimp--3.0.1270-source-only
    curl -OL https://github.com/assimp/assimp/commit/27767031e61629faa0cb9ce4f0d6ded8a5b487f1.patch
    # The following patches will show warnings about whitespace errors and types differing from expected; these are safe to ignore.
    git apply 27767031e61629faa0cb9ce4f0d6ded8a5b487f1.patch
    curl -OL https://github.com/assimp/assimp/commit/8213805dc6556566a9f983e480e44038bdb75674.patch
    git apply 8213805dc6556566a9f983e480e44038bdb75674.patch
    curl -OL https://github.com/assimp/assimp/commit/c338b665ed870363d17a52d2e6e6db276934351f.patch
    git apply c338b665ed870363d17a52d2e6e6db276934351f.patch
    curl -OL https://github.com/assimp/assimp/commit/6551fd4903628bef69f5d7bbd0f0f5b262f32d2e.patch
    git apply 6551fd4903628bef69f5d7bbd0f0f5b262f32d2e.patch
    cmake -DCMAKE_CXX_FLAGS='-Oz' -DENABLE_BOOST_WORKAROUND=ON -DBUILD_STATIC_LIB=ON -DNO_EXPORT=ON -DASSIMP_BUILD_ARCHITECTURE=x86_64 -DBUILD_ASSIMP_TOOLS=OFF -DBUILD_ASSIMP_SAMPLES=OFF -DBUILD_TESTS=OFF
    make -j9
    mkdir -p /usr/local/Cellar/assimp/3.0.1270/{lib,include}
    cp lib/libassimp.a /usr/local/Cellar/assimp/3.0.1270/lib
    cp -R include/assimp/ /usr/local/Cellar/assimp/3.0.1270/include/

### Discount

    cd /tmp
    curl -OL http://www.pell.portland.or.us/~orc/Code/markdown/discount-2.1.6.tar.bz2
    tar jxf discount-2.1.6.tar.bz2 
    cd discount-2.1.6
    ./configure.sh --prefix=/usr/local/Cellar/discount/2.1.6 --enable-all-features
    make
    make install

### yasm

(Required to build FFmpeg, below.)

    (cd /usr/local && git checkout 8f626e6 /usr/local/Library/Formula/yasm.rb)
    brew install yasm

### FFmpeg

    cd /tmp
    curl -OL http://www.ffmpeg.org/releases/ffmpeg-2.1.tar.bz2
    tar jxf ffmpeg-2.1.tar.bz2
    cd ffmpeg-2.1
    ./configure --prefix=/usr/local/Cellar/ffmpeg/2.1 \
        --disable-programs --disable-doc \
        --disable-runtime-cpudetect --disable-ssse3 --disable-sse4 --disable-sse42 --disable-avx \
        --enable-shared --disable-stripping --disable-static --enable-pthreads --enable-yasm --disable-debug \
        --enable-demuxer=mpegts --enable-demuxer=mpegtsraw \
        --disable-bsfs --disable-devices \
        --disable-decoder=aac --disable-decoder=aac_latm --disable-encoder=aac --disable-parser=aac --disable-parser=aac_latm --disable-demuxer=aac \
        --disable-decoder=mp3 --disable-decoder=mp3adu --disable-decoder=mp3adufloat --disable-decoder=mp3float \
        --disable-decoder=mp3on4 --disable-decoder=mp3on4float --disable-demuxer=mp3 --disable-muxer=mp3 \
        --extra-cflags='-arch x86_64' --extra-ldflags='-arch x86_64 -Xlinker -no_function_starts -Xlinker -no_version_load_command' --cc=clang
    make -j9
    make install

### libusb

    cd /tmp
    curl -OL http://sourceforge.net/projects/libusb/files/libusb-1.0/libusb-1.0.9/libusb-1.0.9.tar.bz2
    tar jxf libusb-1.0.9.tar.bz2
    cd libusb-1.0.9
    ./configure LDFLAGS='-framework IOKit -framework CoreFoundation' --prefix=/usr/local/Cellar/libusb/1.0.9
    make -j9
    make install
    ln -s libusb-1.0.0.dylib /usr/local/Cellar/libusb/1.0.9/lib/libusb.dylib

### libfreenect

    cd /tmp
    git clone git://github.com/OpenKinect/libfreenect.git
    cd libfreenect
    git checkout c4505ccacd # v0.2.0
    LDFLAGS='-framework IOKit -framework CoreFoundation -lobjc' cmake -DLIBUSB_1_INCLUDE_DIR='/usr/local/Cellar/libusb/1.0.9/include;/usr/local/Cellar/libusb/1.0.9/include/libusb-1.0' -DLIBUSB_1_LIBRARY=/usr/local/Cellar/libusb/1.0.9/lib/libusb.dylib -DCMAKE_INSTALL_PREFIX:PATH=/usr/local/Cellar/libfreenect/0.2.0 .
    make -j9
    make install

### oscpack

    cd /tmp
    curl -OL https://oscpack.googlecode.com/files/oscpack_1_1_0.zip
    unzip oscpack_1_1_0.zip
    cd oscpack_1_1_0
    # https://code.google.com/p/oscpack/issues/detail?id=15
    curl -OL https://b33p.net/sites/default/files/udpsocket-get-port-1_1_0_0.patch
    patch -p0 < udpsocket-get-port-1_1_0_0.patch
    mkdir -p /usr/local/Cellar/oscpack/1.1.0/{lib,include}
    # On Mac OS 10.9, edit the Makefile: on line 17 ("CXX := g++"), append " -stdlib=libstdc++"
    make install PREFIX=/usr/local/Cellar/oscpack/1.1.0
    ar -r liboscpack.a `find . -name *.o`  # ... if liboscpack.a doesn't already exist
    cp liboscpack.a /usr/local/Cellar/oscpack/1.1.0/lib
    mkdir -p /usr/local/Cellar/oscpack/1.1.0/include/oscpack/{osc,ip}
    cp osc/*.h /usr/local/Cellar/oscpack/1.1.0/include/oscpack/osc
    cp ip/*.h /usr/local/Cellar/oscpack/1.1.0/include/oscpack/ip

### ZXing

    cd /tmp
    curl -OL https://zxing.googlecode.com/files/ZXing-2.3.0.zip
    unzip ZXing-2.3.0.zip
    cd zxing-2.3.0/cpp
    mkdir build
    cd build

If you're running prior to Mac OS 10.9:

    cmake -DCMAKE_BUILD_TYPE=Release ..

If you're running Mac OS 10.9:

    unset CC CXX
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS='-stdlib=libstdc++' ..

If you're running any version of Mac OS X:

    make -j9 libzxing
    mkdir -p /usr/local/Cellar/zxing/2.3.0/{lib,include}
    cp libzxing.a /usr/local/Cellar/zxing/2.3.0/lib
    (cd ../core/src && find . -type f -name \*.h -exec tar cf - {} +) | (cd /usr/local/Cellar/zxing/2.3.0/include && tar xf -)

### libxml2

    cd /tmp
    curl -OL ftp://xmlsoft.org/libxml2/libxml2-sources-2.9.2.tar.gz
    tar zxf libxml2-sources-2.9.2.tar.gz
    cd libxml2-2.9.2
    CFLAGS="-Oz -mmacosx-version-min=10.6" ./configure --prefix=/usr/local/Cellar/libxml2/2.9.2 --with-xpath --with-sax1 --with-threads --disable-shared --enable-ipv6=no --without-debug --without-ftp --without-legacy --without-c14n --without-iconv --without-iso8859x --without-output --without-pattern --without-push --without-reader --without-regexps --without-schemas --without-schematron --without-tree --without-valid --without-writer --without-xinclude --without-modules --without-lzma
    make -j9
    make install

### ld64 133.3

In the instructions below, `$ROOT` is the location of the top-level Vuo source code directory. 

Create a symbolic link to ld64 133.3 in the same directory as Clang (to force Clang to use this linker): 

    ln -s $ROOT/compiler/binary/ld /usr/local/Cellar/llvm/3.2/bin/ld

### OpenSSL

    cd /tmp
    curl -OL http://www.openssl.org/source/openssl-1.0.1g.tar.gz
    tar zxf openssl-1.0.1g.tar.gz
    cd openssl-1.0.1g
    ./Configure --prefix=/usr/local/Cellar/openssl/1.0.1g --openssldir=/usr/local/etc/openssl no-zlib no-shared no-hw no-asm darwin64-x86_64-cc
    make CFLAG="-O0"
    make install

### UPX (only needed if building Vuo or Vuo.framework for distribution)

    cd /tmp
    curl -OL http://www.oberhumer.com/opensource/ucl/download/ucl-1.03.tar.gz
    tar zxf ucl-1.03.tar.gz 
    cd ucl-1.03
    export UPX_UCLDIR=`pwd`
    ./configure
    make -j9
    cd ..
    curl -OL http://downloads.sourceforge.net/project/sevenzip/LZMA%20SDK/4.43/lzma443.tar.bz2
    mkdir lzma443
    cd lzma443
    tar jxf ../lzma443.tar.bz2
    export UPX_LZMADIR=`pwd`
    cd ..
    curl -OL http://upx.sourceforge.net/download/upx-3.91-src.tar.bz2
    tar jxf upx-3.91-src.tar.bz2
    cd upx-3.91-src
    make -j9 all
    mkdir -p /usr/local/Cellar/upx/3.91/bin
    cp src/upx.out /usr/local/Cellar/upx/3.91/bin/upx
    UPX=/usr/local/Cellar/upx/3.91/bin/upx
    $UPX --backup --ultra-brute /usr/local/Cellar/llvm/3.2/bin/clang
    $UPX --backup --ultra-brute /usr/local/Cellar/llvm/3.2/bin/llvm-link


## Reinstall dependencies that are provided with the Vuo source code (optional)

Already-built versions of the dependencies below come with the Vuo source code (in the `compiler/binary` folder). Optionally, you can build them yourself. 

In the instructions below, `$ROOT` is the location of the top-level Vuo source code directory. 

### Csu

Download and unpack [Csu-79](http://www.opensource.apple.com/tarballs/Csu/Csu-79.tar.gz).

    cd Csu-79
    make
    cp crt1.v3.o $ROOT/compiler/binary/crt1.o

### ld64 133.3

Download and unpack ld64's dependencies:
    - [dyld 210.2.3](http://www.opensource.apple.com/tarballs/dyld/dyld-210.2.3.tar.gz)

Download and unpack [ld64 133.3](http://opensource.apple.com/tarballs/ld64/ld64-133.3.tar.gz). 

Edit source files: 

    - `src/ld/Options.cpp`
        - Change `#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070` to `#if 0` in both places.
    - `src/ld/parsers/libunwind/AddressSpace.hpp`
        - Change `#include <mach-o/dyld_priv.h>` to reflect its actual location at `dyld-210.2.3/include/mach-o/dyld_priv.h`.
    - `src/ld/parsers/lto_file.cpp`
        - Change `#include "llvm-c/lto.h"` to reflect its actual location at `/usr/local/Cellar/llvm/3.2/include/llvm-c/lto.h`.

    xcodebuild
    cp ./build/Release-assert/ld $ROOT/compiler/binary/ld
    $UPX --ultra-brute $ROOT/compiler/binary/ld


## Install dependencies for building the RunImageFilter-GLFW API example code (optional)

### GLFW

Install GLFW 2.7.9:

    brew install https://raw.githubusercontent.com/Homebrew/homebrew-versions/ab9fd9a69bc6af2ec6125fae6771aa31111590a7/glfw2.rb


## Install dependencies for building documentation (optional)

### XQuartz

Install [XQuartz](http://xquartz.macosforge.org/).

### MacTeX

Install [MacTeX](http://www.tug.org/mactex/).

### Pandoc

Install [Pandoc](http://code.google.com/p/pandoc/downloads).

### Doxygen

Install Doxygen 1.8.1.2:

    (cd /usr/local && git checkout af32cda Library/Formula/doxygen.rb)
    brew install doxygen


## Install dependencies for contributing to the Vuo source code (optional)

### Subversion

Install Subversion v1.7.x or later (to avoid placing .svn folders in the file system):

    brew install svn

### Build Vuo using Qt Creator

Install [Qt Creator](http://qt-project.org/downloads#qt-creator).

Launch Qt Creator.

Update Qt Creator's Preferences.

   - Go to the "Qt Creator" menu and click Preferences
      - In the left bar, select C++
         - Select the File Naming tab
            - Change Header suffix to "hh"
            - Change Source suffix to "cc"
            - Uncheck the "Lower case file names" checkbox
      - In the left bar, select Build & Run
         - Select the General tab
            - Change Default build directory to "."
         - Select the Qt Versions tab
            - Click Add
            - Press Command-Shift-G and enter `/usr/local/bin/qmake`
            - Click Open
            - Under the Helpers section, expand Details and click Build All
            - On the popup window, click Close
      - Click OK

Set up the Vuo project.

   - Place the Vuo source code in a folder without spaces in its name or in the name of any of its parent folders.
   - Open `vuo.pro`
      - If presented with message "Qt Creator can use the following kits for project vuo: No valid kits found":
            - Click the "options" link within follow-up message "Please add a kit in the options ..."
            - Within the "Build & Run" window that appears:
               - Under the "Name" heading, within the "Manual" option, click "Desktop (default)"
               - Within the "Qt version" menu, select the version of Qt installed per previous instructions.
               - Change the Qt mkspec to "macx-clang".
               - Click OK
            - Within the "Configure Project" page that re-appears:
               -  Check the "Desktop" checkbox to select it as the kit
      - Click the Configure Project button in the bottom-right corner
      - Select the Projects tab in the grey bar on the left
         - Select the Build & Run tab on top
            - Select the Build tab just beneath that
            - Select the "Debug" and "Release" build configurations in turn, and for each one: 
               - Uncheck "Shadow build"
               - In the Build Steps section, expand Details next to the `qmake` build step
                  - In the Additional Arguments textbox, remove `STATIC=1` (if applicable)
               - In the Build Steps section, expand Details next to the `Make` build step
                  - In the Make arguments textbox, add `-j9`
         - Select the Code Style tab on top
            - Change Current Settings to "Qt [built-in]"
            - Click Copy and name the copy "Vuo"
            - Click Edit
               - Under the General tab, set Tab Policy to Tabs Only, and leave the rest as-is
               - Under the "switch" tab, check the "case or default" checkbox, and leave the rest as-is
               - Under the Alignment tab, check the "Align after assignments" checkbox, and leave the rest as-is
               - Click OK
      - Click the Build button (the hammer icon near the bottom of the grey bar on the left)


## Build Vuo from the command line

As an alternative to using Qt Creator (above), you can build from the command line.

Place the Vuo source code in a folder without spaces in its name or in the name of any of its parent folders.

Generate the makefiles:

    qmake -r

Or, to build Vuo and also run the Clang Static Analyzer:

    qmake -r CONFIG+=analyze

Run the makefiles:

    make -j9
    make -j9 vuo32

Optionally, build the Doxygen API documentation and PDF user manual:

    make -j9 docs

Optionally, run the tests:

    make tests

Optionally, build the examples:

    make -j9 examples

You can now run the example compositions from the command line. For example:

    ./node/vuo.scene/examples/AddNoiseToClay

You can now run the command-line tools:

    ./base/vuo-debug/vuo-debug --help
    ./compiler/vuo-compile/vuo-compile --help
    ./compiler/vuo-link/vuo-link --help
    ./renderer/vuo-render/vuo-render --help
    ./renderer/vuo-export/vuo-export --help

For examples of compiling a node class with `vuo-compile`, do:

    cd node && make clean && qmake -r && make

For examples of compiling and linking a composition with `vuo-compile` and `vuo-link`, do:

    cd node/vuo.scene/examples && make clean && qmake && make

For examples of rendering a node class or composition with `vuo-render`, do:

    cd documentation && make clean && qmake -r && make
