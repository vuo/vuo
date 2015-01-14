TEMPLATE = subdirs
QMAKE_CLEAN = Makefile

include(../vuo.pri)

cache()

# Subdirs are listed in the order that the tests should run.
SUBDIRS += \
	test_TestVuoUtilities \
	test_TestVuoRunner \
	test_TestVuoCompiler \
	test_TestVuoCompiler_node \
	test_TestVuoCompilerModule \
	test_TestVuoCompilerType \
	test_TestVuoCompilerNodeClass \
	test_TestVuoCompilerNode \
	test_TestVuoCompilerComposition \
	test_TestVuoCompilerGraphvizParser \
	test_TestVuoCompilerBitcodeGenerator \
	test_TestVuoRenderer \
	test_TestVuoTypes \
	test_TestCompositionExecution \
	test_TestControlAndTelemetry \
	test_TestControlAndTelemetry_node \
	test_TestNodeExecutionOrder \
	test_TestNodeExecutionOrder_node \
	test_TestReferenceCounting \
	test_TestReferenceCounting_node \
	test_TestCompositionOutput \
	test_TestCompositionOutput_node \
	test_TestBuiltProducts

test_TestBuiltProducts.subdir = TestBuiltProducts
test_TestBuiltProducts.depends =

test_TestControlAndTelemetry.subdir = TestControlAndTelemetry
test_TestControlAndTelemetry.depends = test_TestCompositionExecution

test_TestControlAndTelemetry_node.subdir = TestControlAndTelemetry/node-TestControlAndTelemetry
test_TestControlAndTelemetry_node.depends =

test_TestCompositionExecution.subdir = TestCompositionExecution
test_TestCompositionExecution.depends = test_TestVuoCompiler_node

test_TestCompositionOutput.subdir = TestCompositionOutput
test_TestCompositionOutput.depends = test_TestCompositionExecution

test_TestCompositionOutput_node.subdir = TestCompositionOutput/node-TestCompositionOutput
test_TestCompositionOutput_node.depends =

test_TestNodeExecutionOrder.subdir = TestNodeExecutionOrder
test_TestNodeExecutionOrder.depends = test_TestCompositionExecution

test_TestNodeExecutionOrder_node.subdir = TestNodeExecutionOrder/node-TestNodeExecutionOrder
test_TestNodeExecutionOrder_node.depends =

test_TestReferenceCounting.subdir = TestReferenceCounting
test_TestReferenceCounting.depends = test_TestCompositionExecution

test_TestReferenceCounting_node.subdir = TestReferenceCounting/node-TestReferenceCounting
test_TestReferenceCounting_node.depends =

test_TestVuoCompiler_node.subdir = TestVuoCompiler/node-TestVuoCompiler
test_TestVuoCompiler_node.depends =

test_TestVuoCompiler.subdir = TestVuoCompiler
test_TestVuoCompiler.depends = test_TestVuoCompiler_node

test_TestVuoCompilerBitcodeGenerator.subdir = TestVuoCompilerBitcodeGenerator
test_TestVuoCompilerBitcodeGenerator.depends = test_TestVuoCompiler

test_TestVuoCompilerComposition.subdir = TestVuoCompilerComposition
test_TestVuoCompilerComposition.depends = test_TestVuoCompiler

test_TestVuoCompilerGraphvizParser.subdir = TestVuoCompilerGraphvizParser
test_TestVuoCompilerGraphvizParser.depends = test_TestVuoCompiler

test_TestVuoCompilerModule.subdir = TestVuoCompilerModule
test_TestVuoCompilerModule.depends = test_TestVuoCompiler

test_TestVuoCompilerNode.subdir = TestVuoCompilerNode
test_TestVuoCompilerNode.depends = test_TestVuoCompiler

test_TestVuoCompilerNodeClass.subdir = TestVuoCompilerNodeClass
test_TestVuoCompilerNodeClass.depends = test_TestVuoCompiler

test_TestVuoCompilerType.subdir = TestVuoCompilerType
test_TestVuoCompilerType.depends = test_TestVuoCompiler

test_TestVuoRenderer.subdir = TestVuoRenderer
test_TestVuoRenderer.depends =

test_TestVuoRunner.subdir = TestVuoRunner
test_TestVuoRunner.depends =

test_TestVuoTypes.subdir = TestVuoTypes
test_TestVuoTypes.depends =

test_TestVuoUtilities.subdir = TestVuoUtilities
test_TestVuoUtilities.depends =


exists(TestVuoEditor) {
	SUBDIRS += \
		test_TestVuoEditor

	test_TestVuoEditor.subdir = TestVuoEditor
}
