if (NOT CMAKE_SCRIPT_MODE_FILE)

	# Create a test that runs this CMakeLists.txt file as a script (below).
	add_test(NAME TestDocumentation
		COMMAND ${CMAKE_COMMAND}
			-DPROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}
			-DVuo_VERSION=${Vuo_VERSION}
			-P ${CMAKE_CURRENT_LIST_FILE}
	)

else()

	# See TestNodes for tests of deprecated nodes in documentation compositions.
	# See TestVuoEditor for tests of menu items in VuoManual.txt.

	# Ensure the links in the PDF manual aren't broken.
	execute_process(
		# Test the pre-compressed (VuoPackagePDFManual) version,
		# since pdf-link-checker can't parse the compressed version.
		COMMAND pdf-link-checker ../../documentation/VuoManual-en-solarized.pdf
		OUTPUT_VARIABLE out
		ERROR_VARIABLE out)
	string(REPLACE "\n" ";" out "${out}")
	cmake_policy(SET CMP0007 NEW)
	string(JOIN "|" regexes
		# Ignore errors due to this not-yet-released Vuo version.
		api.vuo.org/${Vuo_VERSION}/.*404
		doc.vuo.org/${Vuo_VERSION}/.*404
		# Ignore sites that block our link checker.
		leapmotion.com.*403
		503: Service Unavailable
		# Ignore temporary failures.
		Reason: unknown
	)
	list(FILTER out EXCLUDE REGEX ${regexes})
	list(SORT out)
	list(LENGTH out outLen)
	if (outLen GREATER 0)
		string(REPLACE ";" "\n" out "${out}")
		message(FATAL_ERROR "error: Found broken links in the PDF manual:\n${out}")
	endif()

	# Check for potentially harmful language.
	if (EXISTS /usr/local/bin/alex)
		execute_process(
			WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
			COMMAND /usr/local/bin/alex --quiet --why .
			RESULT_VARIABLE ret
		)
		if (ret)
			message(FATAL_ERROR "error: alex reported issues.")
		endif()
	else()
		message("Can't run Alex since it doesn't exist on this system.")
	endif()

endif()
