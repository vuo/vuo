TEMPLATE = app
CONFIG -= qt
cache()

SOURCES += \
	RunImageFilter.cc

OTHER_FILES += \
	RippleImage.vuo

VUO_FRAMEWORK_PATH=../../../framework

QMAKE_CXX = $$VUO_FRAMEWORK_PATH/Vuo.framework/Frameworks/llvm.framework/Helpers/clang++
QMAKE_MAC_SDK.$$basename(QMAKESPEC).$${QMAKE_MAC_SDK}.QMAKE_CXX = $$QMAKE_CXX
QMAKE_LINK = $$QMAKE_CXX
QMAKE_MAC_SDK.$$basename(QMAKESPEC).$${QMAKE_MAC_SDK}.QMAKE_LINK = $$QMAKE_LINK
QMAKE_CXXFLAGS += -F $$VUO_FRAMEWORK_PATH -DEXAMPLE_PATH=\\\"$$system(pwd)\\\"
QMAKE_LFLAGS += -F $$VUO_FRAMEWORK_PATH -mmacosx-version-min=10.7 -Xlinker -no_function_starts -Xlinker -no_version_load_command \
	$$VUO_FRAMEWORK_PATH/Vuo.framework/Modules/libVuoHeap.dylib \
	$$VUO_FRAMEWORK_PATH/Vuo.framework/Modules/libVuoGlContext.dylib

QMAKE_RPATHDIR = $$VUO_FRAMEWORK_PATH
QMAKE_LFLAGS_RPATH = -rpath$$LITERAL_WHITESPACE

QMAKE_CLEAN = -r $${TARGET}.app

# Rebuild this example when Vuo.framework's headers change.
DEPENDPATH += $$VUO_FRAMEWORK_PATH/Vuo.framework/Headers

GLFW_ROOT = /usr/local/Cellar/glfw2/2.7.9/
INCLUDEPATH += $${GLFW_ROOT}/include
LIBS += -framework Vuo -L$${GLFW_ROOT}/lib -lglfw -framework OpenGL

QMAKE_POST_LINK += install_name_tool -change "/usr/local/lib/libglfw.dylib" "$${GLFW_ROOT}/lib/libglfw.dylib" "RunImageFilter-GLFW.app/Contents/MacOS/RunImageFilter-GLFW"

# In an app you plan to distribute, you should copy the Vuo framework to $${TARGET}.app/Contents/Frameworks,
# and change QMAKE_RPATHDIR to $${TARGET}.app/Contents/Frameworks.
