TEMPLATE = aux

include(../vuo.pri)

EDITOR_FOLDER = $$ROOT/editor
doxygen.commands = (cat Doxyfile ; echo "PROJECT_NUMBER=$$VUO_VERSION" ; echo "STRIP_FROM_PATH=`(cd .. ; pwd)`") | /usr/local/bin/doxygen -
doxygen.depends = \
	../base/*.c* ../base/*.h* \
	../compiler/*.c* ../compiler/*.h* \
	../library/*.c* ../library/*.m* ../library/*.h* \
	../node/*.c* ../node/*.h* \
	../renderer/*.c* ../renderer/*.h* \
	../type/*.c* ../type/*.h* \
	../type/list/*.c* ../type/list/*.h*
exists($$EDITOR_FOLDER) {
	doxygen.depends += $$EDITOR_FOLDER/*.c* $$EDITOR_FOLDER/*.h*
}
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
	../node/vuo.image/vuo.image.color.adjust.c \
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
	../node/vuo.scene/vuo.scene.fetch.c \
	../node/vuo.scene/vuo.scene.render.window.c \
	../node/vuo.select/vuo.select.in.2.c \
	../node/vuo.select/vuo.select.in.8.c \
	../node/vuo.select/vuo.select.in.boolean.c \
	../node/vuo.select/vuo.select.latest.2.c \
	../node/vuo.select/vuo.select.out.2.c \
	../node/vuo.select/vuo.select.out.boolean.event.c \
	../node/vuo.time/vuo.time.firePeriodically.c \
	../node/vuo.time/vuo.time.measureTime.c \
	../node/vuo.video/vuo.video.save.c \
	../node/vuo.video/vuo.video.decodeImage.c \
	../node/vuo.video/vuo.video.play.c

COMPOSITION_IMAGES += \
	composition/2Recur.vuo \
	composition/AreAllValuesTrue.vuo \
   composition/BeepWhenMouseEntersSquare.vuo \
	composition/BuildColoredGrid.vuo \
	composition/CalculateBoxVolume.vuo \
   composition/ChangeBackgroundColor.vuo \
   composition/ChangeTilingPeriodically.vuo \
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
   composition/DetectBarcode.vuo \
   composition/DetectCamera.vuo \
	composition/DisplayHelloWorldImage.vuo \
	composition/DisplayAndTwirlHelloWorldImage.vuo \
	composition/DisplayAndTwirlHelloWorldContinuously.vuo \
	composition/DisplayHelloWorldNoEvent.vuo \
	composition/DisplayMultipleWindows.vuo \
   composition/FetchOnlyWhenURLChanges.vuo \
   composition/GraduallyMoveCircle.vuo \
	composition/InvertMovieColors.vuo \
   composition/MoveCircleWithMouse.vuo \
	composition/MultipleEventOnlyCables.vuo \
   composition/RememberMousePresses.vuo \
   composition/RespondToMagicWord.vuo \
	composition/SelectColor.vuo \
	composition/SelectLatestInput.vuo \
	composition/ShareHeight.vuo \
	composition/SpinCube.vuo \
	composition/SlideCheckerboard.vuo \
	composition/SlideAndBlendCheckerboardsWithPeriodicEvents.vuo \
	composition/SlideAndBlendCheckerboardsWithFrameEvents.vuo \
	composition/SumAddOne.vuo \
   composition/SwitchControllees.vuo \
   composition/SwitchControllers.vuo \
   ../node/vuo.data/examples/StoreMousePosition.vuo \
	../node/vuo.event/examples/LoadImageAsynchronously.vuo \
	../node/vuo.image/examples/BlendImages.vuo \
	../node/vuo.image/examples/DisplayImage.vuo \
   ../node/vuo.list/examples/CycleSeasons.vuo \
   ../node/vuo.list/examples/DisplayGridOfImages.vuo \
   ../node/vuo.list/examples/DisplayRainbowOvals.vuo \
   ../node/vuo.list/examples/ReplaceColorsInGradient.vuo \
   ../node/vuo.logic/examples/IsMouseWithinIntersectingRectangles.vuo \
   ../node/vuo.motion/examples/SpringBack.vuo \
   ../node/vuo.motion/examples/WaveCircle.vuo \
   ../node/vuo.scene/examples/MoveSpinningSphere.vuo \
	../node/vuo.select/examples/SelectMovie.vuo \
   ../node/vuo.select/examples/ShowArrowPresses.vuo \
   ../node/vuo.time/examples/AnimateOnSchedule.vuo \
   ../node/vuo.time/examples/FlashOnMousePress.vuo \
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
OTHER_FILES += \
	VuoManual.txt \
	VuoManualHeader.tex \
	VuoManualTemplate.tex \
	renderNodeClassImages.sh



DOLLAR = $
GHOSTSCRIPT_FLAGS = \
	-dBATCH \
	-q \
	-dQUIET \
	-dSAFER \
	-dNOPAUSE \
	-dNOPROMPT
pandocHTML.commands = \
	mkdir -p VuoManual/image VuoManual/image-generated \
	&& cd VuoManual \
	&& cp ../VuoManual.css . \
	&& cp ../image/*.png ../image/*.svg image \
	&& (for i in ../image/*.pdf; do \
		# Workaround for apparent Ghostscript bug where it deletes the character "i" from the embedded font.
		$$GHOSTSCRIPT_ROOT/bin/gs $$GHOSTSCRIPT_FLAGS \
			-sDEVICE=pdfwrite \
			-dEmbedAllFonts=false \
			-o /tmp/gs.pdf \
			$${DOLLAR}$${DOLLAR}i \
		# Convert PDF to PNG
		&& $$GHOSTSCRIPT_ROOT/bin/gs $$GHOSTSCRIPT_FLAGS \
			-sDEVICE=pngalpha \
			-dMaxBitmap=2147483647 \
			-dAlignToPixels=0 \
			-dGridFitTT=2 \
			-dTextAlphaBits=4 \
			-dGraphicsAlphaBits=4 \
			-r144x144 \
			-o /tmp/gs.png \
			/tmp/gs.pdf \
		# Reduce size of PNG files
		&& $$PNGQUANT_ROOT/bin/pngquant \
			--force \ # overwrite existing files
			--speed 1 \
			--output image/$${DOLLAR}$${DOLLAR}(basename $${DOLLAR}$${DOLLAR}i .pdf).png \
			/tmp/gs.png \
		; done) \
	&& cp ../image-generated/*.png image-generated \
	&& cp ../../editor/VuoEditorApp/Icons/vuo.png image \
	&& cat ../VuoManual.txt \
		| awk \'{sub(/VUO_VERSION/,\"$$VUO_VERSION\");print}\' \
		# Markdown -> JSON
		| /usr/local/bin/pandoc \
			--smart \
			--from markdown-yaml_metadata_block \
			--to json \
			-o - \
			- \
		# JSON -> JSON, changing LaTeX commands into Docbook-compatible XML encoded in JSON
		| ../latexToDocbook.php \
		# JSON -> Docbook
		| /usr/local/bin/pandoc \
			--standalone \
			--from json \
			--to docbook \
			-o - \
			- \
		# Docbook -> Docbook, changing the DTD and other stuff Pandoc doesn't let us change
		| ../transformDocbook.php $$VUO_VERSION \
		# Docbook -> HTML
		| xsltproc \
			--nonet \
			--stringparam chunk.section.depth 2 \
			--stringparam html.stylesheet VuoManual.css \
			--stringparam para.propagates.style 1 \
			--stringparam phrase.propagates.style 1 \
			--stringparam section.autolabel arabic \
			--stringparam toc.section.depth 1 \
			--stringparam use.id.as.filename 1 \
			../VuoManual.xsl \
			- \
		2>&1 \
		| ( grep -v '^Writing ' || true )
pandocHTML.depends = VuoManual.txt
for(i,NODE_CLASS_IMAGE_BASENAMES):  pandocHTML.depends += image-generated/$$replace(i,".c$",".pdf")
for(i,COMPOSITION_IMAGE_BASENAMES): pandocHTML.depends += image-generated/$$replace(i,".vuo$",".pdf")
pandocHTML.target = VuoManual/index.xhtml
QMAKE_EXTRA_TARGETS += pandocHTML
POST_TARGETDEPS += VuoManual/index.xhtml
OTHER_FILES += \
	VuoManual.css \
	VuoManual.xsl \
	VuoManualHeader.xhtml \
	VuoManualNavigation.xhtml \
	latexToDocbook.php \
	transformDocbook.php



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
