TEMPLATE = aux

include(../../../vuo.pri)

NODE_SOURCES += \
	vuo.test.compatibleWith106.c \
	vuo.test.compatibleWith107And108.c \
	vuo.test.compatibleWith107AndUp.c \
	vuo.test.inputDataPortOrder.c \
	vuo.test.keywords.c \
	vuo.test.triggerCarryingFloat.c \
	vuo.test.triggerCarryingInteger.c \
	vuo.test.triggerCarryingPoint3d.c \
	vuo.test.triggerCarryingReal.c \
	vuo.test.triggerWithOutput.c \
	vuo.test.unicodeDefaultString.c

HEADERS += \
	VuoTestFloat.h

TYPE_SOURCES += \
	VuoTestFloat.c

include(../../../module.pri)
