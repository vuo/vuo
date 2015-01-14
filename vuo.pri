VUO_VERSION = 0.5.0
DEFINES += VUO_VERSION=$$VUO_VERSION
DEFINES += VUO_VERSION_STRING=\\\"$$VUO_VERSION\\\"

ROOT = $$system(pwd)
DEFINES += VUO_ROOT=\\\"$$ROOT\\\"

# Always build with debug symbols
CONFIG += debug

# Qt 5's qmake automatically sets DEPENDPATH to INCLUDEPATH.
# Disable this behavior, since our project files already specify DEPENDPATH manually,
# and since the longer DEPENDPATH adds about 10% to qmake's runtime.
CONFIG -= depend_includepath

QMAKE_CLEAN += -R
QMAKE_CLEAN += Makefile $$TARGET lib$${TARGET}.a $${TARGET}.app

LLVM_ROOT = /usr/local/Cellar/llvm/3.2
ICU_ROOT = /usr/local/Cellar/icu4c/4.8.1.1
JSONC_ROOT = /usr/local/Cellar/json-c/0.10
GRAPHVIZ_ROOT = /usr/local/Cellar/graphviz/2.28.0
QT_ROOT = /usr/local/Cellar/qt/5.1.1
LIBFFI_ROOT = /usr/local/Cellar/libffi/3.0.11
ZLIB_ROOT = /usr/local/Cellar/zlib/1.2.8
ZMQ_ROOT = /usr/local/Cellar/zeromq/2.2.0
OPENSSL_ROOT = /usr/local/Cellar/openssl/1.0.1c
MUPARSER_ROOT = /usr/local/Cellar/muparser/2.2.3
FREEIMAGE_ROOT = /usr/local/Cellar/freeimage/3.15.4
FREETYPE_ROOT = /usr/local/Cellar/freetype/2.5.0.1
CURL_ROOT = /usr/local/Cellar/curl/7.30.0
RTMIDI_ROOT = /usr/local/Cellar/rtmidi/2.0.1
ASSIMP_ROOT = /usr/local/Cellar/assimp/3.0.1270

# Don't assume we want the Qt libraries, but do still invoke moc and uic.
QT -= core gui widgets printsupport
CONFIG += moc uic

QMAKE_CC = $${LLVM_ROOT}/bin/clang
QMAKE_MAC_SDK.$$basename(QMAKESPEC).$${QMAKE_MAC_SDK}.QMAKE_CC = $$QMAKE_CC
QMAKE_CXX = $${LLVM_ROOT}/bin/clang++
QMAKE_MAC_SDK.$$basename(QMAKESPEC).$${QMAKE_MAC_SDK}.QMAKE_CXX = $$QMAKE_CXX
QMAKE_LINK = $${LLVM_ROOT}/bin/clang++
QMAKE_MAC_SDK.$$basename(QMAKESPEC).$${QMAKE_MAC_SDK}.QMAKE_LINK = $$QMAKE_LINK

VUOCOMPILE = $$ROOT/compiler/vuo-compile/vuo-compile
VUOLINK = $$ROOT/framework/vuo-link
VUORENDER = $$ROOT/framework/vuo-render

WARNING_REMOVE = -W
WARNING_ADD = -Wno-unused-parameter
QMAKE_CFLAGS_WARN_ON -= $$WARNING_REMOVE
QMAKE_CFLAGS_WARN_ON += $$WARNING_ADD
QMAKE_CXXFLAGS_WARN_ON -= $$WARNING_REMOVE
QMAKE_CXXFLAGS_WARN_ON += $$WARNING_ADD
QMAKE_OBJECTIVE_CFLAGS_WARN_ON -= $$WARNING_REMOVE
QMAKE_OBJECTIVE_CFLAGS_WARN_ON += $$WARNING_ADD

mac {
	DEFINES += MAC

	MAC_VERSION = $$system(sw_vers -productVersion | cut -d. -f "1,2")

	# Avoid "malformed object" / "unknown load command" errors.
	QMAKE_LFLAGS_X86_64 += -Wl,-no_function_starts
	QMAKE_LFLAGS_X86_64 += -Wl,-no_version_load_command

	QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.6
}

LIBS += -lm

FLAGS64 = -march=x86-64
QMAKE_CFLAGS_X86_64 += $$FLAGS64
QMAKE_CXXFLAGS_X86_64 += $$FLAGS64
QMAKE_LFLAGS_X86_64 += $$FLAGS64

