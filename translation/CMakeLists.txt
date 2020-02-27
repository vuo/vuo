set(languages
	de
	es
	fr

	# Even though the strings in the source code are already in English,
	# we need to bundle this so that English is chosen
	# if the user selects it first in a list of multiple system languages.
	en
)

function (getLUpdateCommand var language tsFile)
	set(${var} ${PROJECT_BINARY_DIR}/bin/lupdate
		-no-obsolete
		-extensions c,cc,h,hh,m,mm,qml,qrc,ui
		-source-language POSIX
		-target-language ${language}
		-locations relative

		# Source folders to recursively scan:
		${PROJECT_SOURCE_DIR}/renderer
		${PROJECT_SOURCE_DIR}/editor
		${PROJECT_SOURCE_DIR}/translation
		${PROJECT_SOURCE_DIR}/type/inputEditor

		-ts ${tsFile}
		PARENT_SCOPE
	)
endfunction()


# Extract translatable strings from Vuo's source code.
foreach (language ${languages})
	set(tsFile ${CMAKE_CURRENT_SOURCE_DIR}/vuo_${language}.ts)
	getLUpdateCommand(lupdateCommand ${language} ${tsFile})
	if (${language} STREQUAL "en")
		set(pluralonly -pluralonly)
	endif()
	add_custom_command(
		COMMAND ${lupdateCommand} ${pluralonly}
		OUTPUT ${tsFile}-extracted
	)
	list(APPEND tsFilesExtracted ${tsFile}-extracted)
endforeach()
add_custom_target(VuoTranslationExtract DEPENDS ${tsFilesExtracted})


# Export to a `.po` file on the Desktop.
# Can't use `vuo_en.ts` from the source tree, since it contains only plurals.
set(tsFile ${CMAKE_CURRENT_BINARY_DIR}/vuo.ts)
set(poFile $ENV{HOME}/Desktop/vuo.po)
getLUpdateCommand(lupdateCommand en ${tsFile})
add_custom_command(
	COMMENT "Exporting to ${poFile}"
	COMMAND ${lupdateCommand}
	COMMAND ts2po
		--input ${tsFile}
		--output ${poFile}
	OUTPUT ${poFile}
)
list(APPEND poFiles ${poFile})
add_custom_target(VuoTranslationExport DEPENDS ${poFiles})


# Import from `.po` files in the Downloads folder.
foreach (language ${languages})
	set(tsFile ${CMAKE_CURRENT_SOURCE_DIR}/vuo_${language}.ts)
	set(poFile $ENV{HOME}/Downloads/vuo_${language}.po)
	getLUpdateCommand(lupdateCommand ${language} ${tsFile})
	add_custom_command(
		DEPENDS ${tsFile}
		COMMENT "Importing from ${poFile}"
		COMMAND [ -f "${poFile}" ]
			&& po2ts
				--input ${poFile}
				--output ${tsFile}-imported
			&& mv ${tsFile}-imported ${tsFile}

			# Use xmlstarlet to re-add the source and destination language in the header.
			&& xmlstarlet edit --inplace
				--insert /TS --type attr -n language -v ${language}
				--insert /TS --type attr -n sourcelanguage -v POSIX
				${tsFile}

			# Make the newly-created `.ts` file's apostrophe-encoding match Qt's apostrophe-encoding.
			&& perl -pi -e "s/\"\'\"/\\&apos\;/g" ${tsFile}

			# Use lupdate to re-add the source file/line context.
			&& ${lupdateCommand}

			|| echo "${poFile} does not exist - skipping."
		COMMAND
		OUTPUT ${tsFile}-imported
	)
	list(APPEND tsFilesImported ${tsFile}-imported)
endforeach()
add_custom_target(VuoTranslationImport DEPENDS ${tsFilesImported})


# Compile translations for distribution.
foreach (language ${languages})
	set(tsFile ${CMAKE_CURRENT_SOURCE_DIR}/vuo_${language}.ts)
	set(qmFile ${CMAKE_CURRENT_BINARY_DIR}/vuo_${language}.qm)
	add_custom_command(
		DEPENDS ${tsFile}
		COMMENT "Compiling ${qmFile}"
		COMMAND ${PROJECT_BINARY_DIR}/bin/lrelease
			-removeidentical
			${tsFile}
			-qm ${qmFile}
		OUTPUT ${qmFile}
	)
	list(APPEND qmFiles ${qmFile})
endforeach()
add_custom_target(VuoTranslationCompile DEPENDS ${qmFiles})
set_target_properties(VuoTranslationCompile PROPERTIES qmFiles "${qmFiles}")
