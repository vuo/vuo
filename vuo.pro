TEMPLATE = subdirs
QMAKE_CLEAN = Makefile

SUBDIRS += \
	base \
	base_vuo_debug \
	compiler \
	compiler_vuo_compile \
	compiler_vuo_compile_for_framework \
	compiler_vuo_link \
	framework \
	library \
	node \
	node_color \
	node_console \
	node_event \
	node_hold \
	node_image \
	node_leap \
	node_list \
	node_logic \
	node_math \
	node_midi \
	node_mouse \
	node_movie \
	node_noise \
	node_point \
	node_quaternion \
	node_scene \
	node_select \
	node_shader \
	node_syphon \
	node_text \
	node_time \
	node_transform \
	node_type \
	node_vertices \
	renderer \
	renderer_vuo_export \
	renderer_vuo_render \
	runtime \
	type_list \
	type

OTHER_FILES += \
	module.pri \
	vuo.pri

include(./vuo.pri)

cache()


base_vuo_debug.subdir = base/vuo-debug
base_vuo_debug.depends = framework

compiler.depends = base

compiler_vuo_compile.subdir = compiler/vuo-compile
compiler_vuo_compile.depends = compiler

compiler_vuo_compile_for_framework.subdir = compiler/vuo-compile-for-framework
compiler_vuo_compile_for_framework.depends = framework

compiler_vuo_link.subdir = compiler/vuo-link
compiler_vuo_link.depends = framework

framework.subdir = framework
framework.depends = base compiler renderer runtime node type type_list library \
	node_color \
	node_console \
	node_event \
	node_hold \
	node_image \
	node_leap \
	node_list \
	node_logic \
	node_math \
	node_midi \
	node_mouse \
	node_movie \
	node_noise \
	node_point \
	node_quaternion \
	node_scene \
	node_select \
	node_shader \
	node_syphon \
	node_text \
	node_time \
	node_transform \
	node_type \
	node_vertices

node.depends = compiler_vuo_compile type type_list

node_color.subdir = node/vuo.color
node_color.depends = node

node_console.subdir = node/vuo.console
node_console.depends = node

node_event.subdir = node/vuo.event
node_event.depends = node

node_hold.subdir = node/vuo.hold
node_hold.depends = node

node_image.subdir = node/vuo.image
node_image.depends = node

node_leap.subdir = node/vuo.leap
node_leap.depends = node

node_list.subdir = node/vuo.list
node_list.depends = node

node_logic.subdir = node/vuo.logic
node_logic.depends = node

node_math.subdir = node/vuo.math
node_math.depends = node

node_midi.subdir = node/vuo.midi
node_midi.depends = node

node_mouse.subdir = node/vuo.mouse
node_mouse.depends = node

node_movie.subdir = node/vuo.movie
node_movie.depends = node

node_noise.subdir = node/vuo.noise
node_noise.depends = node

node_point.subdir = node/vuo.point
node_point.depends = node

node_quaternion.subdir = node/vuo.quaternion
node_quaternion.depends = node

node_scene.subdir = node/vuo.scene
node_scene.depends = node

node_select.subdir = node/vuo.select
node_select.depends = node

node_shader.subdir = node/vuo.shader
node_shader.depends = node

node_syphon.subdir = node/vuo.syphon
node_syphon.depends = node

node_text.subdir = node/vuo.text
node_text.depends = node

node_time.subdir = node/vuo.time
node_time.depends = node

node_transform.subdir = node/vuo.transform
node_transform.depends = node

node_type.subdir = node/vuo.type
node_type.depends = node

node_vertices.subdir = node/vuo.vertices
node_vertices.depends = node

renderer.depends = base compiler

renderer_vuo_export.subdir = renderer/vuo-export
renderer_vuo_export.depends = framework

renderer_vuo_render.subdir = renderer/vuo-render
renderer_vuo_render.depends = framework

runtime.depends = base

type.depends = compiler_vuo_compile

type_list.subdir = type/list
type_list.depends = type