FLAGS = $$FLAGS64 -fvisibility-inlines-hidden -Wdocumentation -Wno-documentation-deprecated-sync
QMAKE_CFLAGS_RELEASE += $$FLAGS
QMAKE_CFLAGS_DEBUG += $$FLAGS
QMAKE_CXXFLAGS_RELEASE += $$FLAGS
QMAKE_CXXFLAGS_DEBUG += $$FLAGS

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
		$${LLVM_ROOT}/lib/libLLVMAnalysis.a \
		$${LLVM_ROOT}/lib/libLLVMArchive.a \
		$${LLVM_ROOT}/lib/libLLVMAsmParser.a \
		$${LLVM_ROOT}/lib/libLLVMAsmPrinter.a \
		$${LLVM_ROOT}/lib/libLLVMBitReader.a \
		$${LLVM_ROOT}/lib/libLLVMBitWriter.a \
		$${LLVM_ROOT}/lib/libLLVMCodeGen.a \
		$${LLVM_ROOT}/lib/libLLVMCore.a \
		$${LLVM_ROOT}/lib/libLLVMExecutionEngine.a \
		$${LLVM_ROOT}/lib/libLLVMInstCombine.a \
		$${LLVM_ROOT}/lib/libLLVMInstrumentation.a \
		$${LLVM_ROOT}/lib/libLLVMInterpreter.a \
		$${LLVM_ROOT}/lib/libLLVMJIT.a \
		$${LLVM_ROOT}/lib/libLLVMLinker.a \
		$${LLVM_ROOT}/lib/libLLVMMC.a \
		$${LLVM_ROOT}/lib/libLLVMMCParser.a \
		$${LLVM_ROOT}/lib/libLLVMObject.a \
		$${LLVM_ROOT}/lib/libLLVMRuntimeDyld.a \
		$${LLVM_ROOT}/lib/libLLVMScalarOpts.a \
		$${LLVM_ROOT}/lib/libLLVMSelectionDAG.a \
		$${LLVM_ROOT}/lib/libLLVMSupport.a \
		$${LLVM_ROOT}/lib/libLLVMTarget.a \
		$${LLVM_ROOT}/lib/libLLVMTransformUtils.a \
		$${LLVM_ROOT}/lib/libLLVMVectorize.a \
		$${LLVM_ROOT}/lib/libLLVMX86AsmParser.a \
		$${LLVM_ROOT}/lib/libLLVMX86AsmPrinter.a \
		$${LLVM_ROOT}/lib/libLLVMX86CodeGen.a \
		$${LLVM_ROOT}/lib/libLLVMX86Desc.a \
		$${LLVM_ROOT}/lib/libLLVMX86Info.a \
		$${LLVM_ROOT}/lib/libLLVMX86Utils.a \
		$${LLVM_ROOT}/lib/libLLVMipa.a \
		$${LLVM_ROOT}/lib/libLLVMipo.a \
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
		$${LLVM_ROOT}/lib/libclangSerialization.a \
		$${LLVM_ROOT}/lib/libclangTooling.a
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
	LIBS += $${JSONC_ROOT}/lib/libjson.a
	INCLUDEPATH += $$JSONC_ROOT/include
}

icu {
	INCLUDEPATH += $$ICU_ROOT/include
}

zmq | VuoRuntime {
	DEFINES += ZMQ
	LIBS += $${ZMQ_ROOT}/lib/libzmq.a
	INCLUDEPATH += $${ZMQ_ROOT}/include
}

openssl {
	LIBS += \
	$${OPENSSL_ROOT}/lib/libcrypto.a \
	$${OPENSSL_ROOT}/lib/libssl.a
	INCLUDEPATH += $${OPENSSL_ROOT}/include
}

Carbon | VuoCompiler {
	mac {
		INCLUDEPATH += $$QMAKE_MAC_SDK/System/Library/Frameworks/CoreServices.framework/Headers
		LIBS += -framework Carbon
	}
}

qtCore {
	LIBS += \
		$${ZLIB_ROOT}/lib/libz.a \
		-lm \
		-F$${QT_ROOT}/lib/ \
		-framework ApplicationServices \
		-framework CoreFoundation \
		-framework Security \
		-framework QtCore
	INCLUDEPATH += \
		$${QT_ROOT}/include \
		$${QT_ROOT}/include/QtCore
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
	INCLUDEPATH += \
		$${QT_ROOT}/include \
		$${QT_ROOT}/include/QtCore \
		$${QT_ROOT}/include/QtGui \
		$${QT_ROOT}/include/QtWidgets \
		$${QT_ROOT}/include/QtPrintSupport
	mac {
		INCLUDEPATH += $${QT_ROOT}/include/QtMacExtras
	}
	DEFINES += QT_GUI_LIB
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
	INCLUDEPATH += \
		$${QT_ROOT}/include \
		$${QT_ROOT}/include/QtCore \
		$${QT_ROOT}/include/QtGui \
		$${QT_ROOT}/include/QtWidgets \
		$${QT_ROOT}/include/QtPrintSupport \
		$${QT_ROOT}/include/QtOpenGL
	mac {
		LIBS += -framework QtMacExtras
		INCLUDEPATH += $${QT_ROOT}/include/QtMacExtras
	}
	DEFINES += QT_OPENGL_LIB
}

