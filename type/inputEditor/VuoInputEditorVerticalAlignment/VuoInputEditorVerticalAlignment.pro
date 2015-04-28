TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorVerticalAlignment

include(../../../vuo.pri)

SOURCES +=\
		VuoInputEditorVerticalAlignment.cc

HEADERS += \
		VuoInputEditorVerticalAlignment.hh

OTHER_FILES += \
		VuoInputEditorVerticalAlignment.json

LIBS += $$ROOT/type/VuoVerticalAlignment.o
