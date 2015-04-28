These instructions are for building Vuo Base, Vuo Compiler, Vuo Renderer, Vuo Runtime, and the built-in Vuo Nodes and Types from source code.

Vuo's source code is available so you can learn about how Vuo works, tinker with it, and maybe even help develop Vuo.

You ***do not*** need to build Vuo from source if you:

   - want to run the Vuo Editor application.  (The Vuo Editor application is not included in this source code archive — it is available as a [separate download](https://vuo.org/subscriber).)
   - want to develop an application that uses Vuo.  (Instead, [download Vuo.framework](https://vuo.org/subscriber), and follow the instructions on [api.vuo.org](http://api.vuo.org) under the section "Developing Applications that use Vuo".)
   - want to develop nodes and types for Vuo.  (@todo / instructions forthcoming.)



# Building Vuo on Mac OS X


## Install required dependencies

### Xcode

Install a recent version of [Xcode](https://developer.apple.com/xcode/) (3.2 or 4).

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

    brew install apple-gcc42
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
    CFLAGS="-march=x86-64" CXXFLAGS="-march=x86-64" LDFLAGS="-Wl,-macosx_version_min,10.6" ./configure --prefix=/usr/local/Cellar/llvm/3.2 --enable-optimized --with-optimize-option="-Oz" --disable-bindings --enable-targets=host
    make install -j9
    cd tools/clang
    make install -j9

If building LLVM and Clang for redistribution, also build and execute UPX (see the UPX section below).

### Remove libtool

(If libtool is installed, graphviz uses it, adding an unnecessary dependency.)

    brew remove libtool

### Graphviz

Install Graphviz 2.28.0:

    (cd /usr/local && git checkout ac18935 Library/Formula/graphviz.rb)
    brew edit graphviz
        # Add `args << "--without-expat"`
        # Add `args << "--without-x"`
    brew install graphviz --env=std --build-from-source

### Qt

Install Qt 5.2.1:

    cd /tmp
    curl -OL http://download.qt-project.org/official_releases/qt/5.2/5.2.1/single/qt-everywhere-opensource-src-5.2.1.tar.gz
    tar xzf qt-everywhere-opensource-src-5.2.1.tar.gz
    cd qt-everywhere-opensource-src-5.2.1
    cd qtbase
    # https://bugreports.qt-project.org/browse/QTBUG-36575
    curl -OL https://bugreports.qt-project.org/secure/attachment/37834/qmake-objcxx-cxxflags.patch
    patch -p1 < qmake-objcxx-cxxflags.patch
    # https://bugreports.qt-project.org/browse/QTBUG-33961
    # https://b33p.net/kosada/node/7718#comment-27777
    curl -OL https://b33p.net/sites/default/files/cursorfixfrom5.2.1v2_0.patch
    patch -p1 < cursorfixfrom5.2.1v2_0.patch
    # https://bugreports.qt-project.org/browse/QTBUG-34534
    # https://b33p.net/kosada/node/6477
    curl -OL https://b33p.net/sites/default/files/qtbug-34534-rubberbandtrails.patch
    patch -p0 < qtbug-34534-rubberbandtrails.patch
    cd ..
    ./configure -prefix /usr/local/Cellar/qt/5.2.1/ -opensource -confirm-license -release -no-c++11 -no-ssse3 -no-sse4.1 -no-sse4.2 -no-avx -no-avx2 -qt-zlib -qt-libpng -qt-libjpeg -qt-pcre -qt-xcb -optimized-qmake -no-xcb -no-eglfs -no-directfb -no-linuxfb -no-kms -no-glib -nomake tools -nomake examples -skip qtquick1 -skip qtquickcontrols -skip qtdeclarative -skip qtscript -skip qtsvg -skip qtxmlpatterns -skip qtwebkit
    make -j9
    make install
    ln -s /usr/local/Cellar/qt/5.2.1/bin/qmake /usr/local/bin/qmake

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
    ./configure --prefix=/usr/local/Cellar/zeromq/2.2.0
    make -j9
    make install
    make clean
    CXXFLAGS="-m32" ./configure --prefix=/usr/local/Cellar/zeromq/2.2.0-32
    make -j9
    make install

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

    curl -OL http://sourceforge.net/p/freeimage/bugs/228/attachment/Makefile.osx-10.9
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
    ./configure --prefix=/usr/local/Cellar/curl/7.30.0 --enable-shared=no --disable-ldap --without-libidn --without-ssl
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
    make install PREFIX=/usr/local/Cellar/oscpack/1.1.0
    ar -r liboscpack.a `find . -name *.o`  # ... if liboscpack.a doesn't already exist
    cp liboscpack.a /usr/local/Cellar/oscpack/1.1.0/lib
    mkdir -p /usr/local/Cellar/oscpack/1.1.0/include/oscpack/{osc,ip}
    cp osc/*.h /usr/local/Cellar/oscpack/1.1.0/include/oscpack/osc
    cp ip/*.h /usr/local/Cellar/oscpack/1.1.0/include/oscpack/ip

### ld64 133.3

In the instructions below, `$ROOT` is the location of the top-level Vuo source code directory. 

Create a symbolic link to ld64 133.3 in the same directory as Clang (to force Clang to use this linker): 

    ln -s $ROOT/compiler/binary/ld /usr/local/Cellar/llvm/3.2/bin/ld

### OpenSSL (unnecessary for Vuo end users)

    cd /tmp
    curl -OL http://www.openssl.org/source/openssl-1.0.1g.tar.gz
    tar zxf openssl-1.0.1g.tar.gz
    cd openssl-1.0.1g
    ./Configure --prefix=/usr/local/Cellar/openssl/1.0.1g --openssldir=/usr/local/etc/openssl no-zlib no-shared darwin64-x86_64-cc
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

Install GLFW:

    brew install glfw


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

Generate the makefiles:

    qmake -r

Run the makefiles:

    make -j9
    make -j9 examples
    make -j9 docs

Run the tests:

    make tests

You can now run the example compositions from the command line. For example:

    ./example/graph/Countdown

You can now run the command-line tools:

    ./compiler/vuo-compile/vuo-compile --help
    ./compiler/vuo-link/vuo-link --help
    ./renderer/vuo-render/vuo-render --help

For examples of compiling a node class with `vuo-compile`, do:

    cd node && make clean && qmake -r && make

For examples of compiling and linking a composition with `vuo-compile` and `vuo-link`, do:

    cd example/graph && make clean && qmake -r && make

For examples of rendering a node class or composition with `vuo-render`, do:

    cd documentation && make clean && qmake -r && make
