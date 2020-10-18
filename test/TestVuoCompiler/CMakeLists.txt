add_library(TestVuoCompiler STATIC EXCLUDE_FROM_ALL
	TestVuoCompiler.cc
)
set_target_properties(TestVuoCompiler PROPERTIES
	AUTOMOC ON
)
target_include_directories(TestVuoCompiler
	PUBLIC
	.
)
target_compile_definitions(TestVuoCompiler
	PRIVATE
	BINARY_DIR="${PROJECT_BINARY_DIR}"
)
target_link_libraries(TestVuoCompiler
	PUBLIC
	Qt5::Test
	VuoFrameworkDylib
)
VuoPCH(TestVuoCompiler QTestVuoFramework)

VuoCompileNodes(
	node-TestVuoCompiler/vuo.test.compatibleWith1011.c
	node-TestVuoCompiler/vuo.test.compatibleWith1011And1012.c
	node-TestVuoCompiler/vuo.test.compatibleWith1011AndUp.c
	node-TestVuoCompiler/vuo.test.details.c
	node-TestVuoCompiler/vuo.test.inputDataPortOrder.c
	node-TestVuoCompiler/vuo.test.keywords.c
	node-TestVuoCompiler/vuo.test.multiDigitGenericTypes.c
	node-TestVuoCompiler/vuo.test.triggerCarryingInteger.c
	node-TestVuoCompiler/vuo.test.triggerWithOutput.c
	node-TestVuoCompiler/vuo.test.unicodeDefaultString.c
)
add_dependencies(TestVuoCompiler TestVuoCompiler.nodes)
