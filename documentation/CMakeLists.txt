add_custom_target(VuoDocumentation)


file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/image-generated)
VuoRender(VuoDocumentationNodesAndCompositions
	vuo.console.window
	vuo.event.changed2
	vuo.image.color.adjust
	vuo.image.fetch
	vuo.math.count
	vuo.math.scale
	vuo.scene.fetch
	vuo.scene.render.window
	vuo.time.firePeriodically2
	vuo.video.play
	vuo.video.save

	${PROJECT_SOURCE_DIR}/documentation/composition/2Recur.vuo
	${PROJECT_SOURCE_DIR}/documentation/composition/AreAllValuesTrue.vuo
	${PROJECT_SOURCE_DIR}/documentation/composition/BeepWhenMouseEntersSquare.vuo
	${PROJECT_SOURCE_DIR}/documentation/composition/BlendImages.vuo
	${PROJECT_SOURCE_DIR}/documentation/composition/BuildColoredGrid.vuo
	${PROJECT_SOURCE_DIR}/documentation/composition/CalculateBoxVolume.vuo
	${PROJECT_SOURCE_DIR}/documentation/composition/CalculateVolume.vuo
	${PROJECT_SOURCE_DIR}/documentation/composition/CaptionVideo.vuo
	${PROJECT_SOURCE_DIR}/documentation/composition/ChangeBackgroundColor.vuo
	${PROJECT_SOURCE_DIR}/documentation/composition/ChangeTilingPeriodically.vuo
	${PROJECT_SOURCE_DIR}/documentation/composition/Count.vuo
	${PROJECT_SOURCE_DIR}/documentation/composition/CountAndHoldMousePresses.vuo
	${PROJECT_SOURCE_DIR}/documentation/composition/CountCharactersToConsole.vuo
	${PROJECT_SOURCE_DIR}/documentation/composition/CountIsGreaterThanToConsole.vuo
	${PROJECT_SOURCE_DIR}/documentation/composition/CountIsLessThanToConsole.vuo
	${PROJECT_SOURCE_DIR}/documentation/composition/CountScatter.vuo
	${PROJECT_SOURCE_DIR}/documentation/composition/DetectBarcode.vuo
	${PROJECT_SOURCE_DIR}/documentation/composition/DetectCamera.vuo
	${PROJECT_SOURCE_DIR}/documentation/composition/DisplayAndTwirlHelloWorldContinuously.vuo
	${PROJECT_SOURCE_DIR}/documentation/composition/DisplayAndTwirlHelloWorldImage.vuo
	${PROJECT_SOURCE_DIR}/documentation/composition/DisplayHelloWorldImage.vuo
	${PROJECT_SOURCE_DIR}/documentation/composition/DisplayHelloWorldNoEvent.vuo
	${PROJECT_SOURCE_DIR}/documentation/composition/DisplayMultipleWindows.vuo
	${PROJECT_SOURCE_DIR}/documentation/composition/FetchOnlyWhenURLChanges.vuo
	${PROJECT_SOURCE_DIR}/documentation/composition/FilterImage-OutlineImage.vuo
	${PROJECT_SOURCE_DIR}/documentation/composition/FilterImage-blank.vuo
	${PROJECT_SOURCE_DIR}/documentation/composition/GraduallyMoveCircle.vuo
	${PROJECT_SOURCE_DIR}/documentation/composition/InvertMovieColors.vuo
	${PROJECT_SOURCE_DIR}/documentation/composition/MoveCircleWithMouse.vuo
	${PROJECT_SOURCE_DIR}/documentation/composition/MultipleEventOnlyCables.vuo
	${PROJECT_SOURCE_DIR}/documentation/composition/OutlineVideo.vuo
	${PROJECT_SOURCE_DIR}/documentation/composition/RememberMousePresses.vuo
	${PROJECT_SOURCE_DIR}/documentation/composition/RespondToMagicWord.vuo
	${PROJECT_SOURCE_DIR}/documentation/composition/SelectColor.vuo
	${PROJECT_SOURCE_DIR}/documentation/composition/SelectLatestInput.vuo
	${PROJECT_SOURCE_DIR}/documentation/composition/ShareHeight.vuo
	${PROJECT_SOURCE_DIR}/documentation/composition/SpeakPeriodically.vuo
	${PROJECT_SOURCE_DIR}/documentation/composition/SumAddOne.vuo
	${PROJECT_SOURCE_DIR}/documentation/composition/SwitchControllees.vuo
	${PROJECT_SOURCE_DIR}/documentation/composition/SwitchControllers.vuo
	${PROJECT_SOURCE_DIR}/node/vuo.data/examples/StoreMousePosition.vuo
	${PROJECT_SOURCE_DIR}/node/vuo.data/examples/TwirlImageRepeatedly.vuo
	${PROJECT_SOURCE_DIR}/node/vuo.event/examples/LoadImageAsynchronously.vuo
	${PROJECT_SOURCE_DIR}/node/vuo.image/examples/DisplayImage.vuo
	${PROJECT_SOURCE_DIR}/node/vuo.list/examples/CycleSeasons.vuo
	${PROJECT_SOURCE_DIR}/node/vuo.list/examples/DisplayGridOfImages.vuo
	${PROJECT_SOURCE_DIR}/node/vuo.list/examples/DisplayRainbowOvals.vuo
	${PROJECT_SOURCE_DIR}/node/vuo.list/examples/ReplaceColorsInGradient.vuo
	${PROJECT_SOURCE_DIR}/node/vuo.logic/examples/IsMouseWithinIntersectingRectangles.vuo
	${PROJECT_SOURCE_DIR}/node/vuo.motion/examples/SpringBack.vuo
	${PROJECT_SOURCE_DIR}/node/vuo.motion/examples/WaveCircle.vuo
	${PROJECT_SOURCE_DIR}/node/vuo.scene/examples/MoveSpinningSphere.vuo
	${PROJECT_SOURCE_DIR}/node/vuo.select/examples/SelectStripes.vuo
	${PROJECT_SOURCE_DIR}/node/vuo.select/examples/ShowArrowPresses.vuo
	${PROJECT_SOURCE_DIR}/node/vuo.time/examples/AnimateOnSchedule.vuo
	${PROJECT_SOURCE_DIR}/node/vuo.time/examples/FlashOnMousePress.vuo
	${PROJECT_SOURCE_DIR}/node/vuo.ui/examples/ChangeColorWithButton.vuo
)


