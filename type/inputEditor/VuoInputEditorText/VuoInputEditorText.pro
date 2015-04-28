TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorText

include(../../../vuo.pri)

SOURCES +=\
		VuoInputEditorText.cc

HEADERS += \
		VuoInputEditorText.hh

OTHER_FILES += \
		VuoInputEditorText.json

LIBS += $$ROOT/type/VuoText.o \
		-Wl,-undefined,dynamic_lookup
