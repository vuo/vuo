/**
 * @file
 * TestVuoCompilerModule interface and implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "TestVuoCompiler.hh"

// Be able to use these types in QTest::addColumn()
Q_DECLARE_METATYPE(set<string>);
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
		string testClassID = "vuo.math.round";
		VuoCompilerNodeClass *testClass = compiler->getNodeClass(testClassID);
		Module *module = testClass->getModule();
		string nodeEventFuncName = testClass->nameForGlobal("nodeEvent");
		Function *functionByNameFromNodeClass = module->getFunction(nodeEventFuncName);
		Function *functionByLiteralName = module->getFunction("vuo_math_round__nodeEvent");
		QVERIFY(functionByNameFromNodeClass != NULL);
		QVERIFY(functionByNameFromNodeClass == functionByLiteralName);
	}

	void testDependencies_data()
	{
		QTest::addColumn< QString >("nodeClass");
		QTest::addColumn< set<string> >("expectedDependencies");

		{
			set<string> dependencies;
			dependencies.insert("VuoText");
			dependencies.insert("VuoInteger");
			QTest::newRow("Node class with non-generic port types") << "vuo.text.countCharacters" << dependencies;
		}

		{
			set<string> dependencies;
			dependencies.insert("VuoList_VuoReal");
			dependencies.insert("VuoReal");
			dependencies.insert("VuoList_VuoPoint2d");
			dependencies.insert("VuoPoint2d");
			QTest::newRow("Node class with non-generic list port types") << "vuo.point.merge.xy" << dependencies;
		}

		{
			set<string> dependencies;
			dependencies.insert("VuoPoint4d");
			dependencies.insert("VuoReal");
			dependencies.insert("vuo.point.distance");
			QTest::newRow("Node class with specialized generic port type") << "vuo.point.distance.VuoPoint4d" << dependencies;
		}

		{
			set<string> dependencies;
			dependencies.insert("VuoPoint2d");
			dependencies.insert("VuoReal");
			dependencies.insert("vuo.point.distance");
			QTest::newRow("Node class with unspecialized generic port type") << "vuo.point.distance.VuoGenericType1" << dependencies;
		}

		{
			set<string> dependencies;
			dependencies.insert("VuoInteger");
			dependencies.insert("VuoList_VuoKey");
			dependencies.insert("VuoKey");
			dependencies.insert("vuo.list.count");
			QTest::newRow("Node class with specialized generic list port type") << "vuo.list.count.VuoKey" << dependencies;
		}

		{
			set<string> dependencies;
			dependencies.insert("VuoInteger");
			dependencies.insert("VuoList_VuoInteger");
			dependencies.insert("vuo.list.count");
			QTest::newRow("Node class with unspecialized generic list port type") << "vuo.list.count.VuoGenericType1" << dependencies;
		}

		{
			set<string> dependencies;
			dependencies.insert("VuoSceneObjectGet");
			dependencies.insert("VuoText");
			dependencies.insert("VuoBoolean");
			dependencies.insert("VuoSceneObject");
			QTest::newRow("Node class with dependencies in metadata") << "vuo.scene.fetch" << dependencies;
		}

		{
			set<string> dependencies;
			dependencies.insert("VuoReal");
			dependencies.insert("VuoList_VuoReal");
			QTest::newRow("List node class") << "vuo.list.make.2.VuoReal" << dependencies;
		}

		{
			set<string> dependencies;
			dependencies.insert("VuoText");
			dependencies.insert("VuoReal");
			dependencies.insert("VuoList_VuoText");
			dependencies.insert("VuoList_VuoReal");
			dependencies.insert("VuoDictionary_VuoText_VuoReal");
			QTest::newRow("Dictionary node class") << "vuo.dictionary.make.VuoText.VuoReal" << dependencies;
		}

		{
			set<string> dependencies;
			dependencies.insert("VuoInteger");
			dependencies.insert("VuoPoint2d");
			QTest::newRow("Published input node class") << "vuo.out.2.first.second.VuoInteger.VuoPoint2d" << dependencies;
		}
	}
	void testDependencies()
	{
		QFETCH(QString, nodeClass);
		QFETCH(set<string>, expectedDependencies);

		VuoCompilerNodeClass *cnc = compiler->getNodeClass( nodeClass.toStdString() );

		VuoNode *node = compiler->createNode(cnc);
		VuoCompilerComposition *composition = new VuoCompilerComposition(new VuoComposition(), NULL);
		composition->getBase()->addNode(node);
		compiler->reifyGenericPortTypes(composition);
		cnc = node->getNodeClass()->getCompiler();

		set<string> actualDependencies = cnc->getDependencies();

		vector<string> sortedActualDependencies(actualDependencies.begin(), actualDependencies.end());
		sort(sortedActualDependencies.begin(), sortedActualDependencies.end());
		string actualJoined = VuoStringUtilities::join(sortedActualDependencies, ' ');
		vector<string> sortedExpectedDependencies(expectedDependencies.begin(), expectedDependencies.end());
		sort(sortedExpectedDependencies.begin(), sortedExpectedDependencies.end());
		string expectedJoined = VuoStringUtilities::join(sortedExpectedDependencies, ' ');

		QCOMPARE(QString(actualJoined.c_str()), QString(expectedJoined.c_str()));
	}

	void testCompatibleTargets_data()
	{
		QTest::addColumn<QString>("moduleKey");
		QTest::addColumn<QString>("expectedCompatibility");

		QTest::newRow("no restrictions") << "vuo.event.fireOnStart" << "any system that Vuo supports";
		QTest::newRow("node class is restricted") << "vuo.image.make.web" << "macOS 10.13 and above";
		QTest::newRow("library is restricted") << "VuoLeap" << "macOS on an Intel (X86-64) CPU";
		QTest::newRow("dependency of node class is restricted") << "vuo.ndi.receive" << "any system that Vuo supports";  // VuoCompilerModule::getCompatibleTargets() only checks the compatibility of the module itself, not its dependencies.
	}
	void testCompatibleTargets()
	{
		QFETCH(QString, moduleKey);
		QFETCH(QString, expectedCompatibility);

		// Skip this test on single-architecture builds, since modules will be reported as only compatible with that architecture.
		QStringList availableArchitectures;
		VuoCompilerGroup *compilers = VuoCompilerGroup::compilersForAllDeploymentArchitectures();
		compilers->doForEach([&](VuoCompiler *compiler) {
			availableArchitectures.append(QString::fromStdString(compiler->getArch()));
		});
		if (! (availableArchitectures.contains("x86_64") && availableArchitectures.contains("arm64")) )
			QSKIP("Not testing module compatibility since this is a single-arch build.");

		compiler->getNodeClasses();
		VuoCompilerModule *module = compiler->getModule(moduleKey.toStdString());

		QCOMPARE(QString::fromStdString(module->getCompatibleTargets().toString()), expectedCompatibility);
	}

	void testKeywords_data()
	{
		QTest::addColumn< QString >("nodeClass");
		QTest::addColumn< vector<string> >("expectedKeywords");

		{
			vector<string> keywords;
			QTest::newRow("vuo.test.details does not have keywords") << "vuo.test.details" << keywords;
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

	void testModuleKeyForPath_data()
	{
		QTest::addColumn<QString>("path");
		QTest::addColumn<QString>("moduleKey");

		QTest::newRow("compiled node class with author and node set") << "/Users/me/Library/Caches/org.vuo/1.2.3.45678/Modules/cat.food.eat.vuonode" << "cat.food.eat";
		QTest::newRow("subcomposition with author") << "~/Library/Application Support/Vuo/Modules/cat.purr.vuo" << "cat.purr";
		QTest::newRow("compiled type") << "VuoAudioBins.bc" << "VuoAudioBins";
		QTest::newRow("ISF with double extension") << "me.double.fs.fs" << "me.double";
		QTest::newRow("ISF with spaces") << "me.Space out Words.vs" << "me.spaceOutWords";
		QTest::newRow("ISF with no author") << "anonymous.vs" << "isf.anonymous";
		QTest::newRow("ISF with all of the above") << "Modules/VHS Glitch.fs.fs" << "isf.vHSGlitch";
	}
	void testModuleKeyForPath()
	{
		QFETCH(QString, path);
		QFETCH(QString, moduleKey);

		QCOMPARE(QString::fromStdString(compiler->getModuleKeyForPath(path.toStdString())), moduleKey);
	}
};

QTEST_APPLESS_MAIN(TestVuoCompilerModule)
#include "TestVuoCompilerModule.moc"
