TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorWrapMode

include(../../../vuo.pri)

SOURCES +=\
		VuoInputEditorWrapMode.cc

HEADERS += \
		VuoInputEditorWrapMode.hh

OTHER_FILES += \
		VuoInputEditorWrapMode.json

INCLUDEPATH += $$ROOT/type
LIBS += $$ROOT/type/VuoWrapMode.o