exists(editor) {
	SUBDIRS += \
		editor \
		editor_InputEditors_VuoInputEditorBoolean \
		editor_InputEditors_VuoInputEditorBlendMode \
		editor_InputEditors_VuoInputEditorColor \
		editor_InputEditors_VuoInputEditorCountWrapMode \
		editor_InputEditors_VuoInputEditorCurve \
		editor_InputEditors_VuoInputEditorCurveDomain \
		editor_InputEditors_VuoInputEditorGradientNoise \
		editor_InputEditors_VuoInputEditorInteger \
		editor_InputEditors_VuoInputEditorNoise \
		editor_InputEditors_VuoInputEditorPoint2d \
		editor_InputEditors_VuoInputEditorPoint3d \
		editor_InputEditors_VuoInputEditorPoint4d \
		editor_InputEditors_VuoInputEditorPointableType \
		editor_InputEditors_VuoInputEditorReal \
		editor_InputEditors_VuoInputEditorText \
		editor_InputEditors_VuoInputEditorWave \
		editor_InputEditors_VuoInputEditorLoopType \
		editor_VuoEditor

	editor.depends = compiler renderer

	editor_InputEditors_VuoInputEditorBlendMode.subdir = editor/InputEditors/VuoInputEditorBlendMode
	editor_InputEditors_VuoInputEditorBlendMode.depends = editor

	editor_InputEditors_VuoInputEditorBoolean.subdir = editor/InputEditors/VuoInputEditorBoolean
	editor_InputEditors_VuoInputEditorBoolean.depends = editor

	editor_InputEditors_VuoInputEditorColor.subdir = editor/InputEditors/VuoInputEditorColor
	editor_InputEditors_VuoInputEditorColor.depends = editor

	editor_InputEditors_VuoInputEditorCountWrapMode.subdir = editor/InputEditors/VuoInputEditorCountWrapMode
	editor_InputEditors_VuoInputEditorCountWrapMode.depends = editor

	editor_InputEditors_VuoInputEditorCurve.subdir = editor/InputEditors/VuoInputEditorCurve
	editor_InputEditors_VuoInputEditorCurve.depends = editor

	editor_InputEditors_VuoInputEditorCurveDomain.subdir = editor/InputEditors/VuoInputEditorCurveDomain
	editor_InputEditors_VuoInputEditorCurveDomain.depends = editor

	editor_InputEditors_VuoInputEditorGradientNoise.subdir = editor/InputEditors/VuoInputEditorGradientNoise
	editor_InputEditors_VuoInputEditorGradientNoise.depends = editor

	editor_InputEditors_VuoInputEditorInteger.subdir = editor/InputEditors/VuoInputEditorInteger
	editor_InputEditors_VuoInputEditorInteger.depends = editor

	editor_InputEditors_VuoInputEditorNoise.subdir = editor/InputEditors/VuoInputEditorNoise
	editor_InputEditors_VuoInputEditorNoise.depends = editor

	editor_InputEditors_VuoInputEditorPoint2d.subdir = editor/InputEditors/VuoInputEditorPoint2d
	editor_InputEditors_VuoInputEditorPoint2d.depends = editor

	editor_InputEditors_VuoInputEditorPoint3d.subdir = editor/InputEditors/VuoInputEditorPoint3d
	editor_InputEditors_VuoInputEditorPoint3d.depends = editor

	editor_InputEditors_VuoInputEditorPoint4d.subdir = editor/InputEditors/VuoInputEditorPoint4d
	editor_InputEditors_VuoInputEditorPoint4d.depends = editor

	editor_InputEditors_VuoInputEditorPointableType.subdir = editor/InputEditors/VuoInputEditorPointableType
	editor_InputEditors_VuoInputEditorPointableType.depends = editor

	editor_InputEditors_VuoInputEditorReal.subdir = editor/InputEditors/VuoInputEditorReal
	editor_InputEditors_VuoInputEditorReal.depends = editor

	editor_InputEditors_VuoInputEditorText.subdir = editor/InputEditors/VuoInputEditorText
	editor_InputEditors_VuoInputEditorText.depends = editor

	editor_InputEditors_VuoInputEditorWave.subdir = editor/InputEditors/VuoInputEditorWave
	editor_InputEditors_VuoInputEditorWave.depends = editor

	editor_InputEditors_VuoInputEditorLoopType.subdir = editor/InputEditors/VuoInputEditorLoopType
	editor_InputEditors_VuoInputEditorLoopType.depends = editor

	editor_VuoEditor.subdir = editor/VuoEditorApp
	editor_VuoEditor.depends = \
		editor \
		editor_InputEditors_VuoInputEditorBoolean \
		editor_InputEditors_VuoInputEditorBlendMode \
		editor_InputEditors_VuoInputEditorColor \
		editor_InputEditors_VuoInputEditorCountWrapMode \
		editor_InputEditors_VuoInputEditorCurve \
		editor_InputEditors_VuoInputEditorCurveDomain \
		editor_InputEditors_VuoInputEditorGradientNoise \
		editor_InputEditors_VuoInputEditorInteger \
		editor_InputEditors_VuoInputEditorNoise \
		editor_InputEditors_VuoInputEditorPoint2d \
		editor_InputEditors_VuoInputEditorPoint3d \
		editor_InputEditors_VuoInputEditorPoint4d \
		editor_InputEditors_VuoInputEditorPointableType \
		editor_InputEditors_VuoInputEditorReal \
		editor_InputEditors_VuoInputEditorText \
		editor_InputEditors_VuoInputEditorWave \
		editor_InputEditors_VuoInputEditorLoopType \
		framework
}


# Provide "make go" for easily running the editor from the command line
go.commands = (cd "editor/VuoEditorApp/Vuo*.app/Contents/MacOS" ; ./Vuo*)
QMAKE_EXTRA_TARGETS += go

# Provide "make examples" so we don't need to rebuild all the examples during development
DOLLAR = $
examples.commands = (for i in example/api/* example/node/* example/type/* ; do (cd $${DOLLAR}$${DOLLAR}i && ([ -f Makefile ] || qmake) && make -j9); done)
examples.commands += && (for i in node/vuo.*/examples ; do (cd $${DOLLAR}$${DOLLAR}i && ([ -f Makefile ] || qmake) && make -j9); done)
QMAKE_EXTRA_TARGETS += examples

# Provide "make documentation" so we don't need to rebuild the documentation during development
docs.commands = (cd documentation && ([ -f Makefile ] || qmake) && make -j9);
QMAKE_EXTRA_TARGETS += docs

# Provide "make tests" so we don't need to rebuild the tests during development
tests.commands = (cd test && ([ -f Makefile ] || qmake) && make -j9 && make check);
QMAKE_EXTRA_TARGETS += tests

# Provide "make cleanall", which cleans the main project and all external subprojects
cleanall.commands = make clean
cleanall.commands += ; (for i in example/api/* example/node/* example/type/* ; do (cd $${DOLLAR}$${DOLLAR}i ; make clean ; rm -Rf Makefile pch *.app); done)
cleanall.commands += ; (for i in node/vuo.*/examples; do (cd $${DOLLAR}$${DOLLAR}i && make clean); done)
cleanall.commands += ; (cd documentation && make clean)
cleanall.commands += ; (cd test && make clean)
cleanall.commands += ; true
QMAKE_EXTRA_TARGETS += cleanall
