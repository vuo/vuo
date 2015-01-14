TEMPLATE = lib
CONFIG += no_link target_predeps staticlib VuoNoLibrary

include(../vuo.pri)

NODE_SOURCES += \
	vuo.color.get.hsl.c \
	vuo.color.get.rgb.c \
	vuo.color.make.hsl.c \
	vuo.color.make.rgb.c \
	vuo.console.window.c \
	vuo.event.fireOnStart.c \
	vuo.event.spinOffEvent.c \
	vuo.hold.VuoBlendMode.c \
	vuo.hold.VuoBoolean.c \
	vuo.hold.VuoColor.c \
	vuo.hold.VuoCountWrapMode.c \
	vuo.hold.VuoCurve.c \
	vuo.hold.VuoCurveDomain.c \
	vuo.hold.VuoFrameRequest.c \
	vuo.hold.VuoGradientNoise.c \
	vuo.hold.VuoImage.c \
	vuo.hold.VuoInteger.c \
	vuo.hold.VuoLeapFrame.c \
	vuo.hold.VuoLeapHand.c \
	vuo.hold.VuoLeapPointable.c \
	vuo.hold.VuoMidiController.c \
	vuo.hold.VuoMidiDevice.c \
	vuo.hold.VuoMidiNote.c \
	vuo.hold.VuoMouseButtonAction.c \
	vuo.hold.VuoNoise.c \
	vuo.hold.VuoPoint2d.c \
	vuo.hold.VuoPoint3d.c \
	vuo.hold.VuoPoint4d.c \
	vuo.hold.VuoReal.c \
	vuo.hold.VuoSceneObject.c \
	vuo.hold.VuoShader.c \
	vuo.hold.VuoText.c \
	vuo.hold.VuoTransform.c \
	vuo.hold.VuoVertices.c \
	vuo.hold.VuoWave.c \
	vuo.image.filter.blend.c \
	vuo.image.filter.ripple.c \
	vuo.image.filter.twirl.c \
	vuo.image.get.c \
	vuo.image.render.window.c \
	vuo.leap.c \
	vuo.leap.get.frame.c \
	vuo.leap.get.hand.c \
	vuo.leap.get.pointable.c \
	vuo.leap.filter.hand.id.c \
	vuo.leap.filter.pointable.type.c \
	vuo.leap.filter.pointable.id.c \
	vuo.leap.hand.sort.distance.c \
	vuo.leap.hand.sort.distance.x.c \
	vuo.leap.hand.sort.distance.y.c \
	vuo.leap.hand.sort.distance.z.c \
	vuo.leap.pointable.sort.distance.c \
	vuo.leap.pointable.sort.distance.x.c \
	vuo.leap.pointable.sort.distance.y.c \
	vuo.leap.pointable.sort.distance.z.c \
	vuo.list.count.VuoLeapHand.c \
	vuo.list.count.VuoLeapPointable.c \
	vuo.list.get.VuoLeapFrame.c \
	vuo.list.get.VuoLeapHand.c \
	vuo.list.get.VuoLeapPointable.c \
	vuo.list.get.VuoLeapPointableType.c \
	vuo.list.get.VuoBlendMode.c \
	vuo.list.get.VuoBoolean.c \
	vuo.list.get.VuoColor.c \
	vuo.list.get.VuoCountWrapMode.c \
	vuo.list.get.VuoCurve.c \
	vuo.list.get.VuoCurveDomain.c \
	vuo.list.get.VuoFrameRequest.c \
	vuo.list.get.VuoGradientNoise.c \
	vuo.list.get.VuoImage.c \
	vuo.list.get.VuoInteger.c \
	vuo.list.get.VuoMidiController.c \
	vuo.list.get.VuoMidiDevice.c \
	vuo.list.get.VuoMidiNote.c \
	vuo.list.get.VuoMouseButtonAction.c \
	vuo.list.get.VuoNoise.c \
	vuo.list.get.VuoPoint2d.c \
	vuo.list.get.VuoPoint3d.c \
	vuo.list.get.VuoPoint4d.c \
	vuo.list.get.VuoReal.c \
	vuo.list.get.VuoSceneObject.c \
	vuo.list.get.VuoShader.c \
	vuo.list.get.VuoText.c \
	vuo.list.get.VuoTransform.c \
	vuo.list.get.VuoVertices.c \
	vuo.logic.areAllTrue.c \
	vuo.logic.areAnyTrue.c \
	vuo.logic.isOneTrue.c \
	vuo.logic.negate.c \
	vuo.math.add.integer.c \
	vuo.math.add.real.c \
	vuo.math.areEqual.integer.c \
	vuo.math.areEqual.real.c \
	vuo.math.count.integer.c \
	vuo.math.count.real.c \
	vuo.math.countWithinRange.integer.c \
	vuo.math.countWithinRange.real.c \
	vuo.math.divide.integer.c \
	vuo.math.divide.real.c \
	vuo.math.isGreaterThan.integer.c \
	vuo.math.isGreaterThan.real.c \
	vuo.math.isLessThan.integer.c \
	vuo.math.isLessThan.real.c \
	vuo.math.limitToRange.integer.c \
	vuo.math.limitToRange.real.c \
	vuo.math.max.integer.c \
	vuo.math.max.real.c \
	vuo.math.min.integer.c \
	vuo.math.min.real.c \
	vuo.math.multiply.integer.c \
	vuo.math.multiply.real.c \
	vuo.math.round.c \
	vuo.math.roundDown.c \
	vuo.math.roundUp.c \
	vuo.math.scale.c \
	vuo.math.subtract.integer.c \
	vuo.math.subtract.real.c \
	vuo.math.wave.c \
	vuo.midi.filter.controller.c \
	vuo.midi.filter.note.c \
	vuo.midi.get.controller.c \
	vuo.midi.get.device.c \
	vuo.midi.get.note.c \
	vuo.midi.listDevices.c \
	vuo.midi.make.controller.c \
	vuo.midi.make.device.id.c \
	vuo.midi.make.device.name.c \
	vuo.midi.make.note.c \
	vuo.midi.receive.c \
	vuo.midi.send.c \
	vuo.mouse.c \
	vuo.mouse.filter.action.c \
	vuo.mouse.get.action.c \
	vuo.noise.gradient.1d.c \
	vuo.noise.gradient.2d.c \
	vuo.noise.gradient.3d.c \
	vuo.noise.gradient.4d.c \
	vuo.point.add.2d.c \
	vuo.point.add.3d.c \
	vuo.point.add.4d.c \
	vuo.point.get.2d.c \
	vuo.point.get.3d.c \
	vuo.point.get.4d.c \
	vuo.point.make.2d.c \
	vuo.point.make.3d.c \
	vuo.point.make.4d.c \
	vuo.point.multiply.quaternion.c \
	vuo.point.multiply.scalar.2d.c \
	vuo.point.multiply.scalar.3d.c \
	vuo.point.multiply.scalar.4d.c \
	vuo.point.normalize.2d.c \
	vuo.point.normalize.3d.c \
	vuo.point.normalize.4d.c \
	vuo.point.sort.distance.2d.c \
	vuo.point.sort.distance.3d.c \
	vuo.point.sort.distance.4d.c \
	vuo.point.sort.distance.w.4d.c \
	vuo.point.sort.distance.x.2d.c \
	vuo.point.sort.distance.x.3d.c \
	vuo.point.sort.distance.x.4d.c \
	vuo.point.sort.distance.y.2d.c \
	vuo.point.sort.distance.y.3d.c \
	vuo.point.sort.distance.y.4d.c \
	vuo.point.sort.distance.z.3d.c \
	vuo.point.sort.distance.z.4d.c \
	vuo.point.subtract.2d.c \
	vuo.point.subtract.3d.c \
	vuo.point.subtract.4d.c \
	vuo.quaternion.make.angle.c \
	vuo.quaternion.make.vectors.c \
	vuo.scene.combine.c \
	vuo.scene.frameRequest.get.frameCount.c \
	vuo.scene.frameRequest.get.timestamp.c \
	vuo.scene.get.c \
	vuo.scene.make.c \
	vuo.scene.make.cube.c \
	vuo.scene.make.image.c \
	vuo.scene.render.image.c \
	vuo.scene.render.window.c \
	vuo.select.in.2.VuoBlendMode.c \
	vuo.select.in.2.VuoBoolean.c \
	vuo.select.in.2.VuoColor.c \
	vuo.select.in.2.VuoCountWrapMode.c \
	vuo.select.in.2.VuoCurve.c \
	vuo.select.in.2.VuoCurveDomain.c \
	vuo.select.in.2.VuoFrameRequest.c \
	vuo.select.in.2.VuoGradientNoise.c \
	vuo.select.in.2.VuoImage.c \
	vuo.select.in.2.VuoInteger.c \
	vuo.select.in.2.VuoLeapFrame.c \
	vuo.select.in.2.VuoLeapHand.c \
	vuo.select.in.2.VuoLeapPointable.c \
	vuo.select.in.2.VuoMidiController.c \
	vuo.select.in.2.VuoMidiDevice.c \
	vuo.select.in.2.VuoMidiNote.c \
	vuo.select.in.2.VuoMouseButtonAction.c \
	vuo.select.in.2.VuoNoise.c \
	vuo.select.in.2.VuoPoint2d.c \
	vuo.select.in.2.VuoPoint3d.c \
	vuo.select.in.2.VuoPoint4d.c \
	vuo.select.in.2.VuoReal.c \
	vuo.select.in.2.VuoSceneObject.c \
	vuo.select.in.2.VuoShader.c \
	vuo.select.in.2.VuoText.c \
	vuo.select.in.2.VuoTransform.c \
	vuo.select.in.2.VuoVertices.c \
	vuo.select.in.2.VuoWave.c \
	vuo.select.in.2.event.c \
	vuo.select.in.VuoBlendMode.c \
	vuo.select.in.VuoBoolean.c \
	vuo.select.in.VuoColor.c \
	vuo.select.in.VuoCountWrapMode.c \
	vuo.select.in.VuoCurve.c \
	vuo.select.in.VuoCurveDomain.c \
	vuo.select.in.VuoFrameRequest.c \
	vuo.select.in.VuoGradientNoise.c \
	vuo.select.in.VuoImage.c \
	vuo.select.in.VuoInteger.c \
	vuo.select.in.VuoLeapFrame.c \
	vuo.select.in.VuoLeapHand.c \
	vuo.select.in.VuoLeapPointable.c \
	vuo.select.in.VuoMidiController.c \
	vuo.select.in.VuoMidiDevice.c \
	vuo.select.in.VuoMidiNote.c \
	vuo.select.in.VuoMouseButtonAction.c \
	vuo.select.in.VuoNoise.c \
	vuo.select.in.VuoPoint2d.c \
	vuo.select.in.VuoPoint3d.c \
	vuo.select.in.VuoPoint4d.c \
	vuo.select.in.VuoReal.c \
	vuo.select.in.VuoSceneObject.c \
	vuo.select.in.VuoShader.c \
	vuo.select.in.VuoText.c \
	vuo.select.in.VuoTransform.c \
	vuo.select.in.VuoVertices.c \
	vuo.select.in.VuoWave.c \
	vuo.select.in.event.c \
	vuo.select.latest.2.VuoBlendMode.c \
	vuo.select.latest.2.VuoBoolean.c \
	vuo.select.latest.2.VuoColor.c \
	vuo.select.latest.2.VuoCountWrapMode.c \
	vuo.select.latest.2.VuoCurve.c \
	vuo.select.latest.2.VuoCurveDomain.c \
	vuo.select.latest.2.VuoFrameRequest.c \
	vuo.select.latest.2.VuoGradientNoise.c \
	vuo.select.latest.2.VuoImage.c \
	vuo.select.latest.2.VuoInteger.c \
	vuo.select.latest.2.VuoLeapFrame.c \
	vuo.select.latest.2.VuoLeapHand.c \
	vuo.select.latest.2.VuoLeapPointable.c \
	vuo.select.latest.2.VuoMidiController.c \
	vuo.select.latest.2.VuoMidiDevice.c \
	vuo.select.latest.2.VuoMidiNote.c \
	vuo.select.latest.2.VuoMouseButtonAction.c \
	vuo.select.latest.2.VuoNoise.c \
	vuo.select.latest.2.VuoPoint2d.c \
	vuo.select.latest.2.VuoPoint3d.c \
	vuo.select.latest.2.VuoPoint4d.c \
	vuo.select.latest.2.VuoReal.c \
	vuo.select.latest.2.VuoSceneObject.c \
	vuo.select.latest.2.VuoShader.c \
	vuo.select.latest.2.VuoText.c \
	vuo.select.latest.2.VuoTransform.c \
	vuo.select.latest.2.VuoVertices.c \
	vuo.select.latest.2.VuoWave.c \
	vuo.select.out.2.VuoBlendMode.c \
	vuo.select.out.2.VuoBoolean.c \
	vuo.select.out.2.VuoColor.c \
	vuo.select.out.2.VuoCountWrapMode.c \
	vuo.select.out.2.VuoCurve.c \
	vuo.select.out.2.VuoCurveDomain.c \
	vuo.select.out.2.VuoFrameRequest.c \
	vuo.select.out.2.VuoGradientNoise.c \
	vuo.select.out.2.VuoImage.c \
	vuo.select.out.2.VuoInteger.c \
	vuo.select.out.2.VuoLeapFrame.c \
	vuo.select.out.2.VuoLeapHand.c \
	vuo.select.out.2.VuoLeapPointable.c \
	vuo.select.out.2.VuoMidiController.c \
	vuo.select.out.2.VuoMidiDevice.c \
	vuo.select.out.2.VuoMidiNote.c \
	vuo.select.out.2.VuoMouseButtonAction.c \
	vuo.select.out.2.VuoNoise.c \
	vuo.select.out.2.VuoPoint2d.c \
	vuo.select.out.2.VuoPoint3d.c \
	vuo.select.out.2.VuoPoint4d.c \
	vuo.select.out.2.VuoReal.c \
	vuo.select.out.2.VuoSceneObject.c \
	vuo.select.out.2.VuoShader.c \
	vuo.select.out.2.VuoText.c \
	vuo.select.out.2.VuoTransform.c \
	vuo.select.out.2.VuoVertices.c \
	vuo.select.out.2.VuoWave.c \
	vuo.select.out.2.event.c \
	vuo.select.out.VuoImage.c \
	vuo.select.out.VuoInteger.c \
	vuo.select.out.VuoLeapFrame.c \
	vuo.select.out.VuoLeapHand.c \
	vuo.select.out.VuoLeapPointable.c \
	vuo.select.out.VuoPoint2d.c \
	vuo.select.out.VuoPoint3d.c \
	vuo.select.out.VuoPoint4d.c \
	vuo.select.out.VuoReal.c \
	vuo.select.out.VuoSceneObject.c \
	vuo.select.out.VuoShader.c \
	vuo.select.out.VuoText.c \
	vuo.select.out.VuoVertices.c \
	vuo.select.out.event.c \
	vuo.shader.make.color.c \
	vuo.shader.make.image.c \
	vuo.shader.make.normal.c \
	vuo.text.append.c \
	vuo.text.areEqual.c \
	vuo.text.countCharacters.c \
	vuo.text.cut.c \
	vuo.time.firePeriodically.c \
	vuo.transform.make.c \
	vuo.transform.make.quaternion.c \
	vuo.type.boolean.event.c \
	vuo.type.boolean.integer.c \
	vuo.type.boolean.text.c \
	vuo.type.color.event.c \
	vuo.type.frameRequest.event.c \
	vuo.type.image.event.c \
	vuo.type.integer.boolean.c\
	vuo.type.integer.event.c \
	vuo.type.integer.point2d.x.c \
	vuo.type.integer.point2d.y.c \
	vuo.type.integer.point3d.x.c \
	vuo.type.integer.point3d.y.c \
	vuo.type.integer.point3d.z.c \
	vuo.type.integer.point4d.w.c \
	vuo.type.integer.point4d.x.c \
	vuo.type.integer.point4d.y.c \
	vuo.type.integer.point4d.z.c \
	vuo.type.integer.real.c \
	vuo.type.integer.text.c \
	vuo.type.mouseButtonAction.event.c \
	vuo.type.point2d.event.c \
	vuo.type.point2d.point3d.xy.c \
	vuo.type.point2d.point4d.xy.c \
	vuo.type.point2d.real.x.c \
	vuo.type.point2d.real.y.c \
	vuo.type.point3d.event.c \
	vuo.type.point3d.point2d.xy.c \
	vuo.type.point3d.point4d.xyz.c \
	vuo.type.point3d.real.x.c \
	vuo.type.point4d.event.c \
	vuo.type.point4d.point2d.xy.c \
	vuo.type.point4d.point3d.xyz.c \
	vuo.type.point4d.real.x.c \
	vuo.type.real.event.c \
	vuo.type.real.point2d.x.c \
	vuo.type.real.point2d.y.c \
	vuo.type.real.point3d.x.c \
	vuo.type.real.point3d.y.c \
	vuo.type.real.point3d.z.c \
	vuo.type.real.point4d.w.c \
	vuo.type.real.point4d.x.c \
	vuo.type.real.point4d.y.c \
	vuo.type.real.point4d.z.c \
	vuo.type.real.text.c \
	vuo.type.sceneObject.event.c \
	vuo.type.shader.event.c \
	vuo.type.text.boolean.c \
	vuo.type.text.event.c \
	vuo.type.text.integer.c \
	vuo.type.text.real.c \
	vuo.type.transform.event.c \
	vuo.type.vertices.event.c \
	vuo.vertices.make.parametric.c \
	vuo.vertices.make.sphere.c \
	vuo.vertices.make.square.c \
	vuo.vertices.make.triangle.c

