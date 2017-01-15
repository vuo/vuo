VUO_VERSION = 1.2.1

ROOT = $$system(pwd)
DEFINES += VUO_ROOT=\\\"$$ROOT\\\"

BUILD_ID = $$(BUILD_ID)
isEmpty(BUILD_ID) { VUO_VERSION_AND_BUILD = $${VUO_VERSION}.$$basename(ROOT) }
else              { VUO_VERSION_AND_BUILD = $${VUO_VERSION}.$$BUILD_ID }

# Always build with debug symbols
CONFIG += debug

# Qt 5's qmake automatically sets DEPENDPATH to INCLUDEPATH.
# Disable this behavior, since our project files already specify DEPENDPATH manually,
# and since the longer DEPENDPATH adds about 10% to qmake's runtime.
CONFIG -= depend_includepath

VUO_QMAKE_CONFIG =

QMAKE_CLEAN += -R
QMAKE_CLEAN += Makefile $$TARGET lib$${TARGET}.a \"$${TARGET}.app\" pch *.dSYM *.o *.dylib moc_* *.moc *.vuonode *.vuonode+ *.bc *.bc+ *.gcno *.gcda

LLVM_ROOT = /usr/local/Cellar/llvm/3.2
LLVM_DYLIB = libLLVM-3.2svn.dylib
JSONC_ROOT = /usr/local/Cellar/json-c/0.12
GRAPHVIZ_ROOT = /usr/local/Cellar/graphviz/2.28.0
QT_ROOT = /usr/local/Cellar/qt/5.3.1
LIBFFI_ROOT = /usr/local/Cellar/libffi/3.0.11
ZLIB_ROOT = /usr/local/Cellar/zlib/1.2.8
ZMQ_ROOT = /usr/local/Cellar/zeromq/2.2.0
OPENSSL_ROOT = /usr/local/Cellar/openssl/1.0.1g
MUPARSER_ROOT = /usr/local/Cellar/muparser/2.2.3
FREEIMAGE_ROOT = /usr/local/Cellar/freeimage/3.15.4
CURL_ROOT = /usr/local/Cellar/curl/7.30.0
RTMIDI_ROOT = /usr/local/Cellar/rtmidi/2.0.1
RTAUDIO_ROOT = /usr/local/Cellar/rtaudio/4.0.12
GAMMA_ROOT = /usr/local/Cellar/gamma/0.9.5
ASSIMP_ROOT = /usr/local/Cellar/assimp/3.1.1
DISCOUNT_ROOT = /usr/local/Cellar/discount/2.1.6
FFMPEG_ROOT = /usr/local/Cellar/ffmpeg/2.1
LIBUSB_ROOT = /usr/local/Cellar/libusb/1.0.9
LIBFREENECT_ROOT = /usr/local/Cellar/libfreenect/0.2.0
OSCPACK_ROOT = /usr/local/Cellar/oscpack/1.1.0
ZXING_ROOT = /usr/local/Cellar/zxing/2.3.0
LIBXML2_ROOT = /usr/local/Cellar/libxml2/2.9.2
GHOSTSCRIPT_ROOT = /usr/local/Cellar/ghostscript/9.15
PNGQUANT_ROOT = /usr/local/Cellar/pngquant/2.3.1

# Don't assume we want the Qt libraries, but do still invoke moc and uic.
QT -= core gui widgets printsupport
CONFIG += moc uic

