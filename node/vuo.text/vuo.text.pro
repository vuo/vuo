TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.text.append.c \
	vuo.text.areEqual.c \
	vuo.text.countCharacters.c \
	vuo.text.cut.c

OTHER_FILES += $$NODE_SOURCES

include(../../module.pri)
