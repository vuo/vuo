VuoCompileLibraries(
	VuoMouse.m
)
target_include_directories(vuo.mouse.libraries
	PRIVATE
	../vuo.ui
)

VuoCompileTypes(
	VuoMouseButton.c
)

VuoCompileNodes(
	vuo.mouse.button.c
	vuo.mouse.button2.c
	vuo.mouse.click.c
	vuo.mouse.click2.c
	vuo.mouse.delta.c
	vuo.mouse.delta2.c
	vuo.mouse.drag.c
	vuo.mouse.drag2.c
	vuo.mouse.move.c
	vuo.mouse.move2.c
	vuo.mouse.scroll.c
	vuo.mouse.scroll2.c
	vuo.mouse.status.c
	vuo.mouse.status2.c
	vuo.mouse.touch.c
)

VuoNodeSet()
