TEMPLATE = lib
CONFIG += plugin
TARGET = ExampleLanguageInputEditor
cache()


VUO_FRAMEWORK_PATH = ../../../framework
VUO_USER_MODULES_PATH = ~/Library/Application\ Support/Vuo/Modules
QT_ROOT = /usr/local/Cellar/qt/5.3.1


QMAKE_CC = $$VUO_FRAMEWORK_PATH/Vuo.framework/MacOS/Clang/bin/clang
QMAKE_MAC_SDK.$$basename(QMAKESPEC).$${QMAKE_MAC_SDK}.QMAKE_CC = $$QMAKE_CC
QMAKE_CXX = $$VUO_FRAMEWORK_PATH/Vuo.framework/MacOS/Clang/bin/clang++
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


# If you're creating an input editor for node classes and port types defined in a separate Qt project, remove commands starting here...

NODE_SOURCES += \
	example.greet.c

OTHER_FILES += $$NODE_SOURCES

node.input = NODE_SOURCES
node.output = ${QMAKE_FILE_IN_BASE}.vuonode
node.commands = $${VUO_FRAMEWORK_PATH}/vuo-compile --output ${QMAKE_FILE_OUT} ${QMAKE_FILE_IN} \
	&& mkdir -p "$${VUO_USER_MODULES_PATH}" \
	&& cp ${QMAKE_FILE_OUT} "$${VUO_USER_MODULES_PATH}"
node.variable_out = NODE_BITCODE
node.CONFIG = target_predeps
QMAKE_EXTRA_COMPILERS += node

QMAKE_CLEAN += *.vuonode


TYPE_SOURCES += \
	ExampleLanguage.c

OTHER_FILES += $$TYPE_SOURCES

HEADERS += \
	ExampleLanguage.h

type.input = TYPE_SOURCES
type.output = ${QMAKE_FILE_IN_BASE}.bc
type.commands = $${VUO_FRAMEWORK_PATH}/vuo-compile --output ${QMAKE_FILE_OUT} ${QMAKE_FILE_IN} \
	&& mkdir -p "$${VUO_USER_MODULES_PATH}" \
	&& cp ${QMAKE_FILE_OUT} "$${VUO_USER_MODULES_PATH}"
type.variable_out = TYPE_BITCODE
type.CONFIG = target_predeps
QMAKE_EXTRA_COMPILERS += type

# ... and ending here. Also uncomment the lines below, setting TYPE_DIR to the path to your port type's header files and compiled bitcode:
# TYPE_DIR =
# INCLUDEPATH += $$TYPE_DIR
# TYPE_BITCODE = $$TYPE_DIR/ExampleLanguage.bc

typeObjects.input = TYPE_BITCODE
typeObjects.output = ${QMAKE_FILE_IN_BASE}.o
typeObjects.commands = $$QMAKE_CC -c -o ${QMAKE_FILE_OUT} ${QMAKE_FILE_IN}
typeObjects.CONFIG = target_predeps
QMAKE_EXTRA_COMPILERS += typeObjects

QMAKE_CLEAN += *.bc


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
	$$VUO_FRAMEWORK_PATH/resources/inputEditorWidgets
LIBS += \
	-L$$VUO_FRAMEWORK_PATH/resources/inputEditorWidgets -lwidget \
	-F$$VUO_FRAMEWORK_PATH -framework Vuo \
	-framework QtCore \
	-framework QtGui \
	-framework QtWidgets
QMAKE_LFLAGS += -Wl,-no_function_starts -Wl,-no_version_load_command

QMAKE_POST_LINK = \
	install_name_tool -change "$$QT_ROOT/lib/QtCore.framework/Versions/$$QT_MAJOR_VERSION/QtCore" "@rpath/QtCore.framework/QtCore" lib$${TARGET}.dylib && \
	install_name_tool -change "$$QT_ROOT/lib/QtGui.framework/Versions/$$QT_MAJOR_VERSION/QtGui" "@rpath/QtGui.framework/QtGui" lib$${TARGET}.dylib && \
	install_name_tool -change "$$QT_ROOT/lib/QtWidgets.framework/Versions/$$QT_MAJOR_VERSION/QtWidgets" "@rpath/QtWidgets.framework/QtWidgets" lib$${TARGET}.dylib && \
	cp lib$${TARGET}.dylib "$${VUO_USER_MODULES_PATH}"

QMAKE_CLEAN += lib$${TARGET}.dylib
