TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.kinect.receive.c

NODE_INCLUDEPATH = \
	$${LIBFREENECT_ROOT}/include

include(../../module.pri)
