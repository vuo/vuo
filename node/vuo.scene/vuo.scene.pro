TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.scene.combine.c \
	vuo.scene.frameRequest.get.frameCount.c \
	vuo.scene.frameRequest.get.timestamp.c \
	vuo.scene.get.c \
	vuo.scene.make.c \
	vuo.scene.make.cube.c \
	vuo.scene.make.image.c \
	vuo.scene.render.image.c \
	vuo.scene.render.window.c

OTHER_FILES += $$NODE_SOURCES

NODE_INCLUDEPATH += \
	$${ASSIMP_ROOT}/include

include(../../module.pri)
