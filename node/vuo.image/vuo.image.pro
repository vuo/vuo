TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.image.apply.mask.c \
	vuo.image.blend.c \
	vuo.image.blur.c \
	vuo.image.bulge.c \
	vuo.image.color.adjust.c \
	vuo.image.color.combine.rgb.c \
	vuo.image.color.invert.c \
	vuo.image.color.map.c \
	vuo.image.color.mask.brightness.c \
	vuo.image.color.offset.radial.rgb.c \
	vuo.image.color.offset.rgb.c \
	vuo.image.color.split.rgb.c \
	vuo.image.crop.c \
	vuo.image.crop.pixels.c \
	vuo.image.feedback.c \
	vuo.image.fetch.c \
	vuo.image.fetch.list.c \
	vuo.image.findBarcode.c \
	vuo.image.flip.horizontal.c \
	vuo.image.flip.vertical.c \
	vuo.image.frost.c \
	vuo.image.sample.color.c \
	vuo.image.get.size.c \
	vuo.image.kaleidoscope.c \
	vuo.image.make.checkerboard.c \
	vuo.image.make.color.c \
	vuo.image.make.gradient.linear.c \
	vuo.image.make.gradient.radial.c \
	vuo.image.make.noise.c \
	vuo.image.make.random.c \
	vuo.image.make.shadertoy.c \
	vuo.image.make.text.c \
#	vuo.image.make.triangle.c \
	vuo.image.mirror.c \
	vuo.image.pixellate.c \
#	vuo.image.pixellate.details.c \
	vuo.image.populated.c \
	vuo.image.posterize.c \
	vuo.image.render.window.c \
	vuo.image.resize.c \
	vuo.image.resize.larger.c \
	vuo.image.ripple.c \
	vuo.image.rotate.c \
	vuo.image.save.c \
	vuo.image.tile.c \
	vuo.image.twirl.c \
	vuo.image.vignette.c \
	vuo.image.wrapMode.c

NODE_INCLUDEPATH += \
	../vuo.audio \
	../vuo.font \
	../vuo.layer \
	../vuo.scene \
	$${FREEIMAGE_ROOT}/include

NODE_LIBRARY_SOURCES += \
	VuoBarcode.cc

HEADERS += \
	VuoBarcode.h

NODE_LIBRARY_INCLUDEPATH += \
	$$ZXING_ROOT/include

TYPE_SOURCES += \
	VuoHorizontalReflection.c \
	VuoVerticalReflection.c \
	VuoImageFormat.c \
	VuoSizingMode.c \
	VuoImageStereoType.c \
	VuoThresholdType.c

HEADERS += \
	VuoHorizontalReflection.h \
	VuoVerticalReflection.h \
	VuoImageFormat.h \
	VuoImageStereoType.h \
	VuoSizingMode.h \
	VuoThresholdType.h

include(../../module.pri)
