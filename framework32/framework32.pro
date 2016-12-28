TEMPLATE = aux
CONFIG -= qt
CONFIG += VuoPCH Vuo32 zmq

include(../vuo.pri)

FLAGS32 = -m32
QMAKE_CFLAGS += $$FLAGS32
QMAKE_CXXFLAGS += $$FLAGS32
QMAKE_LFLAGS += $$FLAGS32

ZMQ32_ROOT = /usr/local/Cellar/zeromq/2.2.0-32
JSONC32_ROOT = /usr/local/Cellar/json-c/0.12-32

DEFINES += ZMQ
INCLUDEPATH += \
	$$ZMQ32_ROOT/include \
	$$JSONC32_ROOT/include

INCLUDEPATH += \
	$$ROOT/base \
	$$ROOT/library \
	$$ROOT/runtime \
	$$ROOT/type

SOURCES = \
	VuoTypeStubs.c \
	$$ROOT/base/VuoBase.cc \
	$$ROOT/base/VuoCable.cc \
	$$ROOT/base/VuoComposition.cc \
	$$ROOT/base/VuoFileUtilities.cc \
	$$ROOT/base/VuoModule.cc \
	$$ROOT/base/VuoNode.cc \
	$$ROOT/base/VuoNodeClass.cc \
	$$ROOT/base/VuoNodeSet.cc \
	$$ROOT/base/VuoPort.cc \
	$$ROOT/base/VuoPortClass.cc \
	$$ROOT/base/VuoProtocol.cc \
	$$ROOT/base/VuoPublishedPort.cc \
	$$ROOT/base/VuoRunner.cc \
	$$ROOT/base/VuoStringUtilities.cc \
	$$ROOT/base/VuoTelemetry.c \
	$$ROOT/base/VuoType.cc \
	$$ROOT/base/miniz.c \
	$$ROOT/library/VuoHeap.cc

OBJECTIVE_SOURCES += \
	$$ROOT/base/VuoFileUtilitiesCocoa.mm \
	$$ROOT/base/VuoRunnerCocoa.mm \
	$$ROOT/base/VuoRunnerCocoa+Conversion.mm \
	$$ROOT/runtime/VuoEventLoop.m

INCLUDEPATH += \
	../compiler \
	../framework \
	../library \
	../node \
	$$system(ls -1d ../node/*/) \
	../runtime \
	../type \
	../type/list

# Build type and library object files
TYPE_AND_LIBRARY_SOURCES = \
	$$ROOT/type/VuoBoolean.c \
	$$ROOT/type/VuoColor.c \
	$$ROOT/type/VuoImage.c \
	$$ROOT/type/VuoImageColorDepth.c \
	$$ROOT/type/VuoInteger.c \
	$$ROOT/type/VuoMesh.c \
	$$ROOT/type/VuoPoint2d.c \
	$$ROOT/type/VuoPoint3d.c \
	$$ROOT/type/VuoReal.c \
	$$ROOT/type/VuoShader.cc \
	$$ROOT/type/VuoText.c \
	$$ROOT/type/VuoTransform.c \
	$$ROOT/type/list/VuoList_VuoBoolean.cc \
	$$ROOT/type/list/VuoList_VuoColor.cc \
	$$ROOT/type/list/VuoList_VuoImage.cc \
	$$ROOT/type/list/VuoList_VuoImageColorDepth.cc \
	$$ROOT/type/list/VuoList_VuoInteger.cc \
	$$ROOT/type/list/VuoList_VuoPoint2d.cc \
	$$ROOT/type/list/VuoList_VuoPoint3d.cc \
	$$ROOT/type/list/VuoList_VuoReal.cc \
	$$ROOT/type/list/VuoList_VuoText.cc \
	$$ROOT/runtime/VuoLog.cc \
	$$ROOT/library/VuoGlContext.cc \
	$$ROOT/library/VuoGlPool.cc \
	$$ROOT/library/VuoImageRenderer.cc
TYPE_AND_LIBRARY_FLAGS = \
	$$QMAKE_CFLAGS \
	-I$$ROOT/library \
	-I$$ROOT/library/shader \
	-I$$ROOT/node \
	-I$$ROOT/node/vuo.data \
	-I$$ROOT/node/vuo.font \
	-I$$ROOT/node/vuo.ui \
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
type.depend_command = $$QMAKE_CC -nostdinc -MM -MF - -MG $$TYPE_AND_LIBRARY_FLAGS ${QMAKE_FILE_NAME} | sed \"s,^.*: ,,\"
typeAndLibraryObjects.CONFIG = target_predeps
QMAKE_EXTRA_COMPILERS += typeAndLibraryObjects


# Build 32-bit dynamic library for Vuo.framework

VUO_FRAMEWORK_SOURCES = $$SOURCES $$OBJECTIVE_SOURCES $$TYPE_AND_LIBRARY_SOURCES
for(sourcefile, VUO_FRAMEWORK_SOURCES) {
	objectfile = $${basename(sourcefile)}
	objectfile ~= s/\\.cc?$/.o
	objectfile ~= s/\\.mm?$/.o
	VUO_FRAMEWORK_OBJECTS += $$objectfile
}

# Make sure it can run in older OS X versions.
QMAKE_LFLAGS += -mmacosx-version-min=$$QMAKE_MACOSX_DEPLOYMENT_TARGET

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
		$${JSONC32_ROOT}/lib/libjson-c.a \
		$${ZMQ32_ROOT}/lib/libzmq.a \
		-framework CoreFoundation \
		-framework OpenGL \
		-framework IOSurface \
		-framework CoreVideo \
		-framework QuartzCore \
		-framework AppKit \
		-lobjc \
		-o libVuo32.dylib
linkVuoFramework.target = libVuo32.dylib
linkVuoFramework.depends = $$VUO_FRAMEWORK_OBJECTS
POST_TARGETDEPS += libVuo32.dylib
QMAKE_EXTRA_TARGETS += linkVuoFramework


# Combine 32-bit dynamic library with existing 64-bit dynamic library for Vuo.framework
VUO_FRAMEWORK_BINARY = $$ROOT/framework/$$VUO_FRAMEWORK_BINARY_RELATIVE
lipoVuoFramework.commands = cp $$VUO_FRAMEWORK_BINARY libVuo64.dylib
lipoVuoFramework.commands += && ( \
	lipo -remove i386 libVuo64.dylib -output libVuo64.dylib \
		2>&1 \
		| grep -v "'^fatal error: .*must be a fat file when the -remove option is specified'" \
	|| true )
lipoVuoFramework.commands += && lipo -create libVuo32.dylib libVuo64.dylib -output libVuoUniversal.dylib
lipoVuoFramework.commands += && cp libVuoUniversal.dylib $$VUO_FRAMEWORK_BINARY
lipoVuoFramework.commands += && rm *.dylib
lipoVuoFramework.target = $$VUO_FRAMEWORK_BINARY
lipoVuoFramework.depends = libVuo32.dylib
POST_TARGETDEPS += $$VUO_FRAMEWORK_BINARY
QMAKE_EXTRA_TARGETS += lipoVuoFramework


QMAKE_CLEAN += *.dylib
