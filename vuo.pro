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
	node \
	renderer \
	renderer_vuo_render \
	runtime \
	type_list \
	type

OTHER_FILES += \
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
framework.depends = base compiler renderer runtime node type type_list

node.depends = compiler_vuo_compile type type_list

renderer.depends = base compiler

renderer_vuo_render.subdir = renderer/vuo-render
renderer_vuo_render.depends = framework

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
		framework
}


# Provide "make go" for easily running the editor from the command line
go.commands = (cd "editor/VuoEditorApp/Vuo*.app/Contents/MacOS" ; ./Vuo*)
QMAKE_EXTRA_TARGETS += go

# Provide "make examples" so we don't need to rebuild all the examples during development
DOLLAR = $
examples.commands = (for i in example/api/* example/node/* example/type/* ; do (cd $${DOLLAR}$${DOLLAR}i && ([ -f Makefile ] || qmake) && make -j9); done)
examples.commands += && (cd example/composition && ([ -f Makefile ] || qmake) && make -j9)
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
cleanall.commands += ; (cd example/composition && make clean)
cleanall.commands += ; (cd documentation && make clean)
cleanall.commands += ; (cd test && make clean)
cleanall.commands += ; true
QMAKE_EXTRA_TARGETS += cleanall
