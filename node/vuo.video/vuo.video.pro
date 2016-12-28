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
	VuoVideo.cc \
	VuoFfmpegDecoder.cc \
	VuoAvDecoder.cc \
	VuoVideoPlayer.cc \
	VuoVideoDecoder.cc \
	VuoAvPlayerObject.m \
	VuoAvPlayerInterface.m \
	VuoAvWriter.mm \
	VuoQTCapture.mm \
	VuoAvWriterObject.m \
	VuoQtListener.m

SOURCES += \
	VuoVideo.cc \
	VuoFfmpegDecoder.cc \
	VuoAvDecoder.cc \
	VuoVideoPlayer.cc \
	VuoVideoDecoder.cc

OBJECTIVE_SOURCES += \
	VuoAvPlayerObject.m \
	VuoAvPlayerInterface.m

HEADERS += \
	VuoAvWriter.h \
	VuoAvWriterObject.h \
	VuoAvPlayerInterface.h \
	VuoVideo.h \
	VuoFfmpegDecoder.h \
	VuoFfmpegUtility.h \
	VuoAvDecoder.h \
	VuoAvPlayerObject.h \
	VuoVideoPlayer.h \
	VuoVideoDecoder.h \
	VuoQTCapture.h \
	VuoQtListener.h

HEADERS += \
	VuoAudioFrame.h \
	VuoVideoFrame.h \
	VuoMovieImageEncoding.h \
	VuoVideoOptimization.h \
	VuoAudioEncoding.h \
	VuoMovieFormat.h \
	VuoVideoInputDevice.h

TYPE_SOURCES += \
	VuoAudioFrame.c \
	VuoVideoFrame.c \
	VuoMovieImageEncoding.c \
	VuoVideoOptimization.c \
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
