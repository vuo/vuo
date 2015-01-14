TEMPLATE = app
CONFIG -= app_bundle
CONFIG += console testcase graphviz VuoLLVM qtTest VuoBase VuoRuntime VuoCompiler TestVuoCompiler
TARGET = TestVuoCompilerType

include(../../vuo.pri)

SOURCES += \
	TestVuoCompilerType.cc
