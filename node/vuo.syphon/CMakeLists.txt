VuoCompileLibraries(
	VuoSyphon.m
	VuoSyphonListener.m
	VuoSyphonSender.m
	VuoSyphonServerNotifier.m
)
target_include_directories(vuo.syphon.libraries
	PRIVATE
	Syphon/Syphon.framework/Headers
)
target_link_libraries(vuo.syphon.libraries
	PUBLIC
	"-F${CMAKE_CURRENT_SOURCE_DIR}/Syphon"
	"-framework Syphon"
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
