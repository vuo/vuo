TEMPLATE = app
CONFIG -= qt app_bundle
CONFIG += console VuoFramework
TARGET = vuo-link

include(../../vuo.pri)

SOURCES += \
	../vuo-link/vuo-link.cc

LIBS += -rpath @loader_path/../../../..

QMAKE_POST_LINK += cp vuo-link $$ROOT/framework/Vuo.framework/Versions/$$VUO_VERSION/MacOS
