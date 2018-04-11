TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorTextComparison

include(../../../vuo.pri)

SOURCES +=\
	VuoInputEditorTextComparison.cc

HEADERS += \
	VuoInputEditorTextComparison.hh

OTHER_FILES += \
	VuoInputEditorTextComparison.json

LIBS += $$ROOT/type/VuoTextComparison.o \
	-Wl,-undefined,dynamic_lookup
