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
	VuoRendererInputListDrawer.hh \
	VuoRendererReadOnlyDictionary.hh \
	VuoRendererKeyListForReadOnlyDictionary.hh \
	VuoRendererValueListForReadOnlyDictionary.hh \
	VuoRendererInputDrawer.hh \
	VuoRendererInputAttachment.hh \
	VuoRendererHiddenInputAttachment.hh

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
	VuoRendererInputListDrawer.cc \
	VuoRendererReadOnlyDictionary.cc \
	VuoRendererKeyListForReadOnlyDictionary.cc \
	VuoRendererValueListForReadOnlyDictionary.cc \
	VuoRendererInputDrawer.cc \
	VuoRendererInputAttachment.cc \
	VuoRendererHiddenInputAttachment.cc
