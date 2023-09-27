/**
 * @file
 * TestTypes implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
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

public:
	TestTypes(int argc, char **argv)
	{
		if (argc > 1)
		{
			// If a single test is specified on the command line,
			// improve performance by loading only data for that test.
			singleTestDatum = QString::fromUtf8(argv[1]).section(':', 1);
		}
	}

private slots:
	void testEachType_data()
	{
		QTest::addColumn<QString>("typeName");

		VuoCompiler *compiler = initCompiler("/TestTypes/testEachType");
		VuoDefer(^{ delete compiler; });

		map<string, VuoCompilerType *> allTypes = compiler->getTypes();
		for (map<string, VuoCompilerType *>::iterator i = allTypes.begin(); i != allTypes.end(); ++i)
		{
			string typeName = i->first;
			if (! VuoType::isListTypeName(typeName) && typeName != "VuoList")
			{
				QTest::newRow(typeName.c_str()) << QString::fromStdString(typeName);
				if (! VuoType::isDictionaryTypeName(typeName) && typeName != "VuoMathExpressionList")
					QTest::newRow((VuoType::listTypeNamePrefix + typeName).c_str()) << QString::fromStdString(VuoType::listTypeNamePrefix + typeName);
			}
		}

		QTest::newRow("VuoList_VuoGenericType1") << "VuoList_VuoGenericType1";
	}
	void testEachType()
	{
		QFETCH(QString, typeName);
		VUserLog("%s", typeName.toUtf8().constData());

		VuoCompiler *compiler = initCompiler("/TestTypes/testEachType");
		VuoDefer(^{ delete compiler; });

		string genericNodeClassName = VuoType::isListTypeName(typeName.toStdString()) ? "vuo.data.share.list" : "vuo.data.share";
		string innerTypeName = VuoType::extractInnermostTypeName(typeName.toStdString());
		VuoCompilerNodeClass *nodeClass = compiler->getNodeClass(genericNodeClassName + "." + innerTypeName);

		string composition = wrapNodeInComposition(nodeClass, compiler);

		// Test whether the type includes all necessary headers and dependencies.

		VuoCompilerIssues issues;
		VuoRunner *runner = VuoCompiler::newSeparateProcessRunnerFromCompositionString(composition, "testEachType", "", &issues);
		QVERIFY2(runner, issues.getLongDescription(false).c_str());
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

		string nodeIdentifier = VuoType::isListTypeName(typeName.toStdString()) ? "ShareList" : "ShareValue";
		string portIdentifier = VuoStringUtilities::buildPortIdentifier(nodeIdentifier, "value");

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

		VuoCompiler *compiler = initCompiler("/TestTypes/testLlvmTypes");
		VuoDefer(^{ delete compiler; });

		if (singleTestDatum.isEmpty())
			for (auto i : compiler->getTypes())
				QTest::newRow(i.first.c_str()) << QString::fromStdString(i.first);
		else
			QTest::newRow(singleTestDatum.toUtf8().constData()) << singleTestDatum;
	}
	void testLlvmTypes()
	{
		QFETCH(QString, typeName);

		VuoCompiler *compiler = initCompiler("/TestTypes/testLlvmTypes");
		VuoDefer(^{ delete compiler; });

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
