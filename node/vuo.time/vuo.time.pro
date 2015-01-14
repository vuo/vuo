TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.time.firePeriodically.c

OTHER_FILES += $$NODE_SOURCES

include(../../module.pri)
