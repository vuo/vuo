TEMPLATE = aux
cache()

TYPE_SOURCES += \
	MyPairOfReals.c

OTHER_FILES += $$TYPE_SOURCES

HEADERS += \
	MyPairOfReals.h

VUO_FRAMEWORK_PATH = ../../../framework
VUO_USER_MODULES_PATH = ~/Library/Application\ Support/Vuo/Modules

type.input = TYPE_SOURCES
type.output = ${QMAKE_FILE_IN_BASE}.bc
type.commands = $${VUO_FRAMEWORK_PATH}/vuo-compile --output ${QMAKE_FILE_OUT} ${QMAKE_FILE_IN} \
	&& mkdir -p "$${VUO_USER_MODULES_PATH}" \
	&& cp ${QMAKE_FILE_OUT} "$${VUO_USER_MODULES_PATH}"
QMAKE_EXTRA_COMPILERS += type

QMAKE_CLEAN = *.bc
