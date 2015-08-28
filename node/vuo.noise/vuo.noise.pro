TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

GENERIC_NODE_SOURCES += \
	vuo.noise.gradient.c

NODE_LIBRARY_SOURCES += \
	VuoGradientNoiseCommon.c

HEADERS += \
	VuoGradientNoiseCommon.h

TYPE_SOURCES += \
	VuoGradientNoise.c \
	VuoNoise.c

HEADERS += \
	VuoGradientNoise.h \
	VuoNoise.h

include(../../module.pri)
