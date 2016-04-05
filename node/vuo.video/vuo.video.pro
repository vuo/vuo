TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.video.decodeImage.c \
	vuo.video.info.c \
	vuo.video.listDevices.c \
	vuo.video.make.format.c \
	vuo.video.make.input.c \
	vuo.video.play.c \
	vuo.video.receive.c \
	vuo.video.save.c

NODE_LIBRARY_SOURCES += \
	VuoMovie.cc \
	VuoAvWriter.mm \
	VuoQTCapture.mm \
	VuoAvWriterObject.m \
	VuoQtListener.m

SOURCES += \
	VuoMovie.cc

HEADERS += \
	VuoAvWriter.h \
	VuoAvWriterObject.h \
	VuoMovie.h \
	VuoQTCapture.h \
	VuoQtListener.h

HEADERS += \
	VuoVideoFrame.h \
	VuoMovieImageEncoding.h \
	VuoAudioEncoding.h \
	VuoMovieFormat.h \
	VuoVideoInputDevice.h

TYPE_SOURCES += \
	VuoVideoFrame.c \
	VuoMovieImageEncoding.c \
	VuoAudioEncoding.c \
	VuoMovieFormat.c \
	VuoVideoInputDevice.c

NODE_LIBRARY_INCLUDEPATH = \
	../vuo.font \
	$${FFMPEG_ROOT}/include \
	$${FFMPEG_ROOT}/include/libavcodec \
	$${FFMPEG_ROOT}/include/libavformat \
	$${FFMPEG_ROOT}/include/libavutil \
	$${FFMPEG_ROOT}/include/libswscale

include(../../module.pri)
