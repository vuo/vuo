TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)


NODE_SOURCES += \
	vuo.leap.c \
	vuo.leap.get.frame.c \
	vuo.leap.get.hand.c \
	vuo.leap.get.pointable.c \
	vuo.leap.filter.hand.id.c \
	vuo.leap.filter.pointable.type.c \
	vuo.leap.filter.pointable.id.c \
	vuo.leap.hand.sort.distance.c \
	vuo.leap.hand.sort.distance.x.c \
	vuo.leap.hand.sort.distance.y.c \
	vuo.leap.hand.sort.distance.z.c \
	vuo.leap.pointable.sort.distance.c \
	vuo.leap.pointable.sort.distance.x.c \
	vuo.leap.pointable.sort.distance.y.c \
	vuo.leap.pointable.sort.distance.z.c

OTHER_FILES += $$NODE_SOURCES


NODE_LIBRARY_SOURCES += \
	VuoLeap.cc

OTHER_FILES += $$NODE_LIBRARY_SOURCES

OTHER_FILES += \
	VuoLeap.h

NODE_LIBRARY_INCLUDEPATH = \
	Leap


TYPE_SOURCES += \
	VuoLeapFrame.c \
	VuoLeapHand.c \
	VuoLeapPointable.c \
	VuoLeapPointableType.c

OTHER_FILES += $$TYPE_SOURCES

HEADERS += \
	VuoLeapFrame.h \
	VuoLeapHand.h \
	VuoLeapPointable.h \
	VuoLeapPointableType.h


include(../../module.pri)
