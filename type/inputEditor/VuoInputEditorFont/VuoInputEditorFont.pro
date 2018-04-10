TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorFont

include(../../../vuo.pri)

OBJECTIVE_SOURCES += \
	VuoInputEditorFont.mm

HEADERS += \
	VuoInputEditorFont.hh

OTHER_FILES += \
		VuoInputEditorFont.json

LIBS += \
	$$ROOT/type/VuoFont.o \
	-Wl,-undefined,dynamic_lookup
