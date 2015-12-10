TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.leap.find.hand.confidence.c \
	vuo.leap.find.hand.id.c \
	vuo.leap.find.hand.side.c \
	vuo.leap.find.pointable.id.c \
	vuo.leap.find.pointable.touchZone.c \
	vuo.leap.find.pointable.type.c \
	vuo.leap.get.frame.c \
	vuo.leap.get.hand.c \
	vuo.leap.get.pointable.c \
	vuo.leap.hand.sort.distance.c \
	vuo.leap.hand.sort.distance.x.c \
	vuo.leap.hand.sort.distance.y.c \
	vuo.leap.hand.sort.distance.z.c \
	vuo.leap.pointable.sort.distance.c \
	vuo.leap.pointable.sort.distance.x.c \
	vuo.leap.pointable.sort.distance.y.c \
	vuo.leap.pointable.sort.distance.z.c \
	vuo.leap.receive.c

NODE_LIBRARY_SOURCES += \
	VuoLeap.cc

HEADERS += \
	VuoLeap.h

NODE_LIBRARY_INCLUDEPATH = \
	Leap

TYPE_SOURCES += \
	VuoHorizontalSide.c \
	VuoLeapFrame.c \
	VuoLeapHand.c \
	VuoLeapPointable.c \
	VuoLeapPointableType.c \
	VuoLeapTouchZone.c

HEADERS += \
	VuoHorizontalSide.h \
	VuoLeapFrame.h \
	VuoLeapHand.h \
	VuoLeapPointable.h \
	VuoLeapPointableType.h \
	VuoLeapTouchZone.h

include(../../module.pri)
