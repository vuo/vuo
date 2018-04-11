TEMPLATE = app
CONFIG -= app_bundle
CONFIG += console testcase qtCore qtGui qtNetwork qtTest VuoLLVM VuoBase VuoEditor VuoFramework VuoInputEditorWidget TestCompositionExecution
TARGET = TestInputEditors

include(../../vuo.pri)

QMAKE_RPATHDIR = $$ROOT/framework
QMAKE_LFLAGS_RPATH = -rpath$$LITERAL_WHITESPACE

SOURCES += TestInputEditors.cc
