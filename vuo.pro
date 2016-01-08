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
	library_shader \
	node \
	renderer \
	renderer_vuo_export \
	renderer_vuo_render \
	runtime \
	type \
	type_input_editor \
	type_list

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
framework.depends = base compiler renderer runtime node type type_input_editor type_list library

library.depends = base library_shader type

node.depends = compiler_vuo_compile type type_list

renderer.depends = base compiler

renderer_vuo_export.subdir = renderer/vuo-export
renderer_vuo_export.depends = framework

renderer_vuo_render.subdir = renderer/vuo-render
renderer_vuo_render.depends = framework

runtime.depends = base

library_shader.subdir = library/shader

type.depends = compiler_vuo_compile

type_input_editor.subdir = type/inputEditor
type_input_editor.depends = type type_list node library

type_list.subdir = type/list
type_list.depends = type


exists(editor) {
	SUBDIRS += \
		editor \
		editor_VuoEditor

	editor.depends = \
		compiler \
		renderer \
		type_input_editor

	editor_VuoEditor.subdir = editor/VuoEditorApp
	editor_VuoEditor.depends = \
		editor \
		framework
}


# Provide "make go" for easily running the editor from the command line
go.commands = @(cd "editor/VuoEditorApp/Vuo*.app/Contents/MacOS" ; ./Vuo* 2>&1 \
	| grep --line-buffered -v "'QTextCursor::setPosition: Position .* out of range'" \
	| grep --line-buffered -v "'^INVALID PARENT FOR INTERFACE'" \
	| grep --line-buffered -v "'^QCoreTextFontDatabase: Failed to resolve family name for PostScript name'" \
	; true)
QMAKE_EXTRA_TARGETS += go

# Provide "make examples" so we don't need to rebuild all the examples during development
DOLLAR = $
examples.commands = (for i in example/runner/* example/node/* ; do (cd $${DOLLAR}$${DOLLAR}i && ([ -f Makefile ] || qmake) && make -j9); done)
examples.commands += && (for i in node/vuo.*/examples ; do (cd $${DOLLAR}$${DOLLAR}i && ([ -f Makefile ] || qmake) && make -j9); done)
QMAKE_EXTRA_TARGETS += examples

# Provide "make documentation" so we don't need to rebuild the documentation during development
docs.commands = (cd documentation && ([ -f Makefile ] || qmake) && make -j9);
QMAKE_EXTRA_TARGETS += docs

# Provide "make tests" so we don't need to rebuild the tests during development
tests.commands = (cd test && ([ -f Makefile ] || qmake) && make -j9 && make check);
QMAKE_EXTRA_TARGETS += tests

# Provide "make vuo32" so we don't need to build the 32-bit portion of Vuo.framework during development
vuo32.commands = (cd compiler/vuo-compile-for-framework32 && ([ -f Makefile ] || qmake) && make -j9)
vuo32.commands += && (cd compiler/vuo-link-for-framework32 && ([ -f Makefile ] || qmake) && make -j9)
vuo32.commands += && (cd framework32 && ([ -f Makefile ] || qmake) && make -j9)
QMAKE_EXTRA_TARGETS += vuo32

# Provide "make cleanall", which cleans the main project and all external subprojects
cleanall.commands = make clean
cleanall.commands += ; (for i in example/runner/* example/node/* ; do (cd $${DOLLAR}$${DOLLAR}i ; make clean ; rm -Rf Makefile pch *.app); done)
cleanall.commands += ; (for i in node/vuo.*/examples; do (cd $${DOLLAR}$${DOLLAR}i && make clean); done)
cleanall.commands += ; (cd documentation && make clean)
cleanall.commands += ; (cd framework32 && make clean)
cleanall.commands += ; (cd test && make clean)
cleanall.commands += ; rm -f .qmake.cache .qmake.stash
cleanall.commands += ; true
QMAKE_EXTRA_TARGETS += cleanall
