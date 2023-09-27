/**
 * @file
 * TestVuoCompilerType interface and implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "TestVuoCompiler.hh"

// Be able to use these types in QTest::addColumn()
Q_DECLARE_METATYPE(Type::TypeID);

/**
 * Tests for using types.
 */
class TestVuoCompilerType : public TestVuoCompiler
{
	Q_OBJECT

private slots:

	void initTestCase()
	{
		initCompiler();
	}

	void cleanupTestCase()
	{
		cleanupCompiler();
	}

	void testLoadingTypes()
	{
		VuoCompilerType *type = compiler->getType("VuoInteger");
		QVERIFY(type != NULL);

		string title = type->getBase()->getDefaultTitle();
		QCOMPARE(QString(title.c_str()), QString("Integer"));
	}

	void testLlvmTypes_data()
	{
		QTest::addColumn< QString >("typeName");
		QTest::addColumn< Type::TypeID >("argumentTypeId");
		QTest::addColumn< Type::TypeID >("secondArgumentTypeId");
		QTest::addColumn< Type::TypeID >("returnTypeId");
		QTest::addColumn< bool >("isReturnPassedAsArgument");

		// This only tests types that are lowered the same on x86_64 and arm64. TestTypes::testLlvmTypes tests all types.
		QTest::newRow("integer") << "VuoInteger" << Type::IntegerTyID << Type::VoidTyID << Type::IntegerTyID << false;
		QTest::newRow("pointer") << "VuoText" << Type::PointerTyID << Type::VoidTyID << Type::PointerTyID << false;
		QTest::newRow("large struct") << "VuoArtNetInputDevice" << Type::PointerTyID << Type::VoidTyID << Type::PointerTyID << true;
	}
	void testLlvmTypes()
	{
		QFETCH(QString, typeName);
		QFETCH(Type::TypeID, argumentTypeId);
		QFETCH(Type::TypeID, secondArgumentTypeId);
		QFETCH(Type::TypeID, returnTypeId);
		QFETCH(bool, isReturnPassedAsArgument);

		VuoCompilerType *type = compiler->getType(typeName.toStdString());

		QCOMPARE(QString("%1").arg(type->llvmArgumentType->getTypeID()), QString("%1").arg(argumentTypeId));
		QCOMPARE(QString("%1").arg(type->llvmSecondArgumentType ? type->llvmSecondArgumentType->getTypeID() : Type::VoidTyID), QString("%1").arg(secondArgumentTypeId));
		QCOMPARE(QString("%1").arg(type->llvmReturnType->getTypeID()), QString("%1").arg(returnTypeId));
		QCOMPARE(type->isReturnPassedAsArgument, isReturnPassedAsArgument);
	}

	void testRealizePortTypes_data()
	{
		QTest::addColumn< QString >("nodeClassName");
		QTest::addColumn< QString >("portClassName");
		QTest::addColumn< bool >("isInputPort");
		QTest::addColumn< QString >("typeName");

		QTest::newRow("VuoInteger output port") << "vuo.math.round" << "rounded" << false << "VuoInteger";
		QTest::newRow("VuoText input port") << "vuo.text.cut" << "text" << true << "VuoText";
		QTest::newRow("event-only trigger port defined in event function") << "vuo.time.firePeriodically" << "fired" << false << "void";
		QTest::newRow("VuoInteger trigger port defined in callback start function") << "vuo.test.triggerCarryingInteger" << "fired" << false << "VuoInteger";
	}
	void testRealizePortTypes()
	{
		QFETCH(QString, nodeClassName);
		QFETCH(QString, portClassName);
		QFETCH(bool, isInputPort);
		QFETCH(QString, typeName);

		VuoCompilerNodeClass *nodeClass = compiler->getNodeClass(nodeClassName.toUtf8().constData());
		QVERIFY(nodeClass != NULL);

		VuoPortClass *portClass = (isInputPort ?
									   nodeClass->getInputPortClassWithName(portClassName.toUtf8().constData()) :
									   nodeClass->getOutputPortClassWithName(portClassName.toUtf8().constData()));
		QVERIFY(portClass != NULL);

		VuoCompilerType *expectedCompilerType = compiler->getType(typeName.toUtf8().constData());
		QVERIFY((typeName == "void") != (expectedCompilerType != NULL));  // XOR

		VuoType *expectedBaseType = (expectedCompilerType == NULL ? NULL : expectedCompilerType->getBase());
		QVERIFY((typeName == "void") != (expectedBaseType != NULL));  // XOR

		VuoCompilerEventPortClass *compilerPortClass = static_cast<VuoCompilerEventPortClass *>(portClass->getCompiler());
		VuoCompilerDataClass *actualDataClass = compilerPortClass->getDataClass();
		QVERIFY((typeName == "void") != (actualDataClass != NULL));  // XOR

		VuoType *actualBaseType = compilerPortClass->getDataVuoType();
		QCOMPARE(QString((actualBaseType == NULL ? "" : actualBaseType->getModuleKey().c_str())),
				 QString((expectedBaseType == NULL ? "" : expectedBaseType->getModuleKey().c_str())));
		QCOMPARE(actualBaseType, expectedBaseType);
	}

