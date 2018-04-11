TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.app.launch.c \
	vuo.app.openDocument.c \
	vuo.app.openUrl.c \
	vuo.app.stopComposition.c

NODE_LIBRARY_SOURCES += \
	VuoAppLaunch.m

HEADERS += \
	VuoAppLaunch.h

include(../../module.pri)
