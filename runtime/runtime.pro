TEMPLATE = aux
CONFIG += zmq VuoPCH VuoPCH_objc json

VUO_INFO_PLIST = VuoCompositionLoader-Info.plist
VUO_INFO_PLIST_GENERATED = VuoCompositionLoader-Info-generated.plist

include(../vuo.pri)

RUNTIME_CXX_SOURCES += \
	VuoCompositionDiff.cc \
	VuoLog.cc \
	VuoNodeRegistry.cc \
	VuoRuntime.cc \
	VuoRuntimeCommunicator.cc \
	VuoRuntimePersistentState.cc \
	VuoRuntimeState.cc \
	VuoRuntimeUtilities.cc \
	VuoThreadManager.cc
RUNTIME_OBJC_SOURCES += \
	VuoEventLoop.m
RUNTIME_C_SOURCES += \
	VuoCompositionState.c \
	VuoRuntimeContext.c \
	VuoRuntimeMain.c \
	../base/VuoTelemetry.c
RUNTIME_LOADER_SOURCES += \
	VuoCompositionLoader.cc
OTHER_FILES += $$RUNTIME_CXX_SOURCES $$RUNTIME_C_SOURCES $$RUNTIME_OBJC_SOURCES $$RUNTIME_LOADER_SOURCES

HEADERS += \
	VuoCompositionDiff.hh \
	VuoCompositionState.h \
	VuoEventLoop.h \
	VuoNodeRegistry.hh \
	VuoRuntime.h \
	VuoRuntimeCommunicator.hh \
	VuoRuntimeContext.h \
	VuoRuntimePersistentState.hh \
	VuoRuntimeState.hh \
	VuoRuntimeUtilities.hh \
	VuoThreadManager.hh

QMAKE_CFLAGS += \
	-I$$ROOT/library \
	-I$$ROOT/node \
	-I$$ROOT/type \
	-I$$ROOT/type/list

QMAKE_CLEAN += VuoCompositionLoader

runtime_c.input = RUNTIME_C_SOURCES
runtime_c.output = lib${QMAKE_FILE_IN_BASE}.bc
runtime_c.depends = ${QMAKE_PCH_OUTPUT}
runtime_c.commands = \
	$$QMAKE_CC \
		-Xclang -include-pch -Xclang pch/runtime/c.pch \
		-emit-llvm \
		$(CFLAGS) \	# Use $() here to get the variable at make-time because QMAKE_CFLAGS doesn't have platform-specific flags yet at this point in qmake-time.
		$(INCPATH) \
		-I$$ROOT/node \
		$$VUO_VERSION_DEFINES \
		-I$$ROOT/base \
		-c ${QMAKE_FILE_IN} \
		-o ${QMAKE_FILE_OUT}
QMAKE_EXTRA_COMPILERS += runtime_c

runtime_objc.input = RUNTIME_OBJC_SOURCES
runtime_objc.output = lib${QMAKE_FILE_IN_BASE}.bc
runtime_objc.depends = ${QMAKE_PCH_OUTPUT} pch/runtime/objective-c.pch
runtime_objc.commands = \
	$$QMAKE_CC \
		-Xclang -include-pch -Xclang pch/runtime/objective-c.pch \
		-emit-llvm \
		$(CFLAGS) \	# Use $() here to get the variable at make-time because QMAKE_CFLAGS doesn't have platform-specific flags yet at this point in qmake-time.
		$(INCPATH) \
		-I$$ROOT/node \
		$$VUO_VERSION_DEFINES \
		-I$$ROOT/base \
		-c ${QMAKE_FILE_IN} \
		-o ${QMAKE_FILE_OUT}
QMAKE_EXTRA_COMPILERS += runtime_objc

runtime_cxx.input = RUNTIME_CXX_SOURCES
runtime_cxx.output = lib${QMAKE_FILE_IN_BASE}.bc
runtime_cxx.depends = ${QMAKE_PCH_OUTPUT}
runtime_cxx.commands = \
	$$QMAKE_CXX \
		-Xclang -include-pch -Xclang pch/runtime/c++.pch \
		-emit-llvm \
		$(CXXFLAGS) \	# Use $() here to get the variable at make-time because QMAKE_CXXFLAGS doesn't have platform-specific flags yet at this point in qmake-time.
		$(INCPATH) \
		-I$$ROOT/node \
		$$VUO_VERSION_DEFINES \
		-I$$ROOT/base \
		-c ${QMAKE_FILE_IN} \
		-o ${QMAKE_FILE_OUT}
QMAKE_EXTRA_COMPILERS += runtime_cxx

runtime_loader.input = RUNTIME_LOADER_SOURCES
runtime_loader.output = ${QMAKE_FILE_IN_BASE}
runtime_loader.depends = ${QMAKE_PCH_OUTPUT} $$ROOT/base/VuoTelemetry.o $$VUO_INFO_PLIST_GENERATED VuoEventLoop.o libVuoLog.bc
runtime_loader.commands = \
	$$QMAKE_CXX \
		-Xclang -include-pch -Xclang pch/runtime/c++.pch \
		$(CXXFLAGS) \	# Use $() here to get the variable at make-time because QMAKE_CXXFLAGS doesn't have platform-specific flags yet at this point in qmake-time.
		-I$$ROOT/base \
		$$ROOT/base/VuoTelemetry.o \
		VuoEventLoop.o \
		libVuoLog.bc \
		$${ZMQ_ROOT}/lib/libzmq.a \
		-lobjc \
		-framework AppKit \
		-Wl,-rpath,@loader_path/../../../../.. \
		-Wl,-sectcreate,__TEXT,__info_plist,$$VUO_INFO_PLIST_GENERATED \
		-Wl,-exported_symbol,_VuoApp_mainThread \
		-Wl,-exported_symbol,_VuoApp_dylibPath \
		$$QMAKE_LFLAGS \
		${QMAKE_FILE_IN} \
		-o ${QMAKE_FILE_OUT}
QMAKE_EXTRA_COMPILERS += runtime_loader
