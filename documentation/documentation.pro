TEMPLATE = lib
CONFIG += no_link target_predeps staticlib VuoNoLibrary

include(../vuo.pri)

doxygen.commands = (cat Doxyfile ; echo "PROJECT_NUMBER=$$VUO_VERSION" ; echo "STRIP_FROM_PATH=`(cd .. ; pwd)`") | /usr/local/bin/doxygen -
doxygen.depends = \
	../base/*.c* ../base/*.h* \
	../compiler/*.c* ../compiler/*.h* \
	../editor/*.c* ../editor/*.h* \
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
	../node/vuo.math.isLessThan.integer.c \
	../node/vuo.logic.isOneTrue.c \
	../node/vuo.console.window.c \
	../node/vuo.mouse.c \
	../node/vuo.event.spinOffEvent.c \
	../node/vuo.event.fireOnStart.c \
	../node/vuo.hold.VuoInteger.c \
	../node/vuo.image.get.c \
	../node/vuo.math.add.integer.c \
	../node/vuo.math.subtract.integer.c \
	../node/vuo.math.count.integer.c \
	../node/vuo.select.in.2.VuoInteger.c \
	../node/vuo.select.out.2.VuoInteger.c \
	../node/vuo.select.out.event.c \
	../node/vuo.select.latest.2.VuoInteger.c \
	../node/vuo.time.firePeriodically.c

COMPOSITION_IMAGES += \
	composition/CountIsLessThanToConsole.vuo \
	composition/CalculateBoxVolume.vuo \
	composition/DisplayImageTwice.vuo \
	composition/MultipleEventOnlyCables.vuo \
	composition/2Recur.vuo \
	composition/CountCharactersToConsole.vuo \
	composition/CountScatter.vuo \
	composition/DiscardDataFromEventToCount.vuo \
	composition/CountWithInfiniteFeedback.vuo \
	composition/SumAddOne.vuo \
	../example/composition/Count.vuo \
	../example/composition/CountAndHold.vuo \
	../example/composition/CountAndOffset.vuo \
	../example/composition/CountSometimes.vuo \
	../example/composition/CountWithFeedback.vuo \
	../example/composition/CountWithPublishedPorts.vuo \
	../example/composition/DisplayImage.vuo \
	../example/composition/LoadImageAsynchronously.vuo \
	../example/composition/CheckSMSLength.vuo 

pandoc.commands = cat VuoManual.txt \
	| awk \'{sub(/VUO_VERSION/,\"$$VUO_VERSION\");print}\' \
	| /usr/local/bin/pandoc \
	--latex-engine=xelatex \
	--table-of-contents \
	--number-sections \
	--smart \
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
QMAKE_CLEAN += VuoManual.pdf
OTHER_FILES += VuoManual.txt
OTHER_FILES += VuoManualHeader.tex

node_class_image.input = NODE_CLASS_IMAGES
node_class_image.output = image-generated/${QMAKE_FILE_IN_BASE}.pdf
node_class_image.commands  = $$VUORENDER --output-format=pdf --output image-generated/${QMAKE_FILE_OUT_BASE}.pdf ${QMAKE_FILE_IN_BASE} ;
node_class_image.commands += $$VUORENDER --output-format=png --output image-generated/${QMAKE_FILE_OUT_BASE}.png ${QMAKE_FILE_IN_BASE} ;
QMAKE_EXTRA_COMPILERS += node_class_image

composition_image.input = COMPOSITION_IMAGES
composition_image.output = image-generated/${QMAKE_FILE_IN_BASE}.pdf
composition_image.commands  = $$VUORENDER --output-format=pdf --output image-generated/${QMAKE_FILE_OUT_BASE}.pdf ${QMAKE_FILE_IN} ;
composition_image.commands += $$VUORENDER --output-format=png --output image-generated/${QMAKE_FILE_OUT_BASE}.png ${QMAKE_FILE_IN} ;
QMAKE_EXTRA_COMPILERS += composition_image
