TEMPLATE = lib
CONFIG += plugin
TARGET = ExampleLanguageInputEditor
cache()


VUO_FRAMEWORK_PATH = ../../../framework
VUO_USER_MODULES_PATH = ~/Library/Application\ Support/Vuo/Modules
QT_ROOT = /usr/local/Cellar/qt/5.3.1

QMAKE_CC = $$VUO_FRAMEWORK_PATH/Vuo.framework/Frameworks/llvm.framework/Helpers/clang
QMAKE_MAC_SDK.$$basename(QMAKESPEC).$${QMAKE_MAC_SDK}.QMAKE_CC = $$QMAKE_CC
QMAKE_CXX = $$VUO_FRAMEWORK_PATH/Vuo.framework/Frameworks/llvm.framework/Helpers/clang++
QMAKE_MAC_SDK.$$basename(QMAKESPEC).$${QMAKE_MAC_SDK}.QMAKE_CXX = $$QMAKE_CXX
QMAKE_LINK = $$QMAKE_CXX
QMAKE_MAC_SDK.$$basename(QMAKESPEC).$${QMAKE_MAC_SDK}.QMAKE_LINK = $$QMAKE_LINK

CONFIG += precompile_header clang_pch_style
PRECOMPILED_HEADER = $$VUO_FRAMEWORK_PATH/resources/inputEditorWidgets/widget.pch
PRECOMPILED_DIR = pch
QMAKE_PCH_OUTPUT_EXT = .pch
QMAKE_CFLAGS_PRECOMPILE =
QMAKE_OBJCFLAGS_PRECOMPILE =
QMAKE_CXXFLAGS_USE_PRECOMPILE = -Xclang -include-pch -Xclang ${QMAKE_PCH_OUTPUT}
QMAKE_CLEAN += -r pch


### Compile the node class ###

NODE_SOURCES += \
	example.customType.greet.c

OTHER_FILES += $$NODE_SOURCES

node.input = NODE_SOURCES
node.output = ${QMAKE_FILE_IN_BASE}.vuonode
node.commands = $${VUO_FRAMEWORK_PATH}/vuo-compile --output ${QMAKE_FILE_OUT} ${QMAKE_FILE_IN}
node.variable_out = NODE_BITCODE
node.CONFIG = target_predeps
QMAKE_EXTRA_COMPILERS += node

QMAKE_CLEAN += *.vuonode


### Compile the port type ###

TYPE_SOURCES += \
	ExampleLanguage.c \
	VuoList_ExampleLanguage.cc

OTHER_FILES += $$TYPE_SOURCES

HEADERS += \
	ExampleLanguage.h \
	VuoList_ExampleLanguage.h

type.input = TYPE_SOURCES
type.output = ${QMAKE_FILE_IN_BASE}.bc
type.commands = $${VUO_FRAMEWORK_PATH}/vuo-compile --output ${QMAKE_FILE_OUT} ${QMAKE_FILE_IN}
type.variable_out = TYPE_BITCODE
type.CONFIG = target_predeps
QMAKE_EXTRA_COMPILERS += type

QMAKE_CLEAN += *.bc


### Package the node class and port type into a node set ###

NODE_SET = $$basename(_PRO_FILE_)
NODE_SET = $$replace(NODE_SET,.pro,)
NODE_SET_ZIP = $${NODE_SET}.vuonode

NODE_OBJECTS = $$NODE_SOURCES
NODE_OBJECTS ~= s/\\.cc?$/.vuonode/g

TYPE_OBJECTS = $$TYPE_SOURCES
TYPE_OBJECTS ~= s/\\.cc?$/.bc/g

NODE_SET_ZIP_CONTENTS = \
	$$NODE_OBJECTS \
	$$TYPE_OBJECTS \
	$$HEADERS

createNodeSetZip.commands = \
	( [ -f $$NODE_SET_ZIP ] && rm $$NODE_SET_ZIP ) ; \
	zip --quiet $$NODE_SET_ZIP $$NODE_SET_ZIP_CONTENTS && \
	mkdir -p "$${VUO_USER_MODULES_PATH}" && \
	cp $$NODE_SET_ZIP "$${VUO_USER_MODULES_PATH}"
createNodeSetZip.depends = $$NODE_SET_ZIP_CONTENTS
createNodeSetZip.target = $$NODE_SET_ZIP
POST_TARGETDEPS += $$NODE_SET_ZIP
QMAKE_EXTRA_TARGETS += createNodeSetZip

QMAKE_CLEAN += $$NODE_SET_ZIP


### Compile the port type to a .o file, to be linked into the input editor ###

typeObjects.input = TYPE_BITCODE
typeObjects.output = ${QMAKE_FILE_IN_BASE}.o
typeObjects.commands = $$QMAKE_CC -c -o ${QMAKE_FILE_OUT} ${QMAKE_FILE_IN}
typeObjects.CONFIG = target_predeps
QMAKE_EXTRA_COMPILERS += typeObjects



### Build the input editor ###

SOURCES += \
	ExampleLanguageInputEditor.cc

HEADERS += \
	ExampleLanguageInputEditor.hh

OTHER_FILES += \
	ExampleLanguageInputEditor.json

QMAKE_CXXFLAGS += \
	-F$$VUO_FRAMEWORK_PATH \
	-F$$VUO_FRAMEWORK_PATH/Vuo.framework/Frameworks
INCLUDEPATH += \
	$$VUO_FRAMEWORK_PATH/Vuo.framework/Headers \
	$$VUO_FRAMEWORK_PATH/resources/inputEditorWidgets
LIBS += \
	-L$$VUO_FRAMEWORK_PATH/resources/inputEditorWidgets -lwidget \
	-L$$VUO_FRAMEWORK_PATH/Vuo.framework/Modules -lVuoHeap \
	-F$$VUO_FRAMEWORK_PATH -framework Vuo \
	-framework QtCore \
	-framework QtGui \
	-framework QtWidgets
QMAKE_LFLAGS += -Wl,-no_function_starts -Wl,-no_version_load_command

QMAKE_POST_LINK = \
	install_name_tool -change "$$QT_ROOT/lib/QtCore.framework/Versions/$$QT_MAJOR_VERSION/QtCore" "@rpath/QtCore.framework/QtCore" lib$${TARGET}.dylib && \
	install_name_tool -change "$$QT_ROOT/lib/QtGui.framework/Versions/$$QT_MAJOR_VERSION/QtGui" "@rpath/QtGui.framework/QtGui" lib$${TARGET}.dylib && \
	install_name_tool -change "$$QT_ROOT/lib/QtWidgets.framework/Versions/$$QT_MAJOR_VERSION/QtWidgets" "@rpath/QtWidgets.framework/QtWidgets" lib$${TARGET}.dylib && \
	mkdir -p "$${VUO_USER_MODULES_PATH}" && \
	cp lib$${TARGET}.dylib "$${VUO_USER_MODULES_PATH}"

QMAKE_CLEAN += lib$${TARGET}.dylib
