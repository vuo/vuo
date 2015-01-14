TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)


NODE_SOURCES += \
	vuo.noise.gradient.1d.c \
	vuo.noise.gradient.2d.c \
	vuo.noise.gradient.3d.c \
	vuo.noise.gradient.4d.c

OTHER_FILES += $$NODE_SOURCES


NODE_LIBRARY_SOURCES += \
	VuoGradientNoiseCommon.c

OTHER_FILES += $$NODE_LIBRARY_SOURCES

OTHER_FILES += \
	VuoGradientNoiseCommon.h


TYPE_SOURCES += \
	VuoGradientNoise.c \
	VuoNoise.c

OTHER_FILES += $$TYPE_SOURCES

HEADERS += \
	VuoGradientNoise.h \
	VuoNoise.h


include(../../module.pri)