qtTest {
	LIBS += \
		$${ZLIB_ROOT}/lib/libz.a \
		-lm \
		-framework IOKit \
		-framework Security \
		-framework ApplicationServices \
		-framework CoreFoundation \
		-F$${QT_ROOT}/lib/ \
		-framework QtCore \
		-framework QtTest
	INCLUDEPATH += \
		$${QT_ROOT}/include \
		$${QT_ROOT}/include/QtCore \
		$${QT_ROOT}/include/QtTest
	DEFINES += QT_TESTLIB_LIB
}

VuoPCH | VuoBase | VuoCompiler | VuoRenderer | VuoEditor {
	CONFIG += precompile_header clang_pch_style
	PRECOMPILED_HEADER = $$ROOT/vuo.pch
	QMAKE_CLEAN += pch
	PRECOMPILED_DIR = pch
	QMAKE_PCH_OUTPUT_EXT = .pch
	QMAKE_CFLAGS_PRECOMPILE += $$QMAKE_CFLAGS_X86_64
	QMAKE_CFLAGS_USE_PRECOMPILE = -Xclang -include-pch -Xclang ${QMAKE_PCH_OUTPUT}
	QMAKE_CXXFLAGS_PRECOMPILE += $$QMAKE_CXXFLAGS_X86_64
	QMAKE_CXXFLAGS_USE_PRECOMPILE = $$QMAKE_CFLAGS_USE_PRECOMPILE

	# Don't bother building precompiled headers for Objective-C++ (we only want C++)
	QMAKE_OBJCXXFLAGS_PRECOMPILE =
}

VuoBase {
	LIBS += -L$$ROOT/base -lVuoBase
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
	QMAKE_CXX = $$ROOT/framework/Vuo.framework/MacOS/Clang/bin/clang++
	QMAKE_LINK = $$QMAKE_CXX
	QMAKE_CXXFLAGS += -F$$ROOT/framework
	QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-private-field
	QMAKE_LFLAGS += -F$$ROOT/framework
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
VuoInputEditor {
	SOURCES += \
		$$ROOT/editor/VuoInputEditor.cc
	HEADERS += \
		$$ROOT/editor/VuoInputEditor.hh
	INCLUDEPATH += \
		$$ROOT/type \
		$$ROOT/editor
	QMAKE_LFLAGS += \
		-Wl,-no_function_starts \
		-Wl,-no_version_load_command
}
VuoInputEditorWithLineEdit {
	SOURCES += \
		$$ROOT/editor/VuoInputEditorWithLineEdit.cc \
		$$ROOT/editor/VuoInputEditorWithDialog.cc \
		$$ROOT/editor/VuoDialogForInputEditor.cc
	HEADERS += \
		$$ROOT/editor/VuoInputEditorWithLineEdit.hh \
		$$ROOT/editor/VuoInputEditorWithDialog.hh \
		$$ROOT/editor/VuoDialogForInputEditor.hh
}
VuoInputEditorWithMenu {
	SOURCES += \
		$$ROOT/editor/VuoInputEditorWithMenu.cc \
		$$ROOT/editor/VuoMenu.cc
	HEADERS += \
		$$ROOT/editor/VuoInputEditorWithMenu.hh \
		$$ROOT/editor/VuoMenu.hh
}
TestVuoCompiler {
	LIBS += -L$$ROOT/test/TestVuoCompiler -lTestVuoCompiler
	INCLUDEPATH += $$ROOT/test/TestVuoCompiler
	PRE_TARGETDEPS += $$ROOT/test/TestVuoCompiler/libTestVuoCompiler.a
}
TestCompositionExecution {
	LIBS += -L$$ROOT/test/TestCompositionExecution -lTestCompositionExecution
	INCLUDEPATH += $$ROOT/test/TestCompositionExecution
	PRE_TARGETDEPS += $$ROOT/test/TestCompositionExecution/libTestCompositionExecution.a
}

# This is in vuo.pri, rather than a specific subproject such as node/node.pro, because multiple subprojects define NODE_SOURCES.
VUOCOMPILE_NODE_INCLUDEPATH = \
	$${ROOT}/node \
	$${ROOT}/type \
	$${ROOT}/type/list \
	$${ROOT}/runtime \
	$${JSONC_ROOT}/include \
	$${ASSIMP_ROOT}/include
node.input = NODE_SOURCES
node.depend_command = $$QMAKE_CC -o /dev/null -E -MD -MF - $$join(VUOCOMPILE_NODE_INCLUDEPATH, " -I", "-I") ${QMAKE_FILE_NAME} 2>&1 | sed \"s,^.*: ,,\"
node.output = ${QMAKE_FILE_IN_BASE}.vuonode
node.commands = $$VUOCOMPILE $$join(VUOCOMPILE_NODE_INCLUDEPATH, " --header-search-path ", "--header-search-path ") --output ${QMAKE_FILE_OUT} ${QMAKE_FILE_IN}
QMAKE_EXTRA_COMPILERS += node

# Hack to prevent qmake from building a static library from this project folder.
VuoNoLibrary {
	QMAKE_AR_CMD = @true
	QMAKE_MAC_SDK.$$basename(QMAKESPEC).$${QMAKE_MAC_SDK}.QMAKE_RANLIB = @true
}
