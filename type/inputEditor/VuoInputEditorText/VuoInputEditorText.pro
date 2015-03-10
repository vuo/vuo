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

INCLUDEPATH += $$ROOT/runtime
LIBS += $$ROOT/type/VuoText.o \
	$$ICU_ROOT/lib/libicuuc.a \
	$$ICU_ROOT/lib/libicudata.a \
	-Wl,-undefined,dynamic_lookup
