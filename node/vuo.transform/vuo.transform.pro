TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.transform.make.c \
	vuo.transform.make.quaternion.c

OTHER_FILES += $$NODE_SOURCES

include(../../module.pri)
