TEMPLATE = app
CONFIG -= app_bundle
CONFIG += console testcase graphviz VuoLLVM qtTest VuoBase VuoRuntime VuoCompiler TestVuoCompiler
TARGET = TestVuoCompilerBitcodeGenerator

include(../../vuo.pri)

SOURCES += \
	TestVuoCompilerBitcodeGenerator.cc
