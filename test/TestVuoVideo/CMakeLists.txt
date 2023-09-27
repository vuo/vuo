VuoTest(NAME TestVuoVideo
	SOURCE TestVuoVideo.cc
	TIMEOUT 180
)
target_link_libraries(TestVuoVideo
	PRIVATE
		Qt5::Widgets
		vuo.video.libraries
		vuo.video.types
)
