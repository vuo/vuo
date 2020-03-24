VuoCompileNodes(
	vuo.kinect.receive.c
	vuo.kinect.receive2.c
	vuo.kinect.v2.receive.cc
)
target_link_libraries(vuo.kinect.nodes
	PRIVATE
		CONAN_PKG::libfreenect
		CONAN_PKG::libfreenect2
)

VuoNodeSet()
