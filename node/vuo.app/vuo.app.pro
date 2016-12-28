TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.app.launch.c

NODE_LIBRARY_SOURCES += \
	VuoApp.m

HEADERS += \
	VuoApp.h

include(../../module.pri)