QMAKE_CC = $${LLVM_ROOT}/bin/clang
QMAKE_CXX = $${LLVM_ROOT}/bin/clang++
analyze {
	QMAKE_CC = $$ROOT/base/build-and-analyze $$LLVM_ROOT/bin/clang $$ROOT
	QMAKE_CXX = $$ROOT/base/build-and-analyze $$LLVM_ROOT/bin/clang++ $$ROOT
	VUO_QMAKE_CONFIG += analyze
}
MACOSX_SDK_FOLDER = /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs
exists($$MACOSX_SDK_FOLDER/MacOSX10.9.sdk) { QMAKE_MAC_SDK.macosx.path = $$MACOSX_SDK_FOLDER/MacOSX10.9.sdk }
else { exists($$MACOSX_SDK_FOLDER/MacOSX10.10.sdk) { QMAKE_MAC_SDK.macosx.path = $$MACOSX_SDK_FOLDER/MacOSX10.10.sdk } }
QMAKE_MAC_SDK.$$basename(QMAKESPEC).$${QMAKE_MAC_SDK}.QMAKE_CC = $$QMAKE_CC
QMAKE_MAC_SDK.$$basename(QMAKESPEC).$${QMAKE_MAC_SDK}.QMAKE_CXX = $$QMAKE_CXX
QMAKE_LINK = $${LLVM_ROOT}/bin/clang++
QMAKE_MAC_SDK.$$basename(QMAKESPEC).$${QMAKE_MAC_SDK}.QMAKE_LINK = $$QMAKE_LINK

VUOCOMPILE = $$ROOT/compiler/vuo-compile/vuo-compile
VUOLINK = $$ROOT/framework/vuo-link
VUORENDER = $$ROOT/framework/vuo-render

WARNING_REMOVE = -W
WARNING_ADD = \
#	-Weverything \
	-Wunreachable-code \
	-Wobjc-property-no-attribute \
	-Wno-unused-parameter \
	-Wno-c++11-extensions \
	-Wno-variadic-macros \
	-Wno-disabled-macro-expansion \
	-Wno-padded \
	-Wno-non-virtual-dtor \
	-Wno-shadow \
	-Wno-missing-variable-declarations \
	-Wno-missing-prototypes
QMAKE_CFLAGS_WARN_ON -= $$WARNING_REMOVE
QMAKE_CFLAGS_WARN_ON += -std=c99 $$WARNING_ADD
QMAKE_CXXFLAGS_WARN_ON -= $$WARNING_REMOVE
QMAKE_CXXFLAGS_WARN_ON += $$WARNING_ADD
QMAKE_OBJECTIVE_CFLAGS_WARN_ON -= $$WARNING_REMOVE
QMAKE_OBJECTIVE_CFLAGS_WARN_ON += $$WARNING_ADD

mac {
	DEFINES += MAC

	MAC_VERSION = $$system(sw_vers -productVersion | cut -d. -f "1,2")

	# Avoid "malformed object" / "unknown load command" errors.
	QMAKE_LFLAGS_I386 += -Wl,-no_function_starts
	QMAKE_LFLAGS_I386 += -Wl,-no_version_load_command
	QMAKE_LFLAGS_X86_64 += -Wl,-no_function_starts
	QMAKE_LFLAGS_X86_64 += -Wl,-no_version_load_command

	QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.7

	!isEmpty(VUO_INFO_PLIST) : isEmpty(API_HEADER_LISTS) {
		# Fill in version number
		infoPlist.commands = \
			cat "$$VUO_INFO_PLIST" \
			| sed '"s/@SHORT_VERSION@/$$VUO_VERSION \\(r`svnversion -n` on `date +%Y.%m.%d`\\)/"' \
			> "$$VUO_INFO_PLIST_GENERATED"
		infoPlist.depends = $$VUO_INFO_PLIST
		infoPlist.target = $$VUO_INFO_PLIST_GENERATED
		PRE_TARGETDEPS += "$$system(echo $$VUO_INFO_PLIST_GENERATED)"
		QMAKE_EXTRA_TARGETS += infoPlist
		QMAKE_CLEAN += "$$VUO_INFO_PLIST_GENERATED"
	}
}

LIBS += -lm

FLAGS64 = -march=x86-64
QMAKE_CFLAGS_X86_64 += $$FLAGS64
QMAKE_CXXFLAGS_X86_64 += $$FLAGS64
QMAKE_LFLAGS_X86_64 += $$FLAGS64

FLAGS32 = -m32
QMAKE_CFLAGS_I386 += $$FLAGS32
QMAKE_CXXFLAGS_I386 += $$FLAGS32
QMAKE_LFLAGS_I386 += $$FLAGS32

Vuo32 {
	FLAGS_ARCH = $$FLAGS32
}
else {
	FLAGS_ARCH = $$FLAGS64
}

