TEMPLATE = app
CONFIG -= qt app_bundle
CONFIG += console VuoFramework
TARGET = vuo-compile

include(../../vuo.pri)

SOURCES += \
	../vuo-compile/vuo-compile.cc

LIBS += -rpath @loader_path/../../../..

QMAKE_POST_LINK += cp vuo-compile $$ROOT/framework/Vuo.framework/Versions/$$VUO_VERSION/MacOS
