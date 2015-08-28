TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.dictionary.get.multiple.VuoText.VuoReal.c \
	vuo.dictionary.make.VuoText.VuoReal.c

include(../../module.pri)