FLAGS = $$FLAGS_ARCH \
	-g \
	-fvisibility-inlines-hidden \
	-Wdocumentation \
	-Wno-documentation-deprecated-sync \
	-Oz

coverage {
	FLAGS += --coverage
	DEFINES += COVERAGE
	QMAKE_LFLAGS += $$LLVM_ROOT/lib/libprofile_rt.dylib
	VUO_QMAKE_CONFIG += coverage
}

profile {
	DEFINES += PROFILE
}

QMAKE_CFLAGS_RELEASE += $$FLAGS
QMAKE_CFLAGS_DEBUG += $$FLAGS
QMAKE_CXXFLAGS_RELEASE += $$FLAGS
QMAKE_CXXFLAGS_DEBUG += $$FLAGS
QMAKE_OBJECTIVE_CFLAGS_RELEASE += $$FLAGS
QMAKE_OBJECTIVE_CFLAGS_DEBUG += $$FLAGS
QMAKE_LFLAGS += -Oz

CONFIG(debug, debug|release) {
	DEFINES += DEBUG
	QMAKE_CFLAGS_BASELINE = $${QMAKE_CFLAGS_DEBUG}
}
else {
	QMAKE_CFLAGS_BASELINE = $${QMAKE_CFLAGS_RELEASE}
}

VuoLLVM {
	DEFINES += LLVM
	LLVM_FLAGS = -I$${LLVM_ROOT}/include
	QMAKE_CFLAGS_RELEASE += $$LLVM_FLAGS
	QMAKE_CFLAGS_DEBUG += $$LLVM_FLAGS
	QMAKE_CXXFLAGS_RELEASE += $$LLVM_FLAGS
	QMAKE_CXXFLAGS_DEBUG += $$LLVM_FLAGS
	QMAKE_OBJECTIVE_CFLAGS_RELEASE += $$LLVM_FLAGS
	QMAKE_OBJECTIVE_CFLAGS_DEBUG += $$LLVM_FLAGS

	LIBS += \
		$${LIBFFI_ROOT}/lib/libffi.a \
		$${LLVM_ROOT}/lib/$$LLVM_DYLIB \
		$${LLVM_ROOT}/lib/libclangAnalysis.a \
		$${LLVM_ROOT}/lib/libclangAST.a \
		$${LLVM_ROOT}/lib/libclangBasic.a \
		$${LLVM_ROOT}/lib/libclangCodeGen.a \
		$${LLVM_ROOT}/lib/libclangDriver.a \
		$${LLVM_ROOT}/lib/libclangEdit.a \
		$${LLVM_ROOT}/lib/libclangFrontend.a \
		$${LLVM_ROOT}/lib/libclangLex.a \
		$${LLVM_ROOT}/lib/libclangParse.a \
		$${LLVM_ROOT}/lib/libclangSema.a \
		$${LLVM_ROOT}/lib/libclangSerialization.a

	QMAKE_POST_LINK = ([ -x $$TARGET ] && install_name_tool -change "@executable_path/../lib/$$LLVM_DYLIB" "$$LLVM_ROOT/lib/$$LLVM_DYLIB" $$TARGET) ; true
}

graphviz {
	LIBS += \
		$${ZLIB_ROOT}/lib/libz.a \
		$${GRAPHVIZ_ROOT}/lib/libxdot.dylib \
		$${GRAPHVIZ_ROOT}/lib/graphviz/libgvplugin_dot_layout.dylib \
		$${GRAPHVIZ_ROOT}/lib/graphviz/libgvplugin_core.dylib \
		$${GRAPHVIZ_ROOT}/lib/libgvc.dylib \
		$${GRAPHVIZ_ROOT}/lib/libpathplan.dylib \
		$${GRAPHVIZ_ROOT}/lib/libgraph.dylib \
		$${GRAPHVIZ_ROOT}/lib/libcdt.dylib
	INCLUDEPATH += \
		$${GRAPHVIZ_ROOT}/include
}

json | VuoBase {
	LIBS += $${JSONC_ROOT}/lib/libjson-c.a
	INCLUDEPATH += $$JSONC_ROOT/include
}