OTHER_FILES += $$NODE_SOURCES

NODE_LIBRARY_SOURCES += \
	VuoImageGet.cc \
	VuoImageRenderer.cc \
	VuoMidi.cc \
	VuoLeap.cc \
	VuoSceneRenderer.cc \
	VuoUrl.c \
	VuoWindow.m \
	VuoWindowApplication.m \
	VuoWindowOpenGLInternal.m \
	VuoGradientNoiseCommon.c \
	VuoWindowTextInternal.m \
	VuoVerticesParametric.cc

OTHER_FILES += $$NODE_LIBRARY_SOURCES

SOURCES += \
	VuoImageRenderer.cc

HEADERS += \
	module.h \
	node.h \
	VuoGlContext.h \
	VuoImageGet.h \
	VuoImageRenderer.h \
	VuoMidi.h \
	VuoLeap.h \
	VuoSceneRenderer.h \
	VuoUrl.h \
	VuoWindow.h \
	VuoWindowApplication.h \
	VuoWindowOpenGLInternal.h \
	VuoGradientNoiseCommon.h \
	VuoWindowTextInternal.h \
	VuoVerticesParametric.h

INCLUDEPATH += \
	../runtime \
	../type \
	../type/list

QMAKE_CLEAN += *.vuonode *.bc

