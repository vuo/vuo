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

INCLUDEPATH += \
	$$ROOT/node/vuo.font

LIBS += \
	$$ROOT/node/vuo.font/VuoFont.o \
	-Wl,-undefined,dynamic_lookup