# Build the Doxygen API documentation ========================================
if (VuoPro)
	set(DoxyfilePro "@INCLUDE = pro/Doxyfile")
endif()
configure_file(Doxyfile.in Doxyfile)
add_custom_command(
	DEPENDS
		${CMAKE_BINARY_DIR}/editor/VuoNodeClassFrequencyMap.hh
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/documentation
	COMMAND /usr/local/bin/doxygen ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
		2>&1
		# New warnings in Doxygen 1.8.16 and 1.8.18; seem to be harmless.
		| (grep -v
			-e "warning: multiple use of page label .Changes_2_0_0"
			-e "warning: unexpected token TK_COMMAND_BS as the argument of _internalref"
			|| true)
	OUTPUT doxygen/index.html
)
add_custom_target(VuoDoxygen DEPENDS doxygen/index.html)
add_dependencies(VuoDocumentation VuoDoxygen)


# Build the PDF manual =======================================================
get_directory_property(languages DIRECTORY ../translation DEFINITION languages)
foreach (language ${languages})
	foreach (colors solarized highContrast)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/VuoManualPDF-${language}-${colors})
set(xelatex echo Running xelatex && set -o pipefail && max_print_line=1024 PATH=/Library/TeX/texbin xelatex -halt-on-error -file-line-error VuoManual.tex
	| \(egrep -v '^$$|This is XeTeX|LaTeX Font Warning: Font shape.*undefined|using.*instead on input line|LaTeX Font Warning: Some font shapes were not available|Snakes have been superseded by decorations|LaTeX Warning: Label.* may have changed. Rerun to get cross-references right|Package rerunfilecheck Warning: File .* has changed|LaTeX Warning: Marginpar on page .* moved|Package mparhack Warning: Marginpars may have changed|restricted .write18 enabled|fontencoding T3 patched|entering extended mode|^ *\[[0-9]*\]|Transcript written on|Overfull .[hv]box|Underfull .[hv]box|EveryShipout initializing macros|geometry. detected driver|geometry. driver|to be read again|Standard LaTeX document class|For additional information on|LaTeX2e .[0-9-]+|Output written on|Writing index file|^\\\([/.]|^.TU|^ *.$$|see the transcript file for additional information|AED: lastpage setting LastPage|Babel.*and hyphenation patterns for.*loaded' || true\) )
