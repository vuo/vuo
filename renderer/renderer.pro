TEMPLATE = lib
CONFIG += staticlib graphviz qtCore qtGui VuoLLVM VuoBase VuoCompiler zmq
TARGET = VuoRenderer

include(../vuo.pri)

HEADERS += \
	VuoRendererPort.hh \
	VuoRendererNode.hh \
	VuoRendererCable.hh \
	VuoRendererComposition.hh \
	VuoRendererItem.hh \
	VuoRendererPortList.hh \
	VuoRendererTypecastPort.hh \
	VuoRendererSignaler.hh \
	VuoRendererColors.hh \
	VuoRendererFonts.hh \
	VuoRendererPublishedPort.hh \
	VuoRendererMakeListNode.hh

SOURCES += \
	VuoRendererPort.cc \
	VuoRendererNode.cc \
	VuoRendererCable.cc \
	VuoRendererComposition.cc \
	VuoRendererItem.cc \
	VuoRendererPortList.cc \
	VuoRendererTypecastPort.cc \
	VuoRendererSignaler.cc \
	VuoRendererColors.cc \
	VuoRendererFonts.cc \
	VuoRendererPublishedPort.cc \
	VuoRendererMakeListNode.cc
