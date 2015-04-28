TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.movie.decodeImage.c \
	vuo.movie.info.c \
	vuo.movie.play.c

NODE_LIBRARY_SOURCES += \
	VuoMovie.cc

SOURCES += \
	VuoMovie.cc

# OTHER_FILES instead of HEADERS, to avoid including in Vuo.framework
OTHER_FILES += \
	VuoMovie.h

NODE_LIBRARY_INCLUDEPATH = \
	$${FFMPEG_ROOT}/include \
	$${FFMPEG_ROOT}/include/libavcodec \
	$${FFMPEG_ROOT}/include/libavformat \
	$${FFMPEG_ROOT}/include/libavutil \
	$${FFMPEG_ROOT}/include/libswscale

include(../../module.pri)