add_custom_command(
	DEPENDS
		VuoEditorApp # for CONTRIBUTORS.md
		VuoManual.txt
		VuoManualColors-${colors}.tex
		VuoManualHeader.tex
		VuoDocumentationNodesAndCompositions

	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/VuoManualPDF-${language}-${colors}

	COMMAND cp ${PROJECT_SOURCE_DIR}/documentation/VuoManualColors-${colors}.tex VuoManualColors.tex

	COMMAND echo Running pandoc
		&& cat ${PROJECT_SOURCE_DIR}/documentation/VuoManual.txt ${PROJECT_BINARY_DIR}/bin/Vuo.app/Contents/Resources/CONTRIBUTORS.md
		| perl -pe "s/VUO_VERSION/${Vuo_VERSION}/g"
		| fgrep -v '<abstract>'
		| ${PROJECT_SOURCE_DIR}/documentation/VuoManualTranslate.php
			${language}
			${PROJECT_SOURCE_DIR}/translation/qtbase_${language}.ts
			${PROJECT_SOURCE_DIR}/translation/vuo_${language}.ts
		| /usr/local/Cellar/pandoc/1.12.0.2/bin/pandoc
			--table-of-contents
			--number-sections
			--smart
			--template=${PROJECT_SOURCE_DIR}/documentation/VuoManualTemplate.tex
			--include-in-header=${PROJECT_SOURCE_DIR}/documentation/VuoManualHeader.tex
			--from=markdown
			--to=latex
			-o -
			-
		| perl -pe "s^PROJECT_SOURCE_DIR^${PROJECT_SOURCE_DIR}^g"
		> VuoManual.tex

	# The first xelatex run gathers the names of terms and references,
	# so there will be a lot of harmless undefined reference warnings.
	# xelatex exits with nonzero status even when it succeeds.
	COMMAND ${xelatex}
		| \(egrep -v '[Rr]eference.*on page.*undefined on input line|No file VuoManual.aux|No file VuoManual.gls|Rerun to get|rerunfilecheck|There were undefined references' || true\)

	# The second xelatex run populates terms and references (including the table of contents).
	# There shouldn't be any undefined references after the second run.
	COMMAND ${xelatex}
		| \(egrep -v 'No file VuoManual.gls|mparhack.*Rerun to get them right' || true\)

	# Now that the table of contents is present (which shifts the page numbers back a few),
	# generate the glossary's page number links.
	COMMAND echo Running makeglossaries
		&& PATH=/usr/bin:/Library/TeX/texbin makeglossaries -q VuoManual

	# The third xelatex run includes the glossary,
	# and fixes up page numbers now that the table of contents is populated.
	COMMAND ${xelatex}
		| \(egrep -v 'mparhack.*Rerun to get them right|rerunfilecheck.*Rerun to get outlines right|rerunfilecheck.*or use package .bookmark' || true\)

	# The fourth xelatex run allows the glossary to be included in the table of contents.
	COMMAND ${xelatex}

	COMMAND mv VuoManual.pdf ${CMAKE_CURRENT_BINARY_DIR}/VuoManual-${language}-${colors}.pdf

	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/VuoManual-${language}-${colors}.pdf
)
add_custom_target(VuoManualPDF_${language}_${colors} DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/VuoManual-${language}-${colors}.pdf)
add_dependencies(VuoDocumentation VuoManualPDF_${language}_${colors})
	endforeach()
endforeach()


