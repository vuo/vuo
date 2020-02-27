VuoTest(NAME TestVuoVideo
	SOURCE TestVuoVideo.cc
	RESOURCE_LOCK COMPILER
	TIMEOUT 1800
)
target_link_libraries(TestVuoVideo
	PRIVATE
	vuo.video.libraries
)
