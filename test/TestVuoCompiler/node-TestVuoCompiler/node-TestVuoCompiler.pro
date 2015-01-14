TEMPLATE = lib
CONFIG += no_link target_predeps staticlib VuoNoLibrary

include(../../../vuo.pri)

NODE_SOURCES += \
	vuo.test.compatibleWith106.c \
	vuo.test.compatibleWith107And108.c \
	vuo.test.compatibleWith107AndUp.c \
	vuo.test.inputDataPortOrder.c \
	vuo.test.keywords.c \
	vuo.test.triggerCarryingFloat.c \
	vuo.test.triggerCarryingInteger.c \
	vuo.test.triggerCarryingPoint3d.c \
	vuo.test.triggerCarryingReal.c \
	vuo.test.triggerWithOutput.c \
	vuo.test.unicodeDefaultString.c

HEADERS += \
	VuoTestFloat.h

TYPE_SOURCES += \
	VuoTestFloat.c

OTHER_FILES += $$NODE_SOURCES
OTHER_FILES += $$TYPE_SOURCES

type.input = TYPE_SOURCES
type.output = ${QMAKE_FILE_IN_BASE}.bc
type.commands = $$VUOCOMPILE --header-search-path $$ROOT/type --header-search-path $$ROOT/node --header-search-path $$ROOT/runtime --header-search-path $${ICU_ROOT}/include --header-search-path $${JSONC_ROOT}/include --output ${QMAKE_FILE_OUT} ${QMAKE_FILE_IN}
type.variable_out = TYPE_OBJECTS
type.CONFIG = target_predeps
QMAKE_EXTRA_COMPILERS += type
