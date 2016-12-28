TEMPLATE = aux

include(../../../vuo.pri)

NODE_SOURCES += \
	vuo.test.doNothing.c \
	vuo.test.triggerCarryingFloat.c \
	vuo.test.triggerCarryingInteger.c \
	vuo.test.triggerCarryingPoint3d.c \
	vuo.test.triggerCarryingPoint4d.c \
	vuo.test.triggerCarryingReal.c

HEADERS += \
	VuoTestFloat.h

TYPE_SOURCES += \
	VuoTestFloat.c

include(../../../module.pri)
