VuoCompileNodes(
	vuo.kinect.receive.c
	vuo.kinect.receive2.c
)
target_link_libraries(vuo.kinect.nodes
	PRIVATE
		CONAN_PKG::libfreenect
)

VuoNodeSet()