# Convert PDFs to PNG for HTML manual ========================================
set(ghostscriptFlags
	-dBATCH
	-q
	-dQUIET
	-dSAFER
	-dNOPAUSE
	-dNOPROMPT
)
file(MAKE_DIRECTORY
	${CMAKE_CURRENT_BINARY_DIR}/VuoManualTemp
	${CMAKE_CURRENT_BINARY_DIR}/VuoManual
	${CMAKE_CURRENT_BINARY_DIR}/VuoManual/image
	${CMAKE_CURRENT_BINARY_DIR}/VuoManual/image-generated
)
file(GLOB pdfs image/*.pdf)
foreach (pdf ${pdfs})
	get_filename_component(pdfName ${pdf} NAME_WLE)
	add_custom_command(
		DEPENDS ${pdf}

		COMMENT "Converting ${pdfName} to PNG"

		# Workaround for apparent Ghostscript bug where it deletes the character "i" from the embedded font.
		COMMAND /usr/local/bin/gs ${ghostscriptFlags}
			-sDEVICE=pdfwrite
			-dEmbedAllFonts=false
			-dAutoFilterColorImages=false
			-dColorImageFilter=/FlateEncode
			-o VuoManualTemp/${pdfName}-nofont.pdf
			${pdf}

		# Convert PDF to PNG
		COMMAND /usr/local/bin/gs ${ghostscriptFlags}
			-sDEVICE=pngalpha
			-dMaxBitmap=2147483647
			-dAlignToPixels=0
			-dGridFitTT=2
			-dTextAlphaBits=4
			-dGraphicsAlphaBits=4
			-r144x144
			-o VuoManualTemp/${pdfName}-nofont.png
			VuoManualTemp/${pdfName}-nofont.pdf

		# Reduce size of PNG files
		COMMAND /usr/local/bin/pngquant
			--force
			--speed 1
			--output VuoManual/image-generated/${pdfName}.png
			VuoManualTemp/${pdfName}-nofont.png

		OUTPUT VuoManual/image-generated/${pdfName}.png
	)
	list(APPEND pngs VuoManual/image-generated/${pdfName}.png)
endforeach()
add_custom_target(VuoDocumentationPNGs DEPENDS ${pngs})
add_dependencies(VuoDocumentation VuoDocumentationPNGs)


# Build the HTML manual ======================================================
execute_process(OUTPUT_VARIABLE docbookXSLVersion
	COMMAND brew list --versions docbook-xsl
	COMMAND cut -d " " -f 2
	COMMAND head -1
	OUTPUT_STRIP_TRAILING_WHITESPACE
)
set(docbookXSLPath /usr/local/Cellar/docbook-xsl/${docbookXSLVersion})
configure_file(VuoManual.xsl.in VuoManual.xsl)
add_custom_command(
	DEPENDS
		VuoEditorApp # for CONTRIBUTORS.md
		VuoManual.css
		VuoManual.txt
		VuoDocumentationNodesAndCompositions
		VuoDocumentationPNGs

	WORKING_DIRECTORY VuoManual

	COMMAND cp
		${CMAKE_CURRENT_SOURCE_DIR}/VuoManual.css
		${CMAKE_CURRENT_SOURCE_DIR}/VuoManualTOC.js
		.
	COMMAND cp
		${CMAKE_CURRENT_SOURCE_DIR}/image/*.png
		${CMAKE_CURRENT_SOURCE_DIR}/image/*.svg
		image
	COMMAND cp ../image-generated/*.png image-generated

	COMMAND cat ${CMAKE_CURRENT_SOURCE_DIR}/VuoManual.txt ${PROJECT_BINARY_DIR}/bin/Vuo.app/Contents/Resources/CONTRIBUTORS.md
		| perl -pe "s/VUO_VERSION/${Vuo_VERSION}/g"

		# Markdown -> JSON
		| /usr/local/Cellar/pandoc/1.12.0.2/bin/pandoc
			--smart
			--from markdown
			--to json
			-o -
			-

		# JSON -> JSON, changing LaTeX commands into Docbook-compatible XML encoded in JSON
		| ${CMAKE_CURRENT_SOURCE_DIR}/latexToDocbook.php

		# JSON -> Docbook
		| /usr/local/Cellar/pandoc/1.12.0.2/bin/pandoc
			--standalone
			--from json
			--to docbook
			-o -
			-

		# Docbook -> Docbook, changing the DTD and other stuff Pandoc doesn't let us change
		| ${CMAKE_CURRENT_SOURCE_DIR}/transformDocbook.php ${Vuo_VERSION}

		# Docbook -> HTML
		| xsltproc
			--nonet
			--stringparam chunk.quietly 1
			--stringparam chunk.section.depth 2
			--stringparam chunk.tocs.and.lots 1
			--stringparam generate.meta.abstract 1
			--stringparam html.stylesheet VuoManual.css
			--stringparam para.propagates.style 1
			--stringparam phrase.propagates.style 1
			--stringparam toc.section.depth 2
			--stringparam use.id.as.filename 1
			../VuoManual.xsl
			-

	OUTPUT VuoManual/index.xhtml
)
add_custom_target(VuoManualHTML DEPENDS VuoManual/index.xhtml)
add_dependencies(VuoDocumentation VuoManualHTML)


# Build the MacHelp manual ===================================================
file(MAKE_DIRECTORY
	${CMAKE_CURRENT_BINARY_DIR}/Vuo.help
	${CMAKE_CURRENT_BINARY_DIR}/Vuo.help/Contents
	${CMAKE_CURRENT_BINARY_DIR}/Vuo.help/Contents/Resources
	${CMAKE_CURRENT_BINARY_DIR}/Vuo.help/Contents/Resources/image
	${CMAKE_CURRENT_BINARY_DIR}/Vuo.help/Contents/Resources/image-generated
)
configure_file(VuoManualHelp.xsl.in VuoManualHelp.xsl)
configure_file(MacHelp-Info.plist Vuo.help/Contents/Info.plist)
add_custom_command(
	DEPENDS
		VuoEditorApp # for CONTRIBUTORS.md
		VuoManual.css
		VuoManual.txt
		VuoDocumentationNodesAndCompositions
		VuoDocumentationPNGs

	WORKING_DIRECTORY Vuo.help/Contents/Resources

	COMMAND cp
		${CMAKE_CURRENT_SOURCE_DIR}/VuoManual.css
		${CMAKE_CURRENT_SOURCE_DIR}/VuoManualTOC.js
		.
	COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/MacHelp-access.html _access.html
	COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/image/vuo-60x60.png vuo.png
	COMMAND cp
		${CMAKE_CURRENT_SOURCE_DIR}/image/*.png
		${CMAKE_CURRENT_SOURCE_DIR}/image/*.svg
		image
	COMMAND cp
		${CMAKE_CURRENT_BINARY_DIR}/image-generated/*.png
		${CMAKE_CURRENT_BINARY_DIR}/VuoManual/image-generated/*.png
		image-generated

	COMMAND cat ${CMAKE_CURRENT_SOURCE_DIR}/VuoManual.txt ${PROJECT_BINARY_DIR}/bin/Vuo.app/Contents/Resources/CONTRIBUTORS.md
		| perl -pe "s/VUO_VERSION/${Vuo_VERSION}/g"

		# Markdown -> JSON
		| /usr/local/Cellar/pandoc/1.12.0.2/bin/pandoc
			--smart
			--from markdown
			--to json
			-o -
			-

		# JSON -> JSON, changing LaTeX commands into Docbook-compatible XML encoded in JSON
		| ${CMAKE_CURRENT_SOURCE_DIR}/latexToDocbook.php

		# JSON -> Docbook
		| /usr/local/Cellar/pandoc/1.12.0.2/bin/pandoc
			--standalone
			--from json
			--to docbook
			-o -
			-

		# Docbook -> Docbook, changing the DTD and other stuff Pandoc doesn't let us change
		| ${CMAKE_CURRENT_SOURCE_DIR}/transformDocbook.php ${Vuo_VERSION}

		# Docbook -> HTML
		| xsltproc
			--nonet
			--stringparam chunk.quietly 1
			--stringparam chunk.section.depth 2
			--stringparam chunk.tocs.and.lots 1
			--stringparam generate.meta.abstract 1
			--stringparam html.stylesheet VuoManual.css
			--stringparam para.propagates.style 1
			--stringparam phrase.propagates.style 1
			--stringparam toc.section.depth 2
			--stringparam use.id.as.filename 1
			../../../VuoManualHelp.xsl
			-

	COMMAND hiutil --verbose --create --anchors --file Vuo.helpindex --include ".*\.xhtml" .
		2>&1
		| \(grep -v "Error: anchor was not valid:  .__NSCFConstantString." || true\)

	# Check for chunks missing an abstract
	COMMAND egrep -l -r "meta name=.description. content=...+?. /" .
		| cut -b 3-
		> ${CMAKE_CURRENT_BINARY_DIR}/Vuo.help-filesWithAnAbstract
	COMMAND ls -1
		| egrep -v "'^\(ExactMatch.plist\|VuoManual.css\|VuoManualTOC.js\|image\|image-generated\|index.html\|ar01-toc.html\|resources-vuo-uses.html\|software-vuo-uses.html\|_access.html\|Vuo.helpindex\|vuo.png\)$$'"
		| \(grep -v -f ${CMAKE_CURRENT_BINARY_DIR}/Vuo.help-filesWithAnAbstract || true\)
		| perl -pe "'s/^\(.*\)$$/error: $$1 has no abstract/g'"

	OUTPUT Vuo.help/Contents/Resources/index.html
)
add_custom_target(VuoManualMacHelp DEPENDS Vuo.help/Contents/Resources/index.html)
add_dependencies(VuoDocumentation VuoManualMacHelp)