CLANG_NODE_LIBRARY_FLAGS = \
	-I$${ROOT}/type \
	-I$${ROOT}/type/list \
	-I$${ROOT}/runtime \
	-I$${ICU_ROOT}/include \
	-I$${ROOT}/node/Leap \
	-I$${MUPARSER_ROOT}/include \
	-I$${FREEIMAGE_ROOT}/include \
	-I$${CURL_ROOT}/include \
	-I$${RTMIDI_ROOT}/include \
	-I$${ASSIMP_ROOT}/include \
	-DVUO_COMPILER=1 \
	$$join(DEFINES, " -D", "-D")
node_library.input = NODE_LIBRARY_SOURCES
node_library.depend_command = $$QMAKE_CC -o /dev/null -E -MD -MF - -emit-llvm $$QMAKE_CFLAGS_X86_64 $${CLANG_NODE_LIBRARY_FLAGS} ${QMAKE_FILE_NAME} 2>&1 | sed \"s,^.*: ,,\"
node_library.output = ${QMAKE_FILE_IN_BASE}.bc
node_library.commands = $$QMAKE_CC -cc1 -triple x86_64-apple-macosx10.6.0 -fblocks -fcxx-exceptions -emit-llvm-bc $$CLANG_NODE_LIBRARY_FLAGS -DVUO_COMPILER=1 ${QMAKE_FILE_IN} -o ${QMAKE_FILE_OUT}
QMAKE_EXTRA_COMPILERS += node_library


