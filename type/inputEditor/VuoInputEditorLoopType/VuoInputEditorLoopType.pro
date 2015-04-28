TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorLoopType

include(../../../vuo.pri)

SOURCES +=\
		VuoInputEditorLoopType.cc

HEADERS += \
		VuoInputEditorLoopType.hh

OTHER_FILES += \
		VuoInputEditorLoopType.json

LIBS += $$ROOT/type/VuoLoopType.o
