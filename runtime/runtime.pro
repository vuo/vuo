TEMPLATE = lib
CONFIG -= qt
CONFIG += no_link target_predeps staticlib zmq VuoPCH graphviz json VuoNoLibrary

include(../vuo.pri)

RUNTIME_CXX_SOURCES += \
	VuoHeap.cc
RUNTIME_C_SOURCES += \
	VuoRuntime.c \
	VuoRuntimeMain.c \
	../base/VuoTelemetry.c
RUNTIME_LOADER_SOURCES += \
	VuoCompositionLoader.cc
OTHER_FILES += $$RUNTIME_CXX_SOURCES $$RUNTIME_C_SOURCES $$RUNTIME_LOADER_SOURCES

HEADERS += \
	VuoHeap.h \
	VuoRuntime.h

QMAKE_CLEAN += *.bc VuoCompositionLoader

runtime_c.input = RUNTIME_C_SOURCES
runtime_c.output = lib${QMAKE_FILE_IN_BASE}.bc
runtime_c.depends = ${QMAKE_PCH_OUTPUT}
runtime_c.commands = \
	$$QMAKE_CC \
		-Xclang -include-pch -Xclang pch/runtime/c.pch \
		-emit-llvm \
		$(CFLAGS) \	# Use $() here to get the variable at make-time because QMAKE_CFLAGS doesn't have platform-specific flags yet at this point in qmake-time.
		$(INCPATH) \
		-I$$ROOT/base \
		-c ${QMAKE_FILE_IN} \
		-o ${QMAKE_FILE_OUT}
QMAKE_EXTRA_COMPILERS += runtime_c

runtime_cxx.input = RUNTIME_CXX_SOURCES
runtime_cxx.output = lib${QMAKE_FILE_IN_BASE}.bc
runtime_cxx.depends = ${QMAKE_PCH_OUTPUT}
runtime_cxx.commands = \
	$$QMAKE_CXX \
		-Xclang -include-pch -Xclang pch/runtime/c++.pch \
		-emit-llvm \
		$(CXXFLAGS) \	# Use $() here to get the variable at make-time because QMAKE_CXXFLAGS doesn't have platform-specific flags yet at this point in qmake-time.
		-I$$ROOT/base \
		-c ${QMAKE_FILE_IN} \
		-o ${QMAKE_FILE_OUT}
QMAKE_EXTRA_COMPILERS += runtime_cxx

runtime_loader.input = RUNTIME_LOADER_SOURCES
runtime_loader.output = ${QMAKE_FILE_IN_BASE}
runtime_loader.depends = ${QMAKE_PCH_OUTPUT}
runtime_loader.commands = \
	$$QMAKE_CXX \
		-Xclang -include-pch -Xclang $$ROOT/runtime/pch/runtime/c++.pch \
		$(CXXFLAGS) \	# Use $() here to get the variable at make-time because QMAKE_CXXFLAGS doesn't have platform-specific flags yet at this point in qmake-time.
		-I$$ROOT/base \
		$$ROOT/base/VuoTelemetry.o \
		$${ZMQ_ROOT}/lib/libzmq.a \
		-framework CoreFoundation \
		-Wl,-rpath,@loader_path/../../../../.. \
		${QMAKE_FILE_IN} \
		-o ${QMAKE_FILE_OUT}
QMAKE_EXTRA_COMPILERS += runtime_loader
