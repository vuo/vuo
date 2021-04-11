if (NOT CMAKE_SCRIPT_MODE_FILE)

	# Create a test that runs this CMakeLists.txt file as a script (below).
	add_test(NAME TestTests
		COMMAND ${CMAKE_COMMAND}
			-DPROJECT_BINARY_DIR=${PROJECT_BINARY_DIR}
			-P ${CMAKE_CURRENT_LIST_FILE}
	)

else()

	# Ensure that each QTestLib datum has a unique name.
	execute_process(
		WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
		COMMAND ctest -N
		COMMAND cut -d: -f 2-
		COMMAND sort
		COMMAND uniq -c
		COMMAND sort -rn
		COMMAND grep -E -v "^ +1 "
		OUTPUT_VARIABLE out
		ERROR_VARIABLE out
	)
	if (out)
		message(FATAL_ERROR "Some QTestLib tests have duplicate row names:\n${out}")
	endif()

endif()