zmq | VuoRuntime {
	DEFINES += ZMQ
	LIBS += $${ZMQ_ROOT}/lib/libzmq.a
	INCLUDEPATH += $${ZMQ_ROOT}/include
}

openssl | VuoBase {
	LIBS += \
	$${OPENSSL_ROOT}/lib/libcrypto.a \
	$${OPENSSL_ROOT}/lib/libssl.a
	INCLUDEPATH += $${OPENSSL_ROOT}/include
}

discount | VuoBase {
	LIBS += $$DISCOUNT_ROOT/lib/libmarkdown.a
	INCLUDEPATH += $$DISCOUNT_ROOT/include
}

Carbon | VuoCompiler {
	mac {
		INCLUDEPATH += $$QMAKE_MAC_SDK/System/Library/Frameworks/CoreServices.framework/Headers
		LIBS += -framework Carbon
	}
}

qtCore {
	INCLUDEPATH += $$QT_ROOT/lib/QtCore.framework/Headers
	LIBS += \
		$${ZLIB_ROOT}/lib/libz.a \
		-lm \
		-F$${QT_ROOT}/lib/ \
		-framework ApplicationServices \
		-framework CoreFoundation \
		-framework Security \
		-framework QtCore
	QMAKE_CXXFLAGS += -F$$QT_ROOT/lib
	QMAKE_OBJECTIVE_CFLAGS += -F$$QT_ROOT/lib
	DEFINES += QT_CORE_LIB
}

qtGui {
	LIBS += \
		$${ZLIB_ROOT}/lib/libz.a \
		-lm \
		-framework ApplicationServices \
		-framework CoreFoundation \
		-framework Security \
		-framework Carbon \
		-framework AppKit \
		-F$${QT_ROOT}/lib/ \
		-framework QtCore \
		-framework QtGui \
		-framework QtWidgets \
		-framework QtPrintSupport
	CONFIG += qtGuiIncludes
	mac {
		LIBS += -framework QtMacExtras
	}
}
qtGuiIncludes {
	QMAKE_CXXFLAGS += -F$$QT_ROOT/lib
	INCLUDEPATH += $$QT_ROOT/lib/QtCore.framework/Headers
	DEFINES += QT_GUI_LIB
}

qtNetwork {
	LIBS += \
		-F$${QT_ROOT}/lib/ \
		-framework QtCore \
		-framework QtNetwork
	CONFIG += qtNetworkIncludes
}

qtNetworkIncludes {
	QMAKE_CXXFLAGS += -F$$QT_ROOT/lib
	INCLUDEPATH += $$QT_ROOT/lib/QtNetwork.framework/Headers
	DEFINES += QT_NETWORK_LIB
}

qtOpenGL {
	LIBS += \
		$${ZLIB_ROOT}/lib/libz.a \
		-lm \
		-framework ApplicationServices \
		-framework CoreFoundation \
		-framework Security \
		-framework Carbon \
		-framework AppKit \
		-framework OpenGL \
		-framework AGL \
		-F$${QT_ROOT}/lib/ \
		-framework QtCore \
		-framework QtGui \
		-framework QtWidgets \
		-framework QtPrintSupport \
		-framework QtOpenGL
	mac {
		LIBS += -framework QtMacExtras
	}
	DEFINES += QT_OPENGL_LIB
}

qtTest {
	LIBS += \
		$${ZLIB_ROOT}/lib/libz.a \
		-lm \
		-framework IOKit \
		-framework Security \
		-framework Cocoa \
		-framework CoreFoundation \
		-F$${QT_ROOT}/lib/ \
		-framework QtCore \
		-framework QtTest
	QMAKE_CXXFLAGS += -F$$QT_ROOT/lib
	QMAKE_OBJECTIVE_CFLAGS += -F$$QT_ROOT/lib
	DEFINES += QT_TESTLIB_LIB
}

VuoInputEditor {
	CONFIG += VuoPCH_objcxx
}

