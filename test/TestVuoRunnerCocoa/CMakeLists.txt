VuoTest(NAME TestVuoRunnerCocoa
	SOURCE TestVuoRunnerCocoa.mm
	RESOURCE_LOCK COMPILER
)
target_link_libraries(TestVuoRunnerCocoa
	PUBLIC
	"-framework AppKit"
	"-framework CoreVideo"
	"-framework OpenGL"
	"-framework QuartzCore"
)
