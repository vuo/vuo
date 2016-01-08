TEMPLATE = aux

build.commands = xcodebuild
build.depends = VuoPluginApp/*.mm VuoPluginApp/*.hh
build.target = build/Release/VuoPluginApp.app
POST_TARGETDEPS += build/Release/VuoPluginApp.app
QMAKE_EXTRA_TARGETS += build

QMAKE_CLEAN += -R build
