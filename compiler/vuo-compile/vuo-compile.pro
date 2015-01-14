TEMPLATE = app
CONFIG -= qt app_bundle
CONFIG += console graphviz VuoLLVM VuoBase VuoRuntime VuoCompiler

include(../../vuo.pri)

SOURCES += \
	vuo-compile.cc
