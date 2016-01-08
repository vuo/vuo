TEMPLATE = aux
cache()

NODE_SOURCES += \
	example.stateless.vuoize.c

OTHER_FILES += $$NODE_SOURCES

VUO_FRAMEWORK_PATH = ../../../framework
VUO_USER_MODULES_PATH = ~/Library/Application\ Support/Vuo/Modules
QMAKE_PRE_LINK += mkdir -p "$${VUO_USER_MODULES_PATH}"

node.input = NODE_SOURCES
node.output = ${QMAKE_FILE_IN_BASE}.vuonode
node.commands = $${VUO_FRAMEWORK_PATH}/vuo-compile --output ${QMAKE_FILE_OUT} ${QMAKE_FILE_IN} \
	&& mkdir -p "$${VUO_USER_MODULES_PATH}" \
	&& cp ${QMAKE_FILE_OUT} "$${VUO_USER_MODULES_PATH}"
QMAKE_EXTRA_COMPILERS += node

QMAKE_CLEAN = *.vuonode
