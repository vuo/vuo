TEMPLATE = app
CONFIG -= app_bundle
CONFIG += console testcase qtTest VuoFramework TestCompositionExecution
TARGET = TestNodes

include(../../vuo.pri)
include(../test.pri)

# Use Vuo Editor.app's framework, since Jenkins now removes descriptions from the redistributable framework before this test runs.
#QMAKE_RPATHDIR = $$ROOT/framework
QMAKE_LFLAGS -= -Wl,-rpath,$$ROOT/framework
QMAKE_RPATHDIR = "$$ROOT/editor/VuoEditorApp/Vuo Editor.app/Contents/Frameworks"
QMAKE_LFLAGS_RPATH = -rpath$$LITERAL_WHITESPACE

SOURCES += \
	TestNodes.cc
