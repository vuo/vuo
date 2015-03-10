TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorBoolean

include(../../../vuo.pri)

SOURCES +=\
		VuoInputEditorBoolean.cc

HEADERS += \
		VuoInputEditorBoolean.hh

OTHER_FILES += \
		VuoInputEditorBoolean.json

LIBS += $$ROOT/type/VuoBoolean.o
