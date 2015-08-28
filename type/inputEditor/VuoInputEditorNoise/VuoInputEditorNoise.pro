TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorNoise

include(../../../vuo.pri)

SOURCES +=\
		VuoInputEditorNoise.cc

HEADERS += \
		VuoInputEditorNoise.hh

OTHER_FILES += \
		VuoInputEditorNoise.json

INCLUDEPATH += $$ROOT/node/vuo.noise
LIBS += \
	$$ROOT/library/libVuoHeap.dylib \
	$$ROOT/type/list/VuoList_VuoNoise.o \
	$$ROOT/node/vuo.noise/VuoNoise.o
