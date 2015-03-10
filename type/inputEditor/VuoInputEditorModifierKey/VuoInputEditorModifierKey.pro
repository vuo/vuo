TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorModifierKey

include(../../../vuo.pri)

SOURCES +=\
		VuoInputEditorModifierKey.cc

HEADERS += \
		VuoInputEditorModifierKey.hh

OTHER_FILES += \
		VuoInputEditorModifierKey.json

INCLUDEPATH += $$ROOT/node/vuo.mouse
LIBS += $$ROOT/node/vuo.mouse/VuoModifierKey.o
