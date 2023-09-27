VuoCompileTypes(
	VuoTable.cc
	VuoTableFormat.c
)
target_sources(vuo.table.types PRIVATE
	VuoTable.h
	VuoTableFormat.h
)
target_include_directories(vuo.table.types
	PRIVATE
	../vuo.time
)

VuoCompileCompoundTypes(
	VuoList_VuoTableFormat.cc
)

VuoCompileGenericNodes(
	vuo.table.change.column.c
	vuo.table.change.item.c
	vuo.table.change.row.c
	vuo.table.find.c
	vuo.table.get.column.c
	vuo.table.get.item.c
	vuo.table.get.row.c
	vuo.table.sort.c
)

VuoCompileNodes(
	vuo.table.add.column.c
	vuo.table.add.row.c
	vuo.table.fetch.c
	vuo.table.format.c
	vuo.table.make.text.c
	vuo.table.take.column.c
	vuo.table.take.row.c
	vuo.table.transpose.c
)

VuoNodeSet()

target_link_libraries(vuo.table.types
	PUBLIC
	CONAN_PKG::libcsv
)
