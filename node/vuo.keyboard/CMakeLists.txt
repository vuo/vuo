VuoCompileLibraries(
	VuoKeyboard.m
)
target_sources(vuo.keyboard.libraries PRIVATE
	VuoKeyboard.h
)

VuoCompileTypes(
	VuoKey.c
)
target_sources(vuo.keyboard.types PRIVATE
	VuoKey.h
)
target_link_libraries(vuo.keyboard.types
	PUBLIC
	"-framework Carbon"
	VuoApp
)

VuoCompileCompoundTypes(
	VuoList_VuoKey.cc
)

VuoCompileNodes(
	vuo.keyboard.button.c
	vuo.keyboard.button2.c
	vuo.keyboard.type.c
	vuo.keyboard.type2.c
)

VuoNodeSet()