NODE_LIBRARY_SHARED_SOURCES += \
	VuoGlContext.cc

OTHER_FILES += $$NODE_LIBRARY_SHARED_SOURCES

CLANG_NODE_LIBRARY_SHARED_FLAGS = \
	$$CLANG_NODE_LIBRARY_FLAGS \
	-dynamiclib \
	-framework OpenGL \
	-Wl,-no_function_starts \
	-Wl,-no_version_load_command
CLANG_NODE_LIBRARY_SHARED_FLAGS -= -emit-llvm
node_library_shared.input = NODE_LIBRARY_SHARED_SOURCES
node_library_shared.depend_command = $$QMAKE_CXX -o /dev/null -E -MD -MF - $${CLANG_NODE_LIBRARY_SHARED_FLAGS} ${QMAKE_FILE_NAME} 2>&1 | sed \"s,^.*: ,,\"
node_library_shared.output = lib${QMAKE_FILE_IN_BASE}.dylib
node_library_shared.commands = $$QMAKE_CXX $$CLANG_NODE_LIBRARY_SHARED_FLAGS ${QMAKE_FILE_IN} -o ${QMAKE_FILE_OUT} \
	&& install_name_tool -id @rpath/Vuo.framework/Versions/$$VUO_VERSION/Modules/${QMAKE_FILE_OUT} ${QMAKE_FILE_OUT}