VuoPCH | VuoBase | VuoCompiler | VuoRenderer | VuoEditor {
	CONFIG += precompile_header clang_pch_style
	PRECOMPILED_HEADER = $$ROOT/vuo.pch
	QMAKE_CLEAN += pch
	PRECOMPILED_DIR = pch
	QMAKE_PCH_OUTPUT_EXT = .pch

	QMAKE_CFLAGS_PRECOMPILE += $$FLAGS_ARCH
	QMAKE_CFLAGS_USE_PRECOMPILE = -Xclang -include-pch -Xclang ${QMAKE_PCH_OUTPUT}

	# Since VUO_VERSION changes frequently, define it when compiling source files, but not when precompiling headers.
	VUO_VERSION_DEFINES += \
		-DVUO_VERSION=$$VUO_VERSION \
		-DVUO_VERSION_STRING=\\\"$$VUO_VERSION\\\" \
		-DVUO_VERSION_AND_BUILD_STRING=\\\"$$VUO_VERSION_AND_BUILD\\\"
	QMAKE_CFLAGS_USE_PRECOMPILE += $$VUO_VERSION_DEFINES

	QMAKE_CXXFLAGS_PRECOMPILE += $$FLAGS_ARCH
	QMAKE_CXXFLAGS_USE_PRECOMPILE = -Xclang -include-pch -Xclang
	VuoInputEditor {
		QMAKE_CXXFLAGS_USE_PRECOMPILE += ../widget/pch/widget/c++.pch
	} else {
		TestVuoCompiler {
			QMAKE_CXXFLAGS_USE_PRECOMPILE += ../TestVuoCompiler/pch/TestVuoCompiler/c++.pch
		} else {
			QMAKE_CXXFLAGS_USE_PRECOMPILE += ${QMAKE_PCH_OUTPUT}
		}
	}
	QMAKE_CXXFLAGS_USE_PRECOMPILE += $$VUO_VERSION_DEFINES

	VuoPCH_objc {
		QMAKE_OBJCFLAGS_PRECOMPILE += $$FLAGS_ARCH
		QMAKE_OBJCFLAGS_USE_PRECOMPILE = $$QMAKE_CFLAGS_USE_PRECOMPILE
	} else {
		QMAKE_OBJCFLAGS_PRECOMPILE =
		QMAKE_OBJECTIVE_CFLAGS += $$VUO_VERSION_DEFINES
	}

	VuoPCH_objcxx {
		QMAKE_EXT_CPP += .mm
		QMAKE_OBJCXXFLAGS_PRECOMPILE += $$FLAGS_ARCH
		VuoInputEditor {
			QMAKE_OBJECTIVE_CFLAGS += $$QMAKE_CXXFLAGS_X86_64 -Xclang -include-pch -Xclang ../widget/pch/widget/objective-c++.pch
		} else {
			QMAKE_OBJECTIVE_CFLAGS += $$QMAKE_CXXFLAGS_X86_64 -Xclang -include-pch -Xclang pch/$$TARGET/objective-c++.pch
		}
	} else {
		QMAKE_OBJCXXFLAGS_PRECOMPILE =
	}

	# For VuoLog.h
	PCH_INCLUDE_PATHS += \
		$$JSONC_ROOT/include \
		$$ROOT/library \
		$$ROOT/node/vuo.font \
		$$ROOT/node/vuo.ui \
		$$ROOT/type \
		$$ROOT/type/list
	INCLUDEPATH += $$PCH_INCLUDE_PATHS
	DEPENDPATH += $$PCH_INCLUDE_PATHS
	DEPENDPATH -= $$JSONC_ROOT/include
} else {
	QMAKE_CFLAGS_PRECOMPILE =
	QMAKE_CXXFLAGS_PRECOMPILE =
	QMAKE_OBJCFLAGS_PRECOMPILE =
	QMAKE_OBJCXXFLAGS_PRECOMPILE =
}

