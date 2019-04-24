These instructions are for building Vuo Base, Vuo Compiler, Vuo Renderer, Vuo Runtime, and the built-in Vuo Nodes and Types from source code.

Vuo's source code is available so you can learn about how Vuo works, tinker with it, and maybe even help develop Vuo.

You ***do not*** need to build Vuo from source if you want to:

   - **run the Vuo Editor application**.  The Vuo Editor application is not included in this source code archive — it is available as a [separate download](https://vuo.org/download).
   - **develop an application that uses Vuo**, and you've [bought Vuo](https://vuo.org/buy).  Instead, [download the Vuo SDK](https://vuo.org/download), and follow the instructions on [api.vuo.org](http://api.vuo.org) under the section "Developing Applications that use Vuo".
   - **develop nodes and types for Vuo**, and you've [bought Vuo](https://vuo.org/buy).  Instead, [download the Vuo SDK](https://vuo.org/download), and follow the instructions on [api.vuo.org](http://api.vuo.org) under the section "Developing Node Classes and Port Types".



# Building Vuo on macOS


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

If you're running macOS 10.8 or earlier:

    export CC=gcc
    export CXX=g++

If you're running macOS 10.9:

    brew install https://raw.github.com/Homebrew/homebrew-dupes/ad6f252e3dceb0ca26748816de57c87e0c4630f0/apple-gcc42.rb
    export CC=gcc-4.2
    export CXX=g++-4.2

If you're running macOS 10.11 or later:

    brew install homebrew/versions/gcc46
    export CC=gcc-6
    export CXX=g++-6

If you're running any version of macOS:

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
    CFLAGS="-march=x86-64" CXXFLAGS="-march=x86-64" LDFLAGS="-Wl,-macosx_version_min,10.8" ./configure --prefix=/usr/local/Cellar/llvm/3.2 --enable-optimized --with-optimize-option="-O3" --disable-bindings --enable-targets=host --enable-shared
    make install -j9 CFLAGS="-mmacosx-version-min=10.8" CXXFLAGS="-mmacosx-version-min=10.8"
    cd tools/clang
    make install -j9

### Graphviz

Install Graphviz 2.28.0:

    curl -OL http://www.graphviz.org/pub/graphviz/stable/SOURCES/graphviz-2.28.0.tar.gz
    tar zxf graphviz-2.28.0.tar.gz
    cd graphviz-2.28.0
    curl -OL https://b33p.net/sites/default/files/graphviz-skip-dot-layout.patch
    patch -p0 < graphviz-skip-dot-layout.patch
    export CFLAGS="-mmacosx-version-min=10.8 -mno-avx -mno-sse4 -mno-sse4.1 -mno-sse4.2"
    export LDFLAGS='-Wl,-headerpad_max_install_names'
    ./configure --prefix=/usr/local/Cellar/graphviz/2.28.0 --disable-static --enable-shared --disable-debug --disable-dependency-tracking --without-expat --without-x --disable-ltdl --disable-swig --disable-sharp --disable-guile --disable-java --disable-lua --disable-ocaml --disable-perl --disable-php --disable-python --disable-r --disable-ruby --disable-tcl --with-qt=no --with-quartz --disable-swig --without-pangocairo --without-digcola --without-fontconfig --without-freetype2 --without-ortho --without-png --without-jpeg --without-sfdp --without-gdk-pixbuf --without-quartz
    make -j9
    (cd lib ; make install)
    (cd plugin ; make install)

### Qt

Install Qt:

    unset CC CXX
    cd /tmp
    curl -OL http://download.qt.io/official_releases/qt/5.6/5.6.2/single/qt-everywhere-opensource-src-5.6.2.tar.gz
    tar xzf qt-everywhere-opensource-src-5.6.2.tar.gz
    cd qt-everywhere-opensource-src-5.6.2
    cd qtbase

    # https://bugreports.qt.io/browse/QTBUG-26795
    curl -OL https://bugreports.qt.io/secure/attachment/42769/DevicePixelsRatioImageItemCache.diff
    patch -p1 < DevicePixelsRatioImageItemCache.diff

    # https://bugreports.qt.io/browse/QTBUG-44620
    # https://b33p.net/kosada/node/11273
    curl -OL https://b33p.net/sites/default/files/qt-colorpanel-size_1.patch
    patch -p1 < qt-colorpanel-size_1.patch

    # https://bugreports.qt.io/browse/QTBUG-31406
    # https://b33p.net/kosada/node/6228
    # https://vuo.org/node/111
    curl -OL https://b33p.net/sites/default/files/qt-colorpanel-position_0.patch
    patch -p1 < qt-colorpanel-position_0.patch

    # https://bugreports.qt.io/browse/QTBUG-46351
    # https://b33p.net/kosada/node/12358
    curl -OL https://bugreports.qt.io/secure/attachment/58221/stop-stealing-focus-on-touchbegin.patch
    patch -p0 < stop-stealing-focus-on-touchbegin.patch

    cd ..
    ./configure -prefix /usr/local/Cellar/qt/5.6.2/ -opensource -confirm-license -release -no-c++11 -no-ssse3 -no-sse4.1 -no-sse4.2 -no-avx -no-avx2 -no-qml-debug -qt-zlib -qt-libpng -qt-libjpeg -qt-pcre -no-xcb -no-eglfs -no-directfb -no-linuxfb -no-kms -no-glib -strip -no-dbus -nomake tools -nomake examples -no-sql-mysql -no-sql-sqlite -skip 3d -skip activeqt -skip androidextras -skip canvas3d -skip connectivity -skip declarative -skip doc -skip enginio -skip graphicaleffects -skip imageformats -skip location -skip multimedia -skip quickcontrols -skip quickcontrols2 -skip script -skip sensors -skip serialbus -skip serialport -skip svg -skip tools -skip translations -skip wayland -skip webchannel -skip webengine -skip websockets -skip webview -skip winextras -skip x11extras -skip xmlpatterns -D QT_NO_GESTURES
    make -j9
    make install
    ln -s /usr/local/Cellar/qt/5.6.2/bin/qmake /usr/local/bin/qmake

### JSON-C

Install JSON-C:

    cd /tmp
    curl -OL https://github.com/json-c/json-c/archive/json-c-0.12-20140410.tar.gz
    tar zxf json-c-0.12-20140410.tar.gz
    cd json-c-json-c-0.12-20140410
    CFLAGS="-mmacosx-version-min=10.8 -Wno-error" ./configure --prefix=/usr/local/Cellar/json-c/0.12
    make install -j9
    make clean
    CFLAGS="-mmacosx-version-min=10.8 -Wno-error -m32" ./configure --prefix=/usr/local/Cellar/json-c/0.12-32
    make install -j9

### ØMQ

If you're running macOS 10.9:

    export CC=gcc-4.2
    export CXX=g++-4.2

If you're running any version of macOS:

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

    (cd /usr/local && git init && git checkout d1319df Library/Formula/libffi.rb)
    brew install libffi

### zlib

Install zlib 1.2.8: 

    cd /tmp
    curl -OL http://zlib.net/zlib-1.2.8.tar.gz
    tar zxf zlib-1.2.8.tar.gz
    cd zlib-1.2.8
    CFLAGS="-Oz" ./configure --prefix=/usr/local/Cellar/zlib/1.2.8 --static --archs="-arch x86_64" --64
    make -j9
    make install

### muParser

If you're running macOS 10.9:

    export CC=gcc-4.2
    export CXX=g++-4.2

If you're running any version of macOS:

    cd /tmp
    curl -OL http://downloads.sourceforge.net/project/muparser/muparser/Version%202.2.3/muparser_v2_2_3.zip
    unzip muparser_v2_2_3.zip
    cd muparser_v2_2_3
    ./configure --prefix=/usr/local/Cellar/muparser/2.2.3 --enable-shared=no --enable-samples=no
    make -j9
    make install

### FreeImage

    cd /tmp
    curl -OL http://downloads.sourceforge.net/freeimage/FreeImage3170.zip
    unzip FreeImage3170.zip
    mv FreeImage FreeImage3170
    rm -Rf FreeImage3170/Source/LibJPEG
    curl -OL http://downloads.sourceforge.net/project/libjpeg-turbo/1.4.2/libjpeg-turbo-1.4.2.tar.gz
    tar zxf libjpeg-turbo-1.4.2.tar.gz
    mv libjpeg-turbo-1.4.2 FreeImage3170/Source/LibJPEG
    cd FreeImage3170/Source/LibJPEG
    NASM=/usr/local/Cellar/yasm/1.2.0/bin/yasm ./configure
    make -j9
    # Make will output some "ignoring" warnings you can ignore.
    cd ../..
    curl -OL https://b33p.net/sites/default/files/freeimage3170-makefile_0.patch
    patch -p1 < freeimage3170-makefile_0.patch
    bash gensrclist.sh
    make -j9
    # Make will output some warnings you can ignore.
    mkdir -p /usr/local/Cellar/freeimage/3.17.0/{lib,include}
    cp libfreeimage-3.17.0.dylib-x86_64 /usr/local/Cellar/freeimage/3.17.0/lib/libfreeimage.dylib
    cp Source/FreeImage.h /usr/local/Cellar/freeimage/3.17.0/include/

### cURL

    cd /tmp
    curl -OL http://curl.haxx.se/download/curl-7.30.0.tar.gz
    tar zxf curl-7.30.0.tar.gz
    cd curl-7.30.0
    ./configure --prefix=/usr/local/Cellar/curl/7.30.0 --enable-shared=no --disable-ldap --without-libidn
    make -j9
    make install

### RtMidi

If you're running macOS 10.9:

    export CC=gcc-4.2
    export CXX=g++-4.2

If you're running any version of macOS:

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

    export CC=/usr/local/Cellar/llvm/3.2/bin/clang
    export CXX=/usr/local/Cellar/llvm/3.2/bin/clang++
    cd /tmp
    curl -OL http://www.music.mcgill.ca/~gary/rtaudio/release/rtaudio-4.1.2.tar.gz
    tar zxf rtaudio-4.1.2.tar.gz
    cd rtaudio-4.1.2
    curl -OL https://b33p.net/sites/default/files/rtaudio-modeluid_0.patch
    patch < rtaudio-modeluid_0.patch
    CXXFLAGS="-Oz -mmacosx-version-min=10.8 -DUNICODE" ./configure --prefix=/usr/local/Cellar/rtaudio/4.1.2 --disable-shared
    make -j9
    make install

### Gamma

If you're running macOS 10.9:

    export CC=gcc-4.2
    export CXX=g++-4.2

If you're running any version of macOS:

    cd /tmp
    curl -OL http://mat.ucsb.edu/gamma/dl/gamma-0.9.5.tar.gz
    mkdir gamma-0.9.5
    cd gamma-0.9.5
    tar zxf ../gamma-0.9.5.tar.gz
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

If you're running any version of macOS:

    cd /tmp
    curl -L https://github.com/assimp/assimp/archive/v3.2.tar.gz -o assimp-3.2.0.tar.gz
    tar zxf assimp-3.2.0.tar.gz
    mv assimp-3.2 assimp-3.2.0
    cd assimp-3.2.0
    # https://b33p.net/kosada/node/13345
    # https://github.com/assimp/assimp/pull/1264
    curl -OL https://patch-diff.githubusercontent.com/raw/assimp/assimp/pull/1264.patch
    patch -p1 < 1264.patch

If you're running macOS 10.9 or 10.10 or 10.11:

    cmake -DCMAKE_C_COMPILER='/usr/local/Cellar/llvm/3.2/bin/clang' -DCMAKE_CXX_COMPILER='/usr/local/Cellar/llvm/3.2/bin/clang++' -DCMAKE_CXX_FLAGS='-Oz -DNDEBUG' -DCMAKE_COMPILER_IS_GNUCC=ON -DASSIMP_ENABLE_BOOST_WORKAROUND=ON -DASSIMP_BUILD_STATIC_LIB=OFF -DBUILD_SHARED_LIBS=ON -DASSIMP_NO_EXPORT=ON -DCMAKE_OSX_ARCHITECTURES=x86_64 -DASSIMP_BUILD_ASSIMP_TOOLS=OFF -DASSIMP_BUILD_SAMPLES=OFF -DASSIMP_BUILD_TESTS=OFF
    # You may see a warning about `MACOSX_RPATH`; this is safe to ignore.

If you're running macOS 10.8:

    cmake -DCMAKE_CXX_FLAGS='-Oz -DNDEBUG' -DCMAKE_COMPILER_IS_GNUCC=ON -DASSIMP_ENABLE_BOOST_WORKAROUND=ON -DASSIMP_BUILD_STATIC_LIB=OFF -DBUILD_SHARED_LIBS=ON -DASSIMP_NO_EXPORT=ON -DCMAKE_OSX_ARCHITECTURES=x86_64 -DASSIMP_BUILD_ASSIMP_TOOLS=OFF -DASSIMP_BUILD_SAMPLES=OFF -DASSIMP_BUILD_TESTS=OFF

If you're running any version of macOS:

    make -j9
    mkdir -p /usr/local/Cellar/assimp/3.2.0/{lib,include}
    cp lib/libassimp.3.2.0.dylib /usr/local/Cellar/assimp/3.2.0/lib/libassimp.dylib
    cp -R include/assimp/ /usr/local/Cellar/assimp/3.2.0/include/
    ln -s /usr/local/Cellar/assimp/3.2.0/include /usr/local/Cellar/assimp/3.2.0/include/assimp

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
        --enable-openssl \
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
    curl -OL https://sourceforge.net/projects/libusb/files/libusb-1.0/libusb-1.0.20/libusb-1.0.20.tar.bz2
    tar jxf libusb-1.0.20.tar.bz2
    cd libusb-1.0.20
    CFLAGS="-Oz -mmacosx-version-min=10.8" ./configure --prefix=/usr/local/Cellar/libusb/1.0.20
    make
    make install
    ln -s libusb-1.0.0.dylib /usr/local/Cellar/libusb/1.0.20/lib/libusb.dylib

### libfreenect

    cd /tmp
    curl -L https://github.com/OpenKinect/libfreenect/archive/v0.5.3.tar.gz -o libfreenect-0.5.3.tar.gz
    tar zxf libfreenect-0.5.3.tar.gz
    cd libfreenect-0.5.3
    cmake -DLIBUSB_1_INCLUDE_DIR='/usr/local/Cellar/libusb/1.0.20/include;/usr/local/Cellar/libusb/1.0.20/include/libusb-1.0' -DLIBUSB_1_LIBRARY=/usr/local/Cellar/libusb/1.0.20/lib/libusb.dylib -DCMAKE_INSTALL_PREFIX:PATH=/usr/local/Cellar/libfreenect/0.5.3 .
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
    # On macOS 10.9, edit the Makefile: on line 17 ("CXX := g++"), append " -stdlib=libstdc++"
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

If you're running prior to macOS 10.9:

    cmake -DCMAKE_BUILD_TYPE=Release ..

If you're running macOS 10.9:

    unset CC CXX
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS='-stdlib=libstdc++' ..

If you're running any version of macOS:

    make -j9 libzxing
    mkdir -p /usr/local/Cellar/zxing/2.3.0/{lib,include}
    cp libzxing.a /usr/local/Cellar/zxing/2.3.0/lib
    (cd ../core/src && find . -type f -name \*.h -exec tar cf - {} +) | (cd /usr/local/Cellar/zxing/2.3.0/include && tar xf -)

### libxml2

    cd /tmp
    curl -OL ftp://xmlsoft.org/libxml2/libxml2-sources-2.9.2.tar.gz
    tar zxf libxml2-sources-2.9.2.tar.gz
    cd libxml2-2.9.2
    CFLAGS="-Oz -mmacosx-version-min=10.8" ./configure --prefix=/usr/local/Cellar/libxml2/2.9.2 --with-xpath --with-sax1 --with-threads --disable-shared --enable-ipv6=no --without-debug --without-ftp --without-legacy --without-c14n --without-iconv --without-iso8859x --without-pattern --without-push --without-python --without-reader --without-regexps --without-schemas --without-schematron --without-valid --without-writer --without-xinclude --without-modules --without-lzma
    make -j9
    make install

### libcsv

    cd /tmp
    curl -OL https://sourceforge.net/projects/libcsv/files/libcsv/libcsv-3.0.3/libcsv-3.0.3.tar.gz
    tar zxf libcsv-3.0.3.tar.gz
    cd libcsv-3.0.3
    CFLAGS="-Oz -mmacosx-version-min=10.8" ./configure --prefix=/usr/local/Cellar/libcsv/3.0.3 --disable-static
    make -j9
    make install

### gettext

(Required for glib, below.)

    cd /tmp
    curl -OL http://ftp.gnu.org/pub/gnu/gettext/gettext-0.19.8.1.tar.xz
    brew install xz
    tar xf gettext-0.19.8.1.tar.xz
    cd gettext-0.19.8.1
    CFLAGS="-Oz -mmacosx-version-min=10.8" ./configure --prefix=/usr/local/Cellar/gettext/0.19.8.1 --disable-java --disable-static --enable-shared --disable-c++ --disable-curses
    make -j9
    make install

### glib

(Required for liblqr, below.)

    cd /tmp
    curl -OL https://download.gnome.org/core/3.23/3.23.4/sources/glib-2.51.1.tar.xz
    tar xf glib-2.51.1.tar.xz
    cd glib-2.51.1
    PATH="/usr/local/Cellar/gettext/0.19.8.1/bin:$PATH" CFLAGS="-Oz -mmacosx-version-min=10.8" CPPFLAGS="-I/usr/local/Cellar/gettext/0.19.8.1/include" LIBFFI_CFLAGS="-I/usr/local/Cellar/libffi/3.0.11/lib/libffi-3.0.11/include" LIBFFI_LIBS="/usr/local/Cellar/libffi/3.0.11/lib/libffi.a" LDFLAGS="/usr/local/Cellar/gettext/0.19.8.1/lib/libintl.dylib" ./configure --prefix=/usr/local/Cellar/glib/2.51.1 --without-pcre
    make -j9
    make install

### liblqr

    cd /tmp
    curl -OL https://github.com/carlobaldassi/liblqr/archive/v0.4.2.tar.gz
    tar zxf v0.4.2.tar.gz
    cd liblqr-0.4.2
    CFLAGS="-Oz -mmacosx-version-min=10.8 -I/usr/local/Cellar/glib/2.51.1/include/glib-2.0 -I/usr/local/Cellar/glib/2.51.1/lib/glib-2.0/include" GLIB_CFLAGS="-I/usr/local/Cellar/glib/2.51.1/include/glib-2.0" GLIB_LIBS="/usr/local/Cellar/glib/2.51.1/lib/libglib-2.0.dylib" ./configure --prefix=/usr/local/Cellar/liblqr/0.4.2
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

### WJElement (only needed for TestModules)

    curl -OL https://github.com/netmail-open/wjelement/archive/v1.2.tar.gz
    tar zxf v1.2.tar.gz
    cd wjelement-1.2
    mkdir build
    cd build
    cmake .. -DCMAKE_INSTALL_PREFIX='/usr/local/Cellar/wjelement/1.2'
    make -j9
    make install


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

Install [Pandoc](https://github.com/jgm/pandoc/releases/).

### Doxygen

Install Doxygen:

    brew install doxygen

### Docbook

    brew install docbook
    brew install docbook-xsl

### Ghostscript 9.15

    brew install https://raw.githubusercontent.com/Homebrew/homebrew/f8eb7fa8b98ba511b0136bd1f498f23c1c425499/Library/Formula/ghostscript.rb

### pngquant 2.3.1

    brew install https://raw.githubusercontent.com/Homebrew/homebrew/83bea4d30b76a0ba2e5079b72d0e0ef259cd5e81/Library/Formula/pngquant.rb


## Install dependencies for contributing to the Vuo source code (optional)

### Subversion

Install Subversion v1.7.x or later (to avoid placing .svn folders in the file system):

    brew install svn

### Build Vuo using Qt Creator

Install [Qt Creator](http://qt.io/downloads#qt-creator).

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
      - Select the Projects tab in the gray bar on the left
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
      - Click the Build button (the hammer icon near the bottom of the gray bar on the left)


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
