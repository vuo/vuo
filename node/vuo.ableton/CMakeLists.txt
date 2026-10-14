VuoCompileNodes(
	vuo.ableton.link.c
)
target_link_libraries(vuo.ableton.nodes
	PRIVATE
		CONAN_PKG::link
)

VuoNodeSet()