VuoBase {
	LIBS += -L$$ROOT/base -lVuoBase -lobjc -framework Foundation
	INCLUDEPATH += $$ROOT/base
	DEPENDPATH += $$ROOT/base
	PRE_TARGETDEPS += $$ROOT/base/libVuoBase.a
	HEADERS += $$ROOT/base/*.hh
}
VuoCompiler {
	LIBS += -L$$ROOT/compiler -lVuoCompiler
	INCLUDEPATH += $$ROOT/compiler
	DEPENDPATH += $$ROOT/compiler
	PRE_TARGETDEPS += $$ROOT/compiler/libVuoCompiler.a
	HEADERS += $$ROOT/compiler/*.hh
}
VuoRenderer {
	LIBS += -L$$ROOT/renderer -lVuoRenderer
	INCLUDEPATH += $$ROOT/renderer
	DEPENDPATH += $$ROOT/renderer
	PRE_TARGETDEPS += $$ROOT/renderer/libVuoRenderer.a
	HEADERS += $$ROOT/renderer/*.hh
}
VuoEditor {
	LIBS += -L$$ROOT/editor -lVuoEditor
	INCLUDEPATH += $$ROOT/editor
	DEPENDPATH += $$ROOT/editor
	PRE_TARGETDEPS += $$ROOT/editor/libVuoEditor.a
}
VuoFramework {
	DEFINES += USING_VUO_FRAMEWORK
	QMAKE_CXX = $$ROOT/framework/Vuo.framework/llvm.framework/Helpers/clang++
	QMAKE_LINK = $$QMAKE_CXX
	QMAKE_CXXFLAGS += -F$$ROOT/framework
	QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-private-field
	QMAKE_LFLAGS += -F$$ROOT/framework
	INCLUDEPATH += $$JSONC_ROOT/include
	DEPENDPATH += $$ROOT/framework/Vuo.framework/Headers
	LIBS += -framework Vuo -framework OpenGL
	PRE_TARGETDEPS += $$ROOT/framework/Vuo.framework/Vuo
}
VuoRuntime {
	INCLUDEPATH += $$ROOT/runtime
	DEPENDPATH += $$ROOT/runtime
}
VuoType {
	LIBS += \
		-L$$ROOT/type -lVuoType \
		-L$$ROOT/type/list -lVuoTypeList
	INCLUDEPATH += \
		$$ROOT/type	\
		$$ROOT/type/list
}
VuoInputEditorWidget | VuoInputEditor {
	INCLUDEPATH += $$ROOT/type/inputEditor/widget
	DEPENDPATH += $$ROOT/type/inputEditor/widget
	LIBS += -L$$ROOT/type/inputEditor/widget -lwidget
	PRE_TARGETDEPS += $$ROOT/type/inputEditor/widget/libwidget.a
}
VuoInputEditor {
	# Don't rebuild PCH; just reuse inputEditors/widget's PCH.
	QMAKE_CFLAGS_PRECOMPILE =
	QMAKE_CXXFLAGS_PRECOMPILE =
	QMAKE_OBJCFLAGS_PRECOMPILE =
	QMAKE_OBJCXXFLAGS_PRECOMPILE =

	INCLUDEPATH += \
		$$ROOT/type \
		$$ROOT/type/list
	QMAKE_LFLAGS += \
		-Wl,-no_function_starts \
		-Wl,-no_version_load_command
}
TestVuoCompiler {
	# Don't rebuild PCH; just reuse TestVuoCompiler's PCH.
	QMAKE_CFLAGS_PRECOMPILE =
	QMAKE_CXXFLAGS_PRECOMPILE =
	QMAKE_OBJCFLAGS_PRECOMPILE =
	QMAKE_OBJCXXFLAGS_PRECOMPILE =

	LIBS += -L$$ROOT/test/TestVuoCompiler -lTestVuoCompiler \
		-framework AppKit
	INCLUDEPATH += $$ROOT/test/TestVuoCompiler
	PRE_TARGETDEPS += $$ROOT/test/TestVuoCompiler/libTestVuoCompiler.a
}
TestCompositionExecution {
	LIBS += -L$$ROOT/test/TestCompositionExecution -lTestCompositionExecution
	INCLUDEPATH += $$ROOT/test/TestCompositionExecution
	PRE_TARGETDEPS += $$ROOT/test/TestCompositionExecution/libTestCompositionExecution.a
}
