VuoCompileLibraries(
	VuoSyphon.m
	VuoSyphonListener.m
	VuoSyphonSender.m
	VuoSyphonServerNotifier.m
)
target_include_directories(vuo.syphon.libraries
    PRIVATE
		${CONAN_FRAMEWORK_DIRS_SYPHON}/Syphon.framework/Headers
)
target_link_libraries(vuo.syphon.libraries
	PUBLIC
		CONAN_PKG::syphon
		"-F${CONAN_FRAMEWORK_DIRS_SYPHON}"
		VuoApp
		VuoGlContext
		VuoGlPool
)

VuoCompileTypes(
	VuoSyphonServerDescription.c
)

VuoCompileNodes(
	vuo.syphon.find.server.name.c
	vuo.syphon.find.server.name2.c
	vuo.syphon.find.server.app.c
	vuo.syphon.find.server.app2.c
	vuo.syphon.listServers.c
	vuo.syphon.get.serverDescription.c
	vuo.syphon.make.serverDescription.c
	vuo.syphon.make.serverDescription2.c
	vuo.syphon.send.c
	vuo.syphon.receive.c
)

VuoNodeSet()
