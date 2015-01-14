TEMPLATE = app
CONFIG -= app_bundle
CONFIG += console testcase graphviz VuoLLVM qtTest VuoBase VuoRuntime VuoCompiler TestVuoCompiler
TARGET = TestVuoCompilerComposition

include(../../vuo.pri)

SOURCES += \
	TestVuoCompilerComposition.cc