	void testCompoundTypes_data()
	{
		QTest::addColumn< QString >("typeName");
		QTest::addColumn< QString >("defaultTitle");
		QTest::addColumn< QStringList >("functionsShouldHave");
		QTest::addColumn< QStringList >("functionsShouldNotHave");

		{
			QStringList functionsShouldHave = {
				"VuoListCreate_VuoReal",
				"VuoListSort_VuoReal"
			};
			QStringList functionsShouldNotHave = {
				"VuoList_VuoReal_getInterprocessJson"
			};
			QTest::newRow("VuoList of core type") << "VuoList_VuoReal" << "Real List" << functionsShouldHave << functionsShouldNotHave;
		}
		{
			QStringList functionsShouldHave = {
				"VuoListCreate_VuoAudioInputDevice",
				"VuoListSort_VuoAudioInputDevice"
			};
			QStringList functionsShouldNotHave = {
				"VuoList_VuoAudioInputDevice_getInterprocessJson"
			};
			QTest::newRow("VuoList of type defined in a node set") << "VuoList_VuoAudioInputDevice" << "Audio Input Device List" << functionsShouldHave << functionsShouldNotHave;
		}
		{
			QStringList functionsShouldHave = {
				"VuoListCreate_VuoImage",
				"VuoListSort_VuoImage",
				"VuoList_VuoImage_getInterprocessJson"
			};
			QStringList functionsShouldNotHave;
			QTest::newRow("VuoList of type that overrides interprocess serialization") << "VuoList_VuoImage" << "Image List" << functionsShouldHave << functionsShouldNotHave;
		}
		{
			QStringList functionsShouldHave = {
				"VuoListCreate_VuoBlendMode"
			};
			QStringList functionsShouldNotHave = {
				"VuoListSort_VuoBlendMode",
				"VuoList_VuoBlendMode_getInterprocessJson"
			};
			QTest::newRow("VuoList of type that doesn't support comparison") << "VuoList_VuoBlendMode" << "Blend Mode List" << functionsShouldHave << functionsShouldNotHave;
		}
	}
	void testCompoundTypes()
	{
		QFETCH(QString, typeName);
		QFETCH(QString, defaultTitle);
		QFETCH(QStringList, functionsShouldHave);
		QFETCH(QStringList, functionsShouldNotHave);

		dispatch_queue_t llvmQueue = dispatch_queue_create("org.vuo.TestVuoCompilerType.llvm", nullptr);
		VuoCompilerCompoundType *type = nullptr;
		try
		{
			type = VuoCompilerCompoundType::newType(typeName.toStdString(), compiler, llvmQueue);
			QVERIFY(type);
		}
		catch (VuoCompilerException &e)
		{
			QFAIL(e.getIssues()->getLongDescription(false).c_str());
		}

		QCOMPARE(QString::fromStdString(type->getBase()->getDefaultTitle()), defaultTitle);

		for (auto functionName : functionsShouldHave)
			QVERIFY2(type->module->getFunction(functionName.toStdString()), (functionName.toStdString() + " should be in module").c_str());

		for (auto functionName : functionsShouldNotHave)
			QVERIFY2(! type->module->getFunction(functionName.toStdString()), (functionName.toStdString() + " should not be in module").c_str());
	}
};

QTEST_APPLESS_MAIN(TestVuoCompilerType)
#include "TestVuoCompilerType.moc"
