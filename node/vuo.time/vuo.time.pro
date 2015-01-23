TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.time.bpm.seconds.c \
	vuo.time.firePeriodically.c \
	vuo.time.measureTime.c \
	vuo.time.seconds.bpm.c

include(../../module.pri)
