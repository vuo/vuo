TEMPLATE = lib
CONFIG -= qt
CONFIG += staticlib graphviz VuoLLVM zmq json Carbon VuoPCH VuoBase VuoType
TARGET = VuoCompiler

include(../vuo.pri)
exists($$ROOT/licensetools/licensetools.pro) {include($$ROOT/licensetools/licensetools.pro)}
exists($$ROOT/licensetools/premiumnode.pro) {
   include($$ROOT/licensetools/premiumnode.pro)
   DEFINES += PREMIUM_NODE_LOADER_ENABLED
}

DEFINES += LLVM_ROOT=\\\"$$LLVM_ROOT\\\"
DEFINES += GRAPHVIZ_ROOT=\\\"$$GRAPHVIZ_ROOT\\\"
DEFINES += JSONC_ROOT=\\\"$$JSONC_ROOT\\\"
DEFINES += ZMQ_ROOT=\\\"$$ZMQ_ROOT\\\"
DEFINES += LEAP_ROOT=\\\"$$ROOT/node/vuo.leap/Leap\\\"
DEFINES += MUPARSER_ROOT=\\\"$$MUPARSER_ROOT\\\"
DEFINES += FREEIMAGE_ROOT=\\\"$$FREEIMAGE_ROOT\\\"
DEFINES += CURL_ROOT=\\\"$$CURL_ROOT\\\"
DEFINES += RTMIDI_ROOT=\\\"$$RTMIDI_ROOT\\\"
DEFINES += RTAUDIO_ROOT=\\\"$$RTAUDIO_ROOT\\\"
DEFINES += GAMMA_ROOT=\\\"$$GAMMA_ROOT\\\"
DEFINES += ASSIMP_ROOT=\\\"$$ASSIMP_ROOT\\\"
DEFINES += FFMPEG_ROOT=\\\"$$FFMPEG_ROOT\\\"
DEFINES += LIBUSB_ROOT=\\\"$$LIBUSB_ROOT\\\"
DEFINES += LIBFREENECT_ROOT=\\\"$$LIBFREENECT_ROOT\\\"
DEFINES += OSCPACK_ROOT=\\\"$$OSCPACK_ROOT\\\"
DEFINES += ZXING_ROOT=\\\"$$ZXING_ROOT\\\"
DEFINES += LIBXML2_ROOT=\\\"$$LIBXML2_ROOT\\\"
DEFINES += SYPHON_ROOT=\\\"$$ROOT/node/vuo.syphon/Syphon\\\"

SOURCES += \
	VuoCompilerPortClass.cc \
	VuoCompilerPort.cc \
	VuoCompilerDataClass.cc \
	VuoCompilerData.cc \
	VuoCompilerOutputDataClass.cc \
	VuoCompilerOutputData.cc \
	VuoCompilerEventPortClass.cc \
	VuoCompilerEventPort.cc \
	VuoCompilerOutputEventPortClass.cc \
	VuoCompilerOutputEventPort.cc \
	VuoCompilerNodeClass.cc \
	VuoCompilerNodeArgumentClass.cc \
	VuoCompilerNodeArgument.cc \
	VuoCompilerNode.cc \
	VuoCompilerInstanceDataClass.cc \
	VuoCompilerInstanceData.cc \
	VuoCompilerInputDataClass.cc \
	VuoCompilerInputData.cc \
	VuoCompilerInputEventPortClass.cc \
	VuoCompilerInputEventPort.cc \
	VuoCompilerGraphvizParser.cc \
	VuoCompilerBitcodeGenerator.cc \
	VuoCompilerGraph.cc \
	VuoCompilerTriggerDescription.cc \
	VuoCompilerTriggerPortClass.cc \
	VuoCompilerTriggerPort.cc \
	VuoCompilerChain.cc \
	VuoCompilerBitcodeParser.cc \
	VuoCompiler.cc \
	VuoCompilerCable.cc \
	VuoCompilerPublishedPortClass.cc \
	VuoCompilerPublishedPort.cc \
	VuoCompilerCodeGenUtilities.cc \
	VuoCompilerConstantStringCache.cc \
	VuoCompilerModule.cc \
	VuoCompilerType.cc \
	VuoCompilerGenericType.cc \
	VuoCompilerComposition.cc \
	VuoCompilerMakeListNodeClass.cc \
	VuoCompilerPublishedInputNodeClass.cc \
	VuoCompilerPublishedOutputNodeClass.cc \
	VuoCompilerSpecializedNodeClass.cc \
	VuoCompilerTargetSet.cc \
	VuoCompilerDriver.cc \
	VuoCompilerException.cc

INCLUDEPATH += \
	../framework \
	../library \
	../node \
	$$system(ls -1d ../node/*/) \
	../runtime

HEADERS += \
	VuoCompilerPortClass.hh \
	VuoCompilerPort.hh \
	VuoCompilerDataClass.hh \
	VuoCompilerData.hh \
	VuoCompilerOutputDataClass.hh \
	VuoCompilerOutputData.hh \
	VuoCompilerEventPortClass.hh \
	VuoCompilerEventPort.hh \
	VuoCompilerOutputEventPortClass.hh \
	VuoCompilerOutputEventPort.hh \
	VuoCompilerNodeClass.hh \
	VuoCompilerNodeArgumentClass.hh \
	VuoCompilerNodeArgument.hh \
	VuoCompilerNode.hh \
	VuoCompilerInstanceDataClass.hh \
	VuoCompilerInstanceData.hh \
	VuoCompilerInputDataClass.hh \
	VuoCompilerInputData.hh \
	VuoCompilerInputEventPortClass.hh \
	VuoCompilerInputEventPort.hh \
	VuoCompilerGraphvizParser.hh \
	VuoCompilerBitcodeGenerator.hh \
	VuoCompilerGraph.hh \
	VuoCompilerTriggerDescription.hh \
	VuoCompilerTriggerPortClass.hh \
	VuoCompilerTriggerPort.hh \
	VuoCompilerChain.hh \
	VuoCompilerBitcodeParser.hh \
	VuoCompiler.hh \
	VuoCompilerCable.hh \
	VuoCompilerPublishedPortClass.hh \
	VuoCompilerPublishedPort.hh \
	VuoCompilerCodeGenUtilities.hh \
	VuoCompilerConstantStringCache.hh \
	VuoCompilerModule.hh \
	VuoCompilerType.hh \
	VuoCompilerGenericType.hh \
	VuoCompilerComposition.hh \
	VuoCompilerMakeListNodeClass.hh \
	VuoCompilerPublishedInputNodeClass.hh \
	VuoCompilerPublishedOutputNodeClass.hh \
	VuoCompilerSpecializedNodeClass.hh \
	VuoCompilerTargetSet.hh \
	VuoCompilerDriver.hh \
	VuoCompilerException.hh
