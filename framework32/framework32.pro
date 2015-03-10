TEMPLATE = aux
CONFIG -= qt
CONFIG += VuoPCH Vuo32

include(../vuo.pri)

FLAGS32 = -m32
QMAKE_CFLAGS += $$FLAGS32
QMAKE_CXXFLAGS += $$FLAGS32
QMAKE_LFLAGS += $$FLAGS32

ZMQ32_ROOT = /usr/local/Cellar/zeromq/2.2.0-32
JSONC32_ROOT = /usr/local/Cellar/json-c/0.10-32
ICU32_ROOT = /usr/local/Cellar/icu4c/52.1-32

DEFINES += ZMQ
INCLUDEPATH += \
	$$ZMQ32_ROOT/include \
	$$JSONC32_ROOT/include

INCLUDEPATH += \
	$$ROOT/base \
	$$ROOT/runtime

SOURCES = \
	$$ROOT/base/VuoBase.cc \
	$$ROOT/base/VuoCable.cc \
	$$ROOT/base/VuoFileUtilities.cc \
	$$ROOT/base/VuoModule.cc \
	$$ROOT/base/VuoNode.cc \
	$$ROOT/base/VuoNodeClass.cc \
	$$ROOT/base/VuoNodeSet.cc \
	$$ROOT/base/VuoPort.cc \
	$$ROOT/base/VuoPortClass.cc \
	$$ROOT/base/VuoRunner.cc \
	$$ROOT/base/VuoStringUtilities.cc \
	$$ROOT/base/VuoTelemetry.c \
	$$ROOT/base/miniz.c \
	$$ROOT/runtime/VuoHeap.cc \
	VuoRunner32.cc

HEADERS = \
	VuoRunner32.hh \
	Vuo32.h


# Build type and library object files
TYPE_AND_LIBRARY_SOURCES = \
	$$ROOT/type/VuoBoolean.c \
	$$ROOT/type/VuoColor.c \
	$$ROOT/type/VuoImage.c \
	$$ROOT/type/VuoInteger.c \
	$$ROOT/type/VuoReal.c \
	$$ROOT/type/VuoShader.c \
	$$ROOT/type/VuoText.c \
	$$ROOT/type/list/VuoList_VuoColor.cc \
	$$ROOT/type/list/VuoList_VuoImage.cc \
	$$ROOT/type/list/VuoList_VuoInteger.cc \
	$$ROOT/library/VuoGlContext.cc \
	$$ROOT/library/VuoGlPool.cc \
	$$ROOT/library/VuoImageRenderer.cc
TYPE_AND_LIBRARY_FLAGS = \
	$$QMAKE_CFLAGS \
	-I$$ROOT/library \
	-I$$ROOT/node \
	-I$$ROOT/runtime \
	-I$$ROOT/type \
	-I$$ROOT/type/list \
	-I$${ICU32_ROOT}/include \
	-I$${JSONC32_ROOT}/include
typeAndLibraryObjects.input = TYPE_AND_LIBRARY_SOURCES
typeAndLibraryObjects.output = ${QMAKE_FILE_IN_BASE}.o
typeAndLibraryObjects.commands = \
	$$QMAKE_CC \
		$$TYPE_AND_LIBRARY_FLAGS \
		-c -o ${QMAKE_FILE_OUT} ${QMAKE_FILE_IN}
type.depend_command = $$QMAKE_CC -o /dev/null -E -MD -MF - $$TYPE_AND_LIBRARY_FLAGS ${QMAKE_FILE_NAME} | sed \"s,^.*: ,,\"
typeAndLibraryObjects.CONFIG = target_predeps
QMAKE_EXTRA_COMPILERS += typeAndLibraryObjects


# Build 32-bit dynamic library for Vuo.framework

VUO_FRAMEWORK_SOURCES = $$SOURCES $$TYPE_AND_LIBRARY_SOURCES
for(sourcefile, VUO_FRAMEWORK_SOURCES) {
	objectfile = $${basename(sourcefile)}
	objectfile ~= s/\\.cc?$/.o
	VUO_FRAMEWORK_OBJECTS += $$objectfile
}

VUO_FRAMEWORK_BINARY_RELATIVE = Vuo.framework/Versions/$$VUO_VERSION/Vuo
VUO_FRAMEWORK_OBJECT_FLAGS += \
	$$join(VUO_FRAMEWORK_OBJECTS, " -Wl,-force_load,", "-Wl,-force_load,")
linkVuoFramework.commands = \
	$$QMAKE_LINK \
		$$QMAKE_LFLAGS \
		$$QMAKE_LFLAGS_SHLIB \
		-current_version $$VUO_VERSION \
		-install_name @rpath/$$VUO_FRAMEWORK_BINARY_RELATIVE \
		$$VUO_FRAMEWORK_OBJECT_FLAGS \
		$${ICU32_ROOT}/lib/libicuuc.a \
		$${ICU32_ROOT}/lib/libicudata.a \
		$${JSONC32_ROOT}/lib/libjson.a \
		$${ZMQ32_ROOT}/lib/libzmq.a \
		-framework CoreFoundation \
		-framework OpenGL \
		-framework IOSurface \
		-lobjc \
		-o libVuo32.dylib
linkVuoFramework.target = libVuo32.dylib
linkVuoFramework.depends = $$VUO_FRAMEWORK_OBJECTS
POST_TARGETDEPS += libVuo32.dylib
QMAKE_EXTRA_TARGETS += linkVuoFramework


# Combine 32-bit dynamic library with existing 64-bit dynamic library for Vuo.framework
VUO_FRAMEWORK_BINARY = $$ROOT/framework/$$VUO_FRAMEWORK_BINARY_RELATIVE
lipoVuoFramework.commands = cp $$VUO_FRAMEWORK_BINARY libVuo64.dylib
lipoVuoFramework.commands += && ( lipo -remove i386 libVuo64.dylib -output libVuo64.dylib || true )
lipoVuoFramework.commands += && lipo -create libVuo32.dylib libVuo64.dylib -output libVuoUniversal.dylib
lipoVuoFramework.commands += && cp libVuoUniversal.dylib $$VUO_FRAMEWORK_BINARY
lipoVuoFramework.commands += && rm *.dylib
lipoVuoFramework.target = $$VUO_FRAMEWORK_BINARY
lipoVuoFramework.depends = libVuo32.dylib
POST_TARGETDEPS += $$VUO_FRAMEWORK_BINARY
QMAKE_EXTRA_TARGETS += lipoVuoFramework


# Copy additional headers for 32-bit to Vuo.framework
HEADERS_DEST_DIR = $$ROOT/framework/Vuo.framework/Versions/$$VUO_VERSION/Headers
copyHeaders.commands = cp $$HEADERS $${HEADERS_DEST_DIR}
copyHeaders.target = $${HEADERS_DEST_DIR}/Vuo32.h
copyHeaders.depends = $$HEADERS
POST_TARGETDEPS += $${HEADERS_DEST_DIR}/Vuo32.h
QMAKE_EXTRA_TARGETS += copyHeaders


QMAKE_CLEAN += *.dylib
