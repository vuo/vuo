TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.audio.analyze.loudness.c \
	vuo.audio.analyze.fft.c \
	vuo.audio.detectBeats.cc \
	vuo.audio.file.info.c \
	vuo.audio.file.play.c \
	vuo.audio.get.input.c \
	vuo.audio.get.output.c \
	vuo.audio.image.channels.c \
	vuo.audio.image.waveform.c \
	vuo.audio.listDevices.c \
	vuo.audio.loudness.c \
	vuo.audio.make.input.id.c \
	vuo.audio.make.input.name.c \
	vuo.audio.make.output.id.c \
	vuo.audio.make.output.name.c \
	vuo.audio.mix.c \
	vuo.audio.populated.c \
	vuo.audio.receive.c \
	vuo.audio.send.c \
	vuo.audio.split.frequency.cc \
	vuo.audio.wave.cc

NODE_INCLUDEPATH += \
	$${GAMMA_ROOT}/include \
	../vuo.motion

NODE_LIBRARY_SOURCES += \
	VuoBeatDetektor.cc \
	VuoDsp.mm \
	VuoAudio.cc \
	VuoAudioFile.c

HEADERS += \
	VuoBeatDetektor.hh \
	VuoDsp.h \
	VuoAudio.h \
	VuoAudioFile.h

NODE_LIBRARY_INCLUDEPATH = \
	$${GAMMA_ROOT}/include \
	$${RTAUDIO_ROOT}/include

TYPE_SOURCES += \
	VuoAudioBins.c \
	VuoAudioBinAverageType.c \
	VuoAudioInputDevice.c \
	VuoAudioOutputDevice.c \
	VuoTempoRange.c

HEADERS += \
	VuoAudioBins.h \
	VuoAudioBinAverageType.h \
	VuoAudioInputDevice.h \
	VuoAudioOutputDevice.h \
	VuoTempoRange.h

include(../../module.pri)
