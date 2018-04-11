TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.bcf2000.buttons1.c \
	vuo.bcf2000.buttons2.c \
	vuo.bcf2000.faders.c \
	vuo.bcf2000.foot.c \
	vuo.bcf2000.knobButtons.c \
	vuo.bcf2000.knobs.c \
	vuo.bcf2000.rightButtons.c

NODE_INCLUDEPATH += \
	../vuo.midi

TYPE_SOURCES += \
	VuoRealRegulation.c

NODE_LIBRARY_SOURCES += \
	VuoScribbleStrip.c

HEADERS += \
	VuoRealRegulation.h \
	VuoScribbleStrip.h

include(../../module.pri)
