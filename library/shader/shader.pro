TEMPLATE = aux

SHADERS += \
	VuoGlslProjection.glsl \
	VuoGlslRandom.glsl \
	deform.glsl \
	hsl.glsl \
	lighting.glsl \
	noise2D.glsl \
	noise3D.glsl \
	noise4D.glsl \
	triangle.glsl \
	triangleLine.glsl \
	trianglePoint.glsl

OTHER_FILES += $$SHADERS

include.input = SHADERS
include.output = ${QMAKE_FILE_IN_BASE}.h
include.commands = xxd -i ${QMAKE_FILE_IN} > ${QMAKE_FILE_IN_BASE}.h
include.CONFIG += no_link target_predeps
QMAKE_EXTRA_COMPILERS += include

QMAKE_CLEAN += *.h
