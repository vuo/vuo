TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)


NODE_SOURCES += \
#	vuo.movie.decodeImage.c \
	vuo.movie.info.c \
	vuo.movie.play.c

OTHER_FILES += $$NODE_SOURCES


NODE_LIBRARY_SOURCES += \
	VuoMovie.cc

OTHER_FILES += $$NODE_LIBRARY_SOURCES

TYPE_SOURCES += \
	VuoLoopType.c

OTHER_FILES += \
	VuoMovie.h

NODE_LIBRARY_INCLUDEPATH = \
	$${FFMPEG_ROOT}/include \
	$${FFMPEG_ROOT}/include/libavcodec \
	$${FFMPEG_ROOT}/include/libavformat \
	$${FFMPEG_ROOT}/include/libavutil \
	$${FFMPEG_ROOT}/include/libswscale


HEADERS += \
	VuoLoopType.h

include(../../module.pri)
