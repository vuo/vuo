TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

TYPE_INCLUDEPATH += \
	blackmagic-decklink-sdk \
	$$ROOT/node/vuo.video

TYPE_SOURCES += \
	VuoDeinterlacing.c \
	VuoBlackmagicInputDevice.c \
	VuoBlackmagicOutputDevice.c \
	VuoBlackmagicConnection.cc \
	VuoBlackmagicVideoMode.cc

HEADERS += \
	VuoDeinterlacing.h \
	VuoBlackmagicInputDevice.h \
	VuoBlackmagicOutputDevice.h \
	VuoBlackmagicConnection.h \
	VuoBlackmagicVideoMode.h

include(../../module.pri)
