VuoCompileLibraries(
	VuoAudio.cc
	VuoAudioFile.c
	VuoBeatDetektor.cc
	VuoDsp.mm
)
target_sources(vuo.audio.libraries PRIVATE
	VuoAudio.h
	VuoAudioFile.h
	VuoBeatDetektor.hh
	VuoDsp.h
)
target_link_libraries(vuo.audio.libraries
	INTERFACE
	"-framework CoreAudio"
	VuoApp
	vuo.core.libraries

	PUBLIC
		CONAN_PKG::rtaudio
)

VuoCompileTypes(
	VuoAudioBinAverageType.c
	VuoAudioBins.c
	VuoAudioInputDevice.c
	VuoAudioOutputDevice.c
	VuoSpeechVoice.m
	VuoTempoRange.c
)
target_sources(vuo.audio.types PRIVATE
	VuoAudioBinAverageType.h
	VuoAudioBins.h
	VuoAudioInputDevice.h
	VuoAudioOutputDevice.h
	VuoSpeechVoice.h
	VuoTempoRange.h
)

VuoCompileCompoundTypes(
	# List types needed by targets compiled with Clang rather than the Vuo compiler
	VuoList_VuoSpeechVoice.cc

	# Additional list types referenced by headers in this node set
	VuoList_VuoAudioBinAverageType.cc
	VuoList_VuoAudioBins.cc
	VuoList_VuoAudioInputDevice.cc
	VuoList_VuoAudioOutputDevice.cc
	VuoList_VuoTempoRange.cc
)

VuoCompileNodes(
	vuo.audio.analyze.loudness.c
	vuo.audio.analyze.fft.c
	vuo.audio.analyze.fft2.c
	vuo.audio.bitCrush.c
	vuo.audio.detectBeats.cc
	vuo.audio.file.info.c
	vuo.audio.file.play.c
	vuo.audio.fireAtBufferRate.c
	vuo.audio.get.input.c
	vuo.audio.get.output.c
	vuo.audio.image.channels.c
	vuo.audio.image.waveform.c
	vuo.audio.image.waveform2.c
	vuo.audio.listDevices.c
	vuo.audio.listDevices2.c
	vuo.audio.loudness.c
	vuo.audio.make.input.id.c
	vuo.audio.make.input.model.c
	vuo.audio.make.input.name.c
	vuo.audio.make.output.id.c
	vuo.audio.make.output.model.c
	vuo.audio.make.output.name.c
	vuo.audio.mix.c
	vuo.audio.populated.c
	vuo.audio.receive.c
	vuo.audio.ringModulate.c
	vuo.audio.send.c
	vuo.audio.send2.c
	vuo.audio.speak.m
	vuo.audio.split.frequency.cc
	vuo.audio.wave.cc
	vuo.audio.wave2.cc
)
target_link_libraries(vuo.audio.nodes
	PRIVATE
		CONAN_PKG::gamma
)

VuoNodeSet()
