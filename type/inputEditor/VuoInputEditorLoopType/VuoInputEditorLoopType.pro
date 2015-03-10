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

INCLUDEPATH += $$ROOT/node/vuo.movie
LIBS += $$ROOT/node/vuo.movie/VuoLoopType.o
