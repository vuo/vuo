VuoCompileTypes(
	VuoFileType.c
)
target_sources(vuo.file.types PRIVATE
	VuoFileType.h
)

VuoCompileCompoundTypes(
	# List types needed by targets compiled with Clang rather than the Vuo compiler
	VuoList_VuoFileType.cc
)

VuoCompileNodes(
	vuo.file.create.folder.c
	vuo.file.exists.c
	vuo.file.list.c
)

VuoNodeSet()
