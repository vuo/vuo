TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

GENERIC_NODE_SOURCES += \
	vuo.motion.crossfade.c \
	vuo.motion.curve.c \
	vuo.motion.smooth.duration.c \
	vuo.motion.smooth.inertia.c \
	vuo.motion.smooth.rate.c \
	vuo.motion.smooth.spring.c

NODE_SOURCES += \
	vuo.motion.wave.c

include(../../module.pri)
