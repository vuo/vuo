TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.logic.areAllTrue.c \
	vuo.logic.areAnyTrue.c \
	vuo.logic.isOneTrue.c \
	vuo.logic.negate.c \
	vuo.logic.toggle.c

include(../../module.pri)
