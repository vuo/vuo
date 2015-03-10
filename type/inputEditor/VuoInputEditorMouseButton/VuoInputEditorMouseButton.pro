TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorMouseButton

include(../../../vuo.pri)

SOURCES +=\
		VuoInputEditorMouseButton.cc

HEADERS += \
		VuoInputEditorMouseButton.hh

OTHER_FILES += \
		VuoInputEditorMouseButton.json

INCLUDEPATH += $$ROOT/node/vuo.mouse
LIBS += $$ROOT/node/vuo.mouse/VuoMouseButton.o
