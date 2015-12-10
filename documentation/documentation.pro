TEMPLATE = aux

include(../vuo.pri)

doxygen.commands = (cat Doxyfile ; echo "PROJECT_NUMBER=$$VUO_VERSION" ; echo "STRIP_FROM_PATH=`(cd .. ; pwd)`") | /usr/local/bin/doxygen -
doxygen.depends = \
	../base/*.c* ../base/*.h* \
	../compiler/*.c* ../compiler/*.h* \
	../editor/*.c* ../editor/*.h* \
	../library/*.c* ../library/*.m* ../library/*.h* \
	../node/*.c* ../node/*.h* \
	../renderer/*.c* ../renderer/*.h* \
	../type/*.c* ../type/*.h* \
	../type/list/*.c* ../type/list/*.h*
doxygen.target = ../doxygen
QMAKE_EXTRA_TARGETS += doxygen
POST_TARGETDEPS += ../doxygen
QMAKE_CLEAN += ../doxygen
OTHER_FILES += Doxyfile

NODE_CLASS_IMAGES += \
	../node/vuo.console/vuo.console.window.c \
	../node/vuo.data/vuo.data.share.c \
	../node/vuo.event/vuo.event.becameTrue.c \
	../node/vuo.event/vuo.event.changed.c \
	../node/vuo.event/vuo.event.spinOffEvent.c \
	../node/vuo.event/vuo.event.fireOnStart.c \
	../node/vuo.image/vuo.image.fetch.c \
	../node/vuo.image/vuo.image.render.window.c \
	../node/vuo.logic/vuo.logic.areAllTrue.c \
	../node/vuo.logic/vuo.logic.isOneTrue.c \
	../node/vuo.logic/vuo.logic.toggle.c \
	../node/vuo.logic/vuo.logic.switch.c \
	../node/vuo.math/vuo.math.add.c \
	../node/vuo.list/vuo.list.build.c \
	../node/vuo.list/vuo.list.cycle.c \
	../node/vuo.list/vuo.list.enqueue.c \
	../node/vuo.list/vuo.list.process.c \
	../node/vuo.math/vuo.math.count.c \
	../node/vuo.math/vuo.math.countWithinRange.c \
	../node/vuo.math/vuo.math.divide.VuoInteger.c \
	../node/vuo.math/vuo.math.isLessThan.c \
	../node/vuo.math/vuo.math.limitToRange.c \
	../node/vuo.math/vuo.math.subtract.c \
	../node/vuo.motion/vuo.motion.wave.c \
	../node/vuo.select/vuo.select.in.2.c \
	../node/vuo.select/vuo.select.in.8.c \
	../node/vuo.select/vuo.select.in.boolean.c \
	../node/vuo.select/vuo.select.latest.2.c \
	../node/vuo.select/vuo.select.out.2.c \
	../node/vuo.select/vuo.select.out.boolean.event.c \
	../node/vuo.time/vuo.time.firePeriodically.c \
	../node/vuo.time/vuo.time.measureTime.c \
	../node/vuo.video/vuo.video.decodeImage.c

COMPOSITION_IMAGES += \
	composition/2Recur.vuo \
	composition/AreAllValuesTrue.vuo \
	composition/BuildColoredGrid.vuo \
	composition/CalculateBoxVolume.vuo \
	composition/Count.vuo \
	composition/CountAndHold.vuo \
	composition/CountCharactersToConsole.vuo \
	composition/CountIsLessThanToConsole.vuo \
	composition/CountIsGreaterThanToConsole.vuo \
	composition/CountScatter.vuo \
	composition/CountSometimes.vuo \
	composition/CountWithFeedback.vuo \
	composition/CountWithInfiniteFeedback.vuo \
	composition/DeadlockedFeedbackLoop.vuo \
	composition/DisplayHelloWorldImage.vuo \
	composition/DisplayandTwirlHelloWorldImage.vuo \
	composition/DisplayMultipleWindows.vuo \
	composition/InvertMovieColors.vuo \
	composition/MultipleEventOnlyCables.vuo \
	composition/SelectColor.vuo \
	composition/SelectLatestInput.vuo \
	composition/ShareHeight.vuo \
	composition/SpinCube.vuo \
	composition/SlideCheckerboard.vuo \
	composition/SlideAndBlendCheckerboardsWithPeriodicEvents.vuo \
	composition/SlideAndBlendCheckerboardsWithFrameEvents.vuo \
	composition/SumAddOne.vuo \
	../node/vuo.event/examples/LoadImageAsynchronously.vuo \
	../node/vuo.image/examples/BlendImages.vuo \
	../node/vuo.image/examples/DisplayImage.vuo \
	../node/vuo.video/examples/PlayMovie.vuo

pandoc.commands = cat VuoManual.txt \
	| awk \'{sub(/VUO_VERSION/,\"$$VUO_VERSION\");print}\' \
	| /usr/local/bin/pandoc \
	--latex-engine=xelatex \
	--table-of-contents \
	--number-sections \
	--smart \
	--template=VuoManualTemplate.tex \
	--include-in-header=VuoManualHeader.tex \
	--from=markdown-yaml_metadata_block \
	-o VuoManual.pdf \
	-
pandoc.depends = VuoManual.txt
NODE_CLASS_IMAGE_BASENAMES = $$basename(NODE_CLASS_IMAGES)
for(i,NODE_CLASS_IMAGE_BASENAMES): pandoc.depends += image-generated/$$replace(i,".c$",".pdf")
COMPOSITION_IMAGE_BASENAMES = $$basename(COMPOSITION_IMAGES)
for(i,COMPOSITION_IMAGE_BASENAMES): pandoc.depends += image-generated/$$replace(i,".vuo$",".pdf")
pandoc.target = VuoManual.pdf
QMAKE_EXTRA_TARGETS += pandoc
POST_TARGETDEPS += VuoManual.pdf
QMAKE_CLEAN += VuoManual.pdf image/Magic_Wand-eps-converted-to.pdf
OTHER_FILES += VuoManual.txt
OTHER_FILES += VuoManualHeader.tex

node_class_image.input = NODE_CLASS_IMAGES
node_class_image.output = image-generated/${QMAKE_FILE_IN_BASE}.pdf
node_class_image.commands = $$ROOT/documentation/renderNodeClassImages.sh "$$VUORENDER" "${QMAKE_FILE_OUT_BASE}"
QMAKE_EXTRA_COMPILERS += node_class_image
QMAKE_CLEAN += image-generated/*.png

composition_image.input = COMPOSITION_IMAGES
composition_image.output = image-generated/${QMAKE_FILE_IN_BASE}.pdf
composition_image.commands  = $$VUORENDER --output-format=pdf --output image-generated/${QMAKE_FILE_OUT_BASE}.pdf ${QMAKE_FILE_IN} ;
composition_image.commands += $$VUORENDER --output-format=png --output image-generated/${QMAKE_FILE_OUT_BASE}.png ${QMAKE_FILE_IN} ;
QMAKE_EXTRA_COMPILERS += composition_image
