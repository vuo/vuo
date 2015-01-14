TEMPLATE = lib
CONFIG += staticlib graphviz VuoLLVM zmq qtTest VuoPCH VuoBase VuoCompiler
TARGET = TestVuoCompiler

include(../../vuo.pri)

SOURCES += \
	TestVuoCompiler.cc

HEADERS += \
	TestVuoCompiler.hh

DEFINES += QT_ROOT=\\\"$$QT_ROOT\\\"
