TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorDisplacement

include(../../../vuo.pri)

SOURCES +=\
		VuoInputEditorDisplacement.cc

HEADERS += \
		VuoInputEditorDisplacement.hh

OTHER_FILES += \
		VuoInputEditorDisplacement.json

INCLUDEPATH += \
	$$ROOT/type \
	$$ROOT/node/vuo.scene

LIBS += \
	$$ROOT/library/libVuoHeap.dylib \
	$$ROOT/node/vuo.scene/VuoDisplacement.o
