/**
 * @file
 * TestVuoCompilerModule interface and implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "TestVuoCompiler.hh"
#include "VuoCompilerModule.hh"

// Be able to use these types in QTest::addColumn()
Q_DECLARE_METATYPE(vector<string>);

/**
 * Tests for using modules (node classes and types).
 */
class TestVuoCompilerModule : public TestVuoCompiler
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

	void testModuleSymbolsRenamed()
	{
		string testClassID = "vuo.math.add.integer";
		VuoCompilerNodeClass *testClass = compiler->getNodeClass(testClassID);
		Module *module = testClass->getModule();
		string nodeEventFuncName = testClass->nameForGlobal("nodeEvent");
		Function *functionByNameFromNodeClass = module->getFunction(nodeEventFuncName);
		Function *functionByLiteralName = module->getFunction("vuo_math_add_integer__nodeEvent");
		QVERIFY(functionByNameFromNodeClass != NULL);
		QVERIFY(functionByNameFromNodeClass == functionByLiteralName);
	}

	void testDependencies_data()
	{
		QTest::addColumn< QString >("nodeClass");
		QTest::addColumn< vector<string> >("expectedDependencies");

		{
			vector<string> dependencies;
			QTest::newRow("Add does not have dependencies.") << "vuo.math.add.integer" << dependencies;
		}

		{
			vector<string> dependencies;
			dependencies.push_back("c");
			QTest::newRow("Append has dependencies.") << "vuo.text.append" << dependencies;
		}
	}
	void testDependencies()
	{
		QFETCH(QString, nodeClass);
		QFETCH(vector<string>, expectedDependencies);

		VuoCompilerNodeClass *cnc = compiler->getNodeClass( qPrintable(nodeClass) );
		vector<string> actualDependencies = cnc->getDependencies();

		QCOMPARE(actualDependencies.size(), expectedDependencies.size());
		for (uint i = 0; i < actualDependencies.size(); ++i)
		{
			string actualDependency = actualDependencies.at(i);
			string expectedDependency = expectedDependencies.at(i);
			QCOMPARE(QString(actualDependency.c_str()), QString(expectedDependency.c_str()));
		}
	}

	void testKeywords_data()
	{
		QTest::addColumn< QString >("nodeClass");
		QTest::addColumn< vector<string> >("expectedKeywords");

		{
			vector<string> keywords;
			QTest::newRow("vuo.test.compatibleWith106 does not have keywords") << "vuo.test.compatibleWith106" << keywords;
		}

		{
			vector<string> keywords;
			keywords.push_back("length");
			keywords.push_back("size");
			QTest::newRow("vuo.test.keywords has keywords") << "vuo.test.keywords" << keywords;
		}
	}
	void testKeywords()
	{
		QFETCH(QString, nodeClass);
		QFETCH(vector<string>, expectedKeywords);

		VuoCompilerNodeClass *cnc = compiler->getNodeClass( qPrintable(nodeClass) );
		vector<string> actualKeywords = cnc->getBase()->getKeywords();

		QCOMPARE(actualKeywords.size(), expectedKeywords.size());
		for (uint i = 0; i < actualKeywords.size(); ++i)
		{
			string actualKeyword = actualKeywords.at(i);
			string expectedKeyword = expectedKeywords.at(i);
			QCOMPARE(QString(actualKeyword.c_str()), QString(expectedKeyword.c_str()));
		}
	}

	void testCompatibleOperatingSystems_data()
	{
		QTest::addColumn< QString >("nodeClass");
		QTest::addColumn< bool >("shouldBeCompatibleWithAny");
		QTest::addColumn< bool >("shouldBeCompatibleWith106");
		QTest::addColumn< bool >("shouldBeCompatibleWith107");
		QTest::addColumn< bool >("shouldBeCompatibleWith108");
		QTest::addColumn< bool >("shouldBeCompatibleWith109");

		QTest::newRow("any") << "vuo.math.add.integer" << true << true << true << true << true;
		QTest::newRow("Mac OS X 10.6") << "vuo.test.compatibleWith106" << false << true << false << false << false;
		QTest::newRow("Mac OS X 10.7 and 10.8") << "vuo.test.compatibleWith107And108" << false << false << true << true << false;
		QTest::newRow("Mac OS X 10.7 and up") << "vuo.test.compatibleWith107AndUp" << false << false << true << true << true;
	}
	void testCompatibleOperatingSystems()
	{
		QFETCH(QString, nodeClass);
		QFETCH(bool, shouldBeCompatibleWithAny);
		QFETCH(bool, shouldBeCompatibleWith106);
		QFETCH(bool, shouldBeCompatibleWith107);
		QFETCH(bool, shouldBeCompatibleWith108);
		QFETCH(bool, shouldBeCompatibleWith109);

		VuoCompilerTargetSet targetAny;
		VuoCompilerTargetSet target106;
		target106.setMinMacVersion(VuoCompilerTargetSet::MacVersion_10_6);
		target106.setMaxMacVersion(VuoCompilerTargetSet::MacVersion_10_6);
		VuoCompilerTargetSet target107;
		target107.setMinMacVersion(VuoCompilerTargetSet::MacVersion_10_7);
		target107.setMaxMacVersion(VuoCompilerTargetSet::MacVersion_10_7);
		VuoCompilerTargetSet target108;
		target108.setMinMacVersion(VuoCompilerTargetSet::MacVersion_10_8);
		target108.setMaxMacVersion(VuoCompilerTargetSet::MacVersion_10_8);
		VuoCompilerTargetSet target109;
		target109.setMinMacVersion(VuoCompilerTargetSet::MacVersion_10_9);
		target109.setMaxMacVersion(VuoCompilerTargetSet::MacVersion_10_9);

		VuoCompilerNodeClass *cnc = compiler->getNodeClass( qPrintable(nodeClass) );
		VuoCompilerTargetSet actualTargets = cnc->getCompatibleTargets();

		QCOMPARE(actualTargets.isCompatibleWithAllOf(targetAny), shouldBeCompatibleWithAny);
		QCOMPARE(actualTargets.isCompatibleWithAllOf(target106), shouldBeCompatibleWith106);
		QCOMPARE(actualTargets.isCompatibleWithAllOf(target107), shouldBeCompatibleWith107);
		QCOMPARE(actualTargets.isCompatibleWithAllOf(target108), shouldBeCompatibleWith108);
		QCOMPARE(actualTargets.isCompatibleWithAllOf(target109), shouldBeCompatibleWith109);
	}
};

QTEST_APPLESS_MAIN(TestVuoCompilerModule)
#include "TestVuoCompilerModule.moc"
