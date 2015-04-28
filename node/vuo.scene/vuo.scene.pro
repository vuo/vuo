TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.scene.combine.c \
	vuo.scene.copy.c \
	vuo.scene.copy.trs.c \
	vuo.scene.get.c \
	vuo.scene.make.c \
	vuo.scene.make.camera.orthographic.c \
	vuo.scene.make.camera.orthographic.target.c \
	vuo.scene.make.camera.perspective.c \
	vuo.scene.make.camera.perspective.target.c \
	vuo.scene.make.cube.c \
	vuo.scene.make.image.c \
	vuo.scene.make.image.unlit.c \
	vuo.scene.make.light.ambient.c \
	vuo.scene.make.light.point.c \
	vuo.scene.make.light.spot.c \
	vuo.scene.make.light.spot.target.c \
	vuo.scene.render.image.c \
	vuo.scene.render.window.c


# NODE_LIBRARY_SOURCES += \
# 	VuoPbMesh.c

# OTHER_FILES instead of HEADERS, to avoid including in Vuo.framework
#OTHER_FILES += \
#	VuoPbMesh.h

NODE_INCLUDEPATH += \
	$${ASSIMP_ROOT}/include

include(../../module.pri)
