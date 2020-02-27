add_library(TestCompositionExecution STATIC EXCLUDE_FROM_ALL
	TestCompositionExecution.cc
)
set_target_properties(TestCompositionExecution PROPERTIES
	AUTOMOC ON
)
target_include_directories(TestCompositionExecution
	PUBLIC
	.
)
target_compile_definitions(TestCompositionExecution
	PRIVATE
	BINARY_DIR="${PROJECT_BINARY_DIR}"
)
target_link_libraries(TestCompositionExecution
	PUBLIC
	Qt5::Test
	VuoFrameworkDylib
)
VuoPCH(TestCompositionExecution QTestVuoFramework)
