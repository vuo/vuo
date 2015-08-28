TEMPLATE = aux
CONFIG += zmq VuoPCH graphviz json

VUO_INFO_PLIST = VuoCompositionLoader-Info.plist
VUO_INFO_PLIST_GENERATED = VuoCompositionLoader-Info-generated.plist

include(../vuo.pri)

RUNTIME_C_SOURCES += \
	VuoRuntime.c \
	VuoRuntimeMain.c \
	../base/VuoTelemetry.c
RUNTIME_LOADER_SOURCES += \
	VuoCompositionLoader.cc
OTHER_FILES += $$RUNTIME_CXX_SOURCES $$RUNTIME_C_SOURCES $$RUNTIME_LOADER_SOURCES

HEADERS += \
	VuoRuntime.h

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
		$$VUO_VERSION_DEFINES \
		-I$$ROOT/base \
		-c ${QMAKE_FILE_IN} \
		-o ${QMAKE_FILE_OUT}
QMAKE_EXTRA_COMPILERS += runtime_c

runtime_cxx.input = RUNTIME_CXX_SOURCES
runtime_cxx.output = lib${QMAKE_FILE_IN_BASE}.dylib
runtime_cxx.depends = ${QMAKE_PCH_OUTPUT}
runtime_cxx.commands = \
	$$QMAKE_CXX \
		-Xclang -include-pch -Xclang pch/runtime/c++.pch \
		$(CXXFLAGS) \	# Use $() here to get the variable at make-time because QMAKE_CXXFLAGS doesn't have platform-specific flags yet at this point in qmake-time.
		-I$$ROOT/base \
		-dynamiclib \
		-headerpad_max_install_names \
		-Wl,-no_function_starts \
		-Wl,-no_version_load_command \
		${QMAKE_FILE_IN} \
		-o ${QMAKE_FILE_OUT}
QMAKE_EXTRA_COMPILERS += runtime_cxx

runtime_loader.input = RUNTIME_LOADER_SOURCES
runtime_loader.output = ${QMAKE_FILE_IN_BASE}
runtime_loader.depends = ${QMAKE_PCH_OUTPUT} $$ROOT/base/VuoTelemetry.o $$VUO_INFO_PLIST_GENERATED
runtime_loader.commands = \
	$$QMAKE_CXX \
		-Xclang -include-pch -Xclang $$ROOT/runtime/pch/runtime/c++.pch \
		$(CXXFLAGS) \	# Use $() here to get the variable at make-time because QMAKE_CXXFLAGS doesn't have platform-specific flags yet at this point in qmake-time.
		-I$$ROOT/base \
		$$ROOT/base/VuoTelemetry.o \
		$${ZMQ_ROOT}/lib/libzmq.a \
		-lobjc \
		-framework CoreFoundation \
		-framework Foundation \
		-Wl,-rpath,@loader_path/../../../../.. \
		-Wl,-sectcreate,__TEXT,__info_plist,$$VUO_INFO_PLIST_GENERATED \
		${QMAKE_FILE_IN} \
		-o ${QMAKE_FILE_OUT}
QMAKE_EXTRA_COMPILERS += runtime_loader
