VuoCompileLibraries(
	VuoKeyboard.m
)

VuoCompileTypes(
	VuoKey.c
)
target_link_libraries(vuo.keyboard.types
	PUBLIC
	"-framework Carbon"
	VuoApp
)

VuoCompileNodes(
	vuo.keyboard.button.c
	vuo.keyboard.button2.c
	vuo.keyboard.type.c
	vuo.keyboard.type2.c
)

VuoNodeSet()
