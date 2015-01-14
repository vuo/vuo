TEMPLATE = lib
CONFIG -= qt
CONFIG += staticlib graphviz VuoLLVM zmq json Carbon VuoPCH VuoBase
TARGET = VuoCompiler

include(../vuo.pri)

DEFINES += LLVM_ROOT=\\\"$$LLVM_ROOT\\\"
DEFINES += GRAPHVIZ_ROOT=\\\"$$GRAPHVIZ_ROOT\\\"
DEFINES += ICU_ROOT=\\\"$$ICU_ROOT\\\"
DEFINES += JSONC_ROOT=\\\"$$JSONC_ROOT\\\"
DEFINES += ZMQ_ROOT=\\\"$$ZMQ_ROOT\\\"
DEFINES += LEAP_ROOT=\\\"$$ROOT/node/Leap\\\"
DEFINES += MUPARSER_ROOT=\\\"$$MUPARSER_ROOT\\\"
DEFINES += FREEIMAGE_ROOT=\\\"$$FREEIMAGE_ROOT\\\"
DEFINES += FREETYPE_ROOT=\\\"$$FREETYPE_ROOT\\\"
DEFINES += CURL_ROOT=\\\"$$CURL_ROOT\\\"
DEFINES += RTMIDI_ROOT=\\\"$$RTMIDI_ROOT\\\"
DEFINES += ASSIMP_ROOT=\\\"$$ASSIMP_ROOT\\\"
DEFINES += SYPHON_ROOT=\\\"$$ROOT/node/vuo.syphon/Syphon\\\"

SOURCES += \
	VuoCompilerPassiveEdge.cc \
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
	VuoCompilerLeaf.cc \
	VuoCompilerInstanceDataClass.cc \
	VuoCompilerInstanceData.cc \
	VuoCompilerInputDataClass.cc \
	VuoCompilerInputData.cc \
	VuoCompilerInputEventPortClass.cc \
	VuoCompilerInputEventPort.cc \
	VuoCompilerGraphvizParser.cc \
	VuoCompilerBitcodeGenerator.cc \
	VuoCompilerTriggerPortClass.cc \
	VuoCompilerTriggerPort.cc \
	VuoCompilerTriggerEdge.cc \
	VuoCompilerTriggerAction.cc \
	VuoCompilerEdge.cc \
	VuoCompilerDebug.cc \
	VuoCompilerChain.cc \
	VuoCompilerBitcodeParser.cc \
	VuoCompiler.cc \
	VuoCompilerCable.cc \
	VuoCompilerPublishedPort.cc \
	VuoCompilerPublishedInputPort.cc \
	VuoCompilerPublishedOutputPort.cc \
	VuoCompilerCodeGenUtilities.cc \
	VuoCompilerModule.cc \
	VuoCompilerType.cc \
	VuoCompilerComposition.cc \
	VuoCompilerMakeListNodeClass.cc \
	VuoCompilerTargetSet.cc

HEADERS += \
	VuoCompilerPassiveEdge.hh \
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
	VuoCompilerLeaf.hh \
	VuoCompilerInstanceDataClass.hh \
	VuoCompilerInstanceData.hh \
	VuoCompilerInputDataClass.hh \
	VuoCompilerInputData.hh \
	VuoCompilerInputEventPortClass.hh \
	VuoCompilerInputEventPort.hh \
	VuoCompilerGraphvizParser.hh \
	VuoCompilerBitcodeGenerator.hh \
	VuoCompilerTriggerPortClass.hh \
	VuoCompilerTriggerPort.hh \
	VuoCompilerTriggerEdge.hh \
	VuoCompilerTriggerAction.hh \
	VuoCompilerEdge.hh \
	VuoCompilerDebug.hh \
	VuoCompilerChain.hh \
	VuoCompilerBitcodeParser.hh \
	VuoCompiler.hh \
	VuoCompilerCable.hh \
	VuoCompilerPublishedPort.hh \
	VuoCompilerPublishedInputPort.hh \
	VuoCompilerPublishedOutputPort.hh \
	VuoCompilerCodeGenUtilities.hh \
	VuoCompilerModule.hh \
	VuoCompilerType.hh \
	VuoCompilerComposition.hh \
	VuoCompilerMakeListNodeClass.hh \
	VuoCompilerTargetSet.hh