QMAKE_EXTRA_COMPILERS += node_library_shared



NODE_LIBRARY_SHARED_SOURCES_DEPENDENT_ON_CONTEXT += \
	VuoGlTexturePool.cc

OTHER_FILES += $$NODE_LIBRARY_SHARED_SOURCES_DEPENDENT_ON_CONTEXT

CLANG_NODE_LIBRARY_SHARED_DEPENDENT_ON_CONTEXT_FLAGS = \
	$$CLANG_NODE_LIBRARY_SHARED_FLAGS \
	-L . \
	-lVuoGlContext
node_library_shared_dependent_on_context.input = NODE_LIBRARY_SHARED_SOURCES_DEPENDENT_ON_CONTEXT
node_library_shared_dependent_on_context.depends = libVuoGlContext.dylib
node_library_shared_dependent_on_context.depend_command = $$QMAKE_CXX -o /dev/null -E -MD -MF - $${CLANG_NODE_LIBRARY_SHARED_DEPENDENT_ON_CONTEXT_FLAGS} ${QMAKE_FILE_NAME} 2>&1 | sed \"s,^.*: ,,\"
node_library_shared_dependent_on_context.output = lib${QMAKE_FILE_IN_BASE}.dylib
node_library_shared_dependent_on_context.commands = $$QMAKE_CXX $$CLANG_NODE_LIBRARY_SHARED_DEPENDENT_ON_CONTEXT_FLAGS ${QMAKE_FILE_IN} -o ${QMAKE_FILE_OUT} \
	&& install_name_tool -id @rpath/Vuo.framework/Versions/$$VUO_VERSION/Modules/${QMAKE_FILE_OUT} ${QMAKE_FILE_OUT}
QMAKE_EXTRA_COMPILERS += node_library_shared_dependent_on_context
