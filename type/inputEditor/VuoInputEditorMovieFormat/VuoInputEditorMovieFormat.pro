TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorMovieFormat

include(../../../vuo.pri)

SOURCES +=\
	VuoInputEditorMovieFormat.cc

HEADERS += \
	VuoInputEditorMovieFormat.hh

OTHER_FILES += \
	VuoInputEditorMovieFormat.json

INCLUDEPATH += \
	$$ROOT/node/vuo.video

LIBS += \
	$$ROOT/library/libVuoHeap.dylib \
	$$ROOT/node/vuo.video/VuoMovieFormat.o \
	$$ROOT/node/vuo.video/VuoAudioEncoding.o \
	$$ROOT/node/vuo.video/VuoMovieImageEncoding.o \
	$$ROOT/type/VuoInteger.o \
	$$ROOT/type/VuoReal.o \
	$$ROOT/type/list/VuoList_VuoReal.o \
	$$ROOT/type/list/VuoList_VuoAudioEncoding.o \
	$$ROOT/type/list/VuoList_VuoMovieImageEncoding.o \
	$$ROOT/type/VuoText.o
