TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorSizingMode

include(../../../vuo.pri)

SOURCES +=\
		VuoInputEditorSizingMode.cc

HEADERS += \
		VuoInputEditorSizingMode.hh

OTHER_FILES += \
		VuoInputEditorSizingMode.json

INCLUDEPATH += $$ROOT/type
LIBS += $$ROOT/type/VuoSizingMode.o
