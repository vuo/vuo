TEMPLATE = lib
CONFIG -= qt
CONFIG += staticlib VuoPCH VuoRuntime graphviz json discount
TARGET = VuoBase

include(../vuo.pri)

SOURCES += \
	VuoBase.cc \
	VuoBaseDetail.cc \
	VuoNodeClass.cc \
	VuoNode.cc \
	VuoNodeSet.cc \
	VuoPort.cc \
	VuoPortClass.cc \
	VuoCable.cc \
	VuoRunner.cc \
	VuoFileUtilities.cc \
	VuoTelemetry.c \
	VuoStringUtilities.cc \
	VuoPublishedPort.cc \
	VuoModule.cc \
	VuoType.cc \
	VuoGenericType.cc \
	VuoTimeUtilities.cc \
	VuoCompositionStub.c \
	VuoComposition.cc \
	miniz.c

HEADERS += \
	VuoBase.hh \
	VuoBaseDetail.hh \
	VuoNodeClass.hh \
	VuoNode.hh \
	VuoNodeSet.hh \
	VuoPort.hh \
	VuoPortClass.hh \
	VuoCable.hh \
	VuoRunner.hh \
	VuoFileUtilities.hh \
	VuoTelemetry.h \
	VuoStringUtilities.hh \
	VuoPublishedPort.hh \
	VuoModule.hh \
	VuoType.hh \
	VuoGenericType.hh \
	VuoTimeUtilities.hh \
	VuoComposition.hh \
	miniz.h

BASE_STUB_SOURCES += \
	VuoCompositionStub.c

base_stub.input = BASE_STUB_SOURCES
base_stub.output = ${QMAKE_FILE_IN_BASE}.dylib
base_stub.depends = ${QMAKE_PCH_OUTPUT}
base_stub.variable_out = BASE_STUB_OBJECTS  # Don't try to add VuoCompositionStub.dylib to libVuoBase.a ...
base_stub.CONFIG = target_predeps  # ... but still build this target.
base_stub.commands = \
	$$QMAKE_CC \
		-Xclang -include-pch -Xclang pch/VuoBase/c.pch \
		$(CFLAGS) \	# Use $() here to get the variable at make-time because QMAKE_CFLAGS doesn't have platform-specific flags yet at this point in qmake-time.
		-dynamiclib \
		-Xlinker -no_function_starts -Xlinker -no_version_load_command \
		${QMAKE_FILE_IN} \
		-o ${QMAKE_FILE_OUT}
QMAKE_EXTRA_COMPILERS += base_stub
