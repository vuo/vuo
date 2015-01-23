TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

GENERIC_NODE_SOURCES += \
	vuo.noise.gradient.c

NODE_LIBRARY_SOURCES += \
	VuoGradientNoiseCommon.c

# OTHER_FILES instead of HEADERS, to avoid including in Vuo.framework
OTHER_FILES += \
	VuoGradientNoiseCommon.h

TYPE_SOURCES += \
	VuoGradientNoise.c \
	VuoNoise.c

HEADERS += \
	VuoGradientNoise.h \
	VuoNoise.h

include(../../module.pri)
