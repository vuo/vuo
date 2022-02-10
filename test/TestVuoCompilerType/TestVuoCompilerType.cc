/**
 * @file
 * TestVuoCompilerType interface and implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
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
};

QTEST_APPLESS_MAIN(TestVuoCompilerType)
#include "TestVuoCompilerType.moc"
