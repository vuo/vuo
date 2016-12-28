include(../framework/masterLists.pri)

TEMPLATE = lib
CONFIG -= qt
CONFIG += staticlib VuoPCH VuoRuntime graphviz json discount openssl
TARGET = VuoBase

include(../vuo.pri)
exists($$ROOT/licensetools/licensetools.pro) {
	include($$ROOT/licensetools/licensetools.pro)
	QMAKE_OBJECTIVE_CFLAGS += -DENCRYPTED_DATE=\\\"$${ENCRYPTED_DATE}\\\"
}
include (base.pri)

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
		$$QMAKE_LFLAGS \
		${QMAKE_FILE_IN} \
		-o ${QMAKE_FILE_OUT}
coverage {
	base_stub.commands += && install_name_tool -change @executable_path/../lib/libprofile_rt.dylib $$LLVM_ROOT/lib/libprofile_rt.dylib ${QMAKE_FILE_OUT}
}
QMAKE_EXTRA_COMPILERS += base_stub

createMasterHeader.commands += (cd ../framework ; ./generateFrameworkHeader.pl $${FRAMEWORK_VUO_STUB_HEADER} $${MASTER_VUO_HEADER_LIST} > Vuo.h)
createMasterHeader.depends += ../framework/$$FRAMEWORK_VUO_STUB_HEADER
createMasterHeader.target = ../framework/Vuo.h
POST_TARGETDEPS += ../framework/Vuo.h
QMAKE_EXTRA_TARGETS += createMasterHeader

createCoreTypesHeader.commands = (cd ../type ; ./generateCoreTypesHeader.sh $$TYPE_HEADERS)
createCoreTypesHeader.depends = ../type/type.pro ../type/list/list.pro
createCoreTypesHeader.target = ../type/coreTypes.h
POST_TARGETDEPS += ../type/coreTypes.h
QMAKE_EXTRA_TARGETS += createCoreTypesHeader
QMAKE_CLEAN += ../type/coreTypes.h ../type/coreTypesStringify.h ../type/coreTypesStringify.hh

QMAKE_OBJECTIVE_CFLAGS += \
	-I../type \
	-I../type/list \
	-I../library
