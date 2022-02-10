/**
 * @file
 * TestTypes implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "TestCompositionExecution.hh"
#include <Vuo/Vuo.h>

/**
 * Tests each type for common mistakes and errors.
 */
class TestTypes : public TestCompositionExecution
{
	Q_OBJECT

private:
	QString singleTestDatum;
	VuoCompiler *compiler;

public:
	TestTypes(int argc, char **argv)
	{
		compiler = initCompiler();

		if (argc > 1)
		{
			// If a single test is specified on the command line,
			// improve performance by loading only data for that test.
			singleTestDatum = QString::fromUtf8(argv[1]).section(':', 1);
		}
	}
	~TestTypes()
	{
		delete compiler;
	}

private slots:
	/**
	 * Tests that each node class lists all necessary dependencies. If not, a composition that contains just that
	 * node class will fail to build.
	 *
	 * Tests that each generic node class successfully compiles when its generic port types are specialized.
	 */
	void testEachType_data()
	{
		QTest::addColumn<bool>("dummy");

		map<string, VuoCompilerType *> allTypes = compiler->getTypes();
		for (map<string, VuoCompilerType *>::iterator i = allTypes.begin(); i != allTypes.end(); ++i)
			QTest::newRow(i->first.c_str()) << true;
	}
	void testEachType()
	{
		printf("%s\n", QTest::currentDataTag()); fflush(stdout);

		string type = QTest::currentDataTag();
		string portIdentifier = VuoStringUtilities::buildPortIdentifier("ShareValue", "value");

		VuoCompilerIssues issues;
		VuoRunner *runner = VuoCompiler::newCurrentProcessRunnerFromCompositionString(
			"digraph G { ShareValue [type=\"vuo.data.share." + type + "\"]; }", ".", &issues);
		runner->start();


		// Test whether basic type functions can handle all the top-level JSON types without crashing.
		json_object *jsonTypes[] = {
			NULL,
			json_object_new_boolean(false),
			json_object_new_boolean(true),
			json_object_new_double(42.42),
			json_object_new_int(42),
			json_object_new_object(),
			json_object_new_array(),
			json_object_new_string("")
		};
		for (int i = 0; i < sizeof(jsonTypes)/sizeof(json_object *); ++i)
		{
//			VLog("%s",json_object_to_json_string(jsonTypes[i]));
			// Tests `VuoType_makeFromJson()`.
			runner->setInputPortValue("", portIdentifier, jsonTypes[i]);
			// Tests `VuoType_getJson()`.
			runner->getInputPortValue("", portIdentifier);
			// Tests `VuoType_getSummary()`.
			runner->getInputPortSummary("", portIdentifier);
		}


		runner->stop();
		delete runner;
	}

	void testLlvmTypes_data()
	{
		QTest::addColumn< QString >("typeName");

		initCompiler();
		if (singleTestDatum.isEmpty())
			for (auto i : compiler->getTypes())
				QTest::newRow(i.first.c_str()) << QString::fromStdString(i.first);
		else
			QTest::newRow(singleTestDatum.toUtf8().constData()) << singleTestDatum;
	}
	void testLlvmTypes()
	{
		QFETCH(QString, typeName);

		VuoCompilerType *type = compiler->getType(typeName.toStdString());

		Module *module = new Module("", *VuoCompiler::globalLLVMContext);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), false);
		Function *function = Function::Create(functionType, GlobalValue::ExternalLinkage, "test", module);
		BasicBlock *block = BasicBlock::Create(*VuoCompiler::globalLLVMContext, "", function, nullptr);

		Value *emptyString = VuoCompilerCodeGenUtilities::generatePointerToConstantString(module, "");
		Value *value = type->generateRetainedValueFromString(module, block, emptyString);  // tests llvmReturnType

		Value *pointerToValue = VuoCompilerCodeGenUtilities::generatePointerToValue(block, value);
		vector<Value *> args = type->convertPortDataToArgs(module, block, pointerToValue, type->getSummaryFunction->getFunctionType(), 0, false);  // tests llvmArgumentType
		QVERIFY2(args.size() == 1 || args.size() == 2, qPrintable(QString("%1 should be 1 or 2").arg(args.size())));

		size_t argsTotalSize = 0;
		for (Value *arg : args)
			argsTotalSize += module->getDataLayout().getTypeAllocSize(arg->getType());
		QVERIFY2(argsTotalSize <= type->getAllocationSize(module), qPrintable(QString("%1 should be <= %2").arg(argsTotalSize).arg(type->getAllocationSize(module))));

		CallInst::Create(type->getSummaryFunction, args, "", block);

		delete module;
	}

};

int main(int argc, char *argv[])
{
	TestTypes tc(argc, argv);
	return QTest::qExec(&tc, argc, argv);
}

#include "TestTypes.moc"
