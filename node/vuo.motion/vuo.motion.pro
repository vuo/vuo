TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

GENERIC_NODE_SOURCES += \
	vuo.motion.curve.c \
	vuo.motion.smooth.duration.c \
	vuo.motion.smooth.inertia.c \
	vuo.motion.smooth.rate.c \
	vuo.motion.spring.c

NODE_SOURCES += \
	vuo.motion.wave.c

include(../../module.pri)
