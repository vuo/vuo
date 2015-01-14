TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.noise.gradient.1d.c \
	vuo.noise.gradient.2d.c \
	vuo.noise.gradient.3d.c \
	vuo.noise.gradient.4d.c

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
