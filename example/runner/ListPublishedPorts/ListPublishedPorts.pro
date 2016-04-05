TEMPLATE = app
CONFIG -= qt app_bundle
CONFIG += console
cache()

SOURCES += \
	ListPublishedPorts.cc

VUO_FRAMEWORK_PATH=../../../framework

QMAKE_CXX = $$VUO_FRAMEWORK_PATH/Vuo.framework/Frameworks/llvm.framework/Helpers/clang++
QMAKE_MAC_SDK.$$basename(QMAKESPEC).$${QMAKE_MAC_SDK}.QMAKE_CXX = $$QMAKE_CXX
QMAKE_LINK = $$QMAKE_CXX
QMAKE_MAC_SDK.$$basename(QMAKESPEC).$${QMAKE_MAC_SDK}.QMAKE_LINK = $$QMAKE_LINK
QMAKE_CXXFLAGS += -F $$VUO_FRAMEWORK_PATH -DVUO_COMPOSITION_PATH=\\\"$$VUO_COMPOSITION_PATH\\\"
QMAKE_LFLAGS += -F $$VUO_FRAMEWORK_PATH -mmacosx-version-min=10.7
QMAKE_RPATHDIR = $$VUO_FRAMEWORK_PATH
QMAKE_LFLAGS_RPATH = -rpath$$LITERAL_WHITESPACE

QMAKE_CLEAN = $$TARGET

# Rebuild this example when Vuo.framework's headers change.
DEPENDPATH += $$VUO_FRAMEWORK_PATH/Vuo.framework/Headers

LIBS += -framework Vuo

# In an app you plan to distribute, you should copy the Vuo framework to $${TARGET}.app/Contents/Frameworks,
# and change QMAKE_RPATHDIR to $${TARGET}.app/Contents/Frameworks.
