TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorHorizontalAlignment

include(../../../vuo.pri)

SOURCES +=\
		VuoInputEditorHorizontalAlignment.cc

HEADERS += \
		VuoInputEditorHorizontalAlignment.hh

OTHER_FILES += \
		VuoInputEditorHorizontalAlignment.json

LIBS += $$ROOT/type/VuoHorizontalAlignment.o
