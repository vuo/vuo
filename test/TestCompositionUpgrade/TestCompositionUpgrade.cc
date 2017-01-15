/**
 * @file
 * TestCompositionUpgrade interface and implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <sstream>
#include <Vuo/Vuo.h>

#include "TestCompositionExecution.hh"

#include "VuoCompilerException.hh"
#include "VuoModuleUpgradeManager.hh"
#include "VuoRendererComposition.hh"


/**
 * Tests VuoModuleUpgradeManager and upgrade paths.
 */
class TestCompositionUpgrade : public TestCompositionExecution
{
	Q_OBJECT

private:

	VuoCompiler *compiler;

private slots:

	void initTestCase()
	{
		compiler = initCompiler();
	}

	void cleanupTestCase()
	{
		delete compiler;
	}

	void readComposition(const QString &compositionFileName, VuoCompilerComposition *&compilerComposition)
	{
		string compositionPath = getCompositionPath(compositionFileName.toStdString());
		string compositionString = VuoFileUtilities::readFileToString(compositionPath);
		QVERIFY(!compositionString.empty());

		VuoCompilerGraphvizParser *parser = VuoCompilerGraphvizParser::newParserFromCompositionString(compositionString, compiler);
		compilerComposition = new VuoCompilerComposition(new VuoComposition(), parser);
		delete parser;

		bool compositionWasUpgraded = false;
		VuoCompilerComposition *upgradedCcompilerComposition = VuoModuleUpgradeManager::parseComposition(compositionString, compiler, compositionWasUpgraded);
		bool compositionUpgradeExpected = (compilerComposition->getBase()->getName().find("TestCompositionUpgrade:ExpectCompilationToFailDueToObsoleteNodes") != std::string::npos) &&
										  (compilerComposition->getBase()->getName().find("TestCompositionUpgrade:ExpectNoUpgrade") == std::string::npos);
		QCOMPARE(compositionWasUpgraded, compositionUpgradeExpected);
		delete upgradedCcompilerComposition;
	}

	void readAndUpgradeComposition(const QString &compositionFileName, VuoCompilerComposition *&compilerComposition)
	{
		string compositionPath = getCompositionPath(compositionFileName.toStdString());
		string compositionString = VuoFileUtilities::readFileToString(compositionPath);
		QVERIFY(!compositionString.empty());

		bool compositionWasUpgraded = false;
		compilerComposition = VuoModuleUpgradeManager::parseComposition(compositionString, compiler, compositionWasUpgraded);
		bool compositionUpgradeExpected = compilerComposition->getBase()->getName().find("TestCompositionUpgrade:ExpectNoUpgrade") == std::string::npos;
		QCOMPARE(compositionWasUpgraded, compositionUpgradeExpected);

		VuoRendererComposition *rendererComposition = new VuoRendererComposition(compilerComposition->getBase());
		delete rendererComposition;
	}

	void checkEqual(set<VuoNode *> nodes1, set<VuoNode *> nodes2)
	{
		vector<string> names1;
		foreach (VuoNode *n, nodes1)
		{
			string name = n->getNodeClass()->getClassName() + "/" + n->getTitle();
			names1.push_back(name);
		}

		vector<string> names2;
		foreach (VuoNode *n, nodes2)
		{
			string name = n->getNodeClass()->getClassName() + "/" + n->getTitle();
			names2.push_back(name);
		}

		sort(names1.begin(), names1.end());
		sort(names2.begin(), names2.end());

		for (size_t i = 0; i < MAX(names1.size(), names2.size()); ++i)
		{
			QVERIFY(i < names1.size());
			QVERIFY(i < names2.size());
			QCOMPARE(QString::fromStdString(names1[i]), QString::fromStdString(names2[i]));
		}
	}

	void checkEqual(set<VuoCable *> cables1, set<VuoCable *> cables2)
	{
		vector<string> names1;
		foreach (VuoCable *c, cables1)
		{
			QVERIFY(c->getFromNode());
			QVERIFY(c->getFromPort());
			QVERIFY(c->getToNode());
			QVERIFY(c->getToPort());
			string name = c->getFromNode()->getTitle() + ":" + c->getFromPort()->getClass()->getName() + " -> " +
						  c->getToNode()->getTitle() + ":" + c->getToPort()->getClass()->getName();
			names1.push_back(name);
		}

		vector<string> names2;
		foreach (VuoCable *c, cables2)
		{
			QVERIFY(c->getFromNode());
			QVERIFY(c->getFromPort());
			QVERIFY(c->getToNode());
			QVERIFY(c->getToPort());
			string name = c->getFromNode()->getTitle() + ":" + c->getFromPort()->getClass()->getName() + " -> " +
						  c->getToNode()->getTitle() + ":" + c->getToPort()->getClass()->getName();
			names2.push_back(name);
		}

		sort(names1.begin(), names1.end());
		sort(names2.begin(), names2.end());

		for (size_t i = 0; i < MAX(names1.size(), names2.size()); ++i)
		{
			QVERIFY(i < names1.size());
			QVERIFY(i < names2.size());
			QCOMPARE(QString::fromStdString(names1[i]), QString::fromStdString(names2[i]));
		}
	}

	void checkEqual(vector<VuoPublishedPort *> ports1, vector<VuoPublishedPort *> ports2)
	{
		for (size_t i = 0; i < MAX(ports1.size(), ports2.size()); ++i)
		{
			QVERIFY(i < ports1.size());
			QVERIFY(i < ports2.size());
			QCOMPARE(QString::fromStdString(ports1[i]->getClass()->getName()), QString::fromStdString(ports2[i]->getClass()->getName()));
			QCOMPARE(static_cast<VuoCompilerEventPortClass *>(ports1[i]->getClass()->getCompiler())->getDataVuoType(),
					 static_cast<VuoCompilerEventPortClass *>(ports2[i]->getClass()->getCompiler())->getDataVuoType());
		}
	}

/*
	void testUpgradingCompositions_data()
	{
		QTest::addColumn<QString>("compositionFileName");

		QDir compositionDir = getCompositionDir().filePath("past");
		QStringList filter("*.vuo");
		QStringList compositionFileNames = compositionDir.entryList(filter);
		foreach (QString compositionFileName, compositionFileNames)
			QTest::newRow(compositionFileName.toUtf8()) << compositionFileName;
	}
	void testUpgradingCompositions()
	{
		QFETCH(QString, compositionFileName);

		VuoCompilerComposition *actualUpgradedComposition = NULL;
		readAndUpgradeComposition(QString("past") + QDir::separator() + compositionFileName, actualUpgradedComposition);

		QString currentCompositionFileName = compositionFileName;
		int lastHyphen = compositionFileName.lastIndexOf("-");
		currentCompositionFileName.remove(lastHyphen, compositionFileName.length() - 4 - lastHyphen);
		VuoCompilerComposition *expectedUpgradedComposition = NULL;
		readComposition(QString("current") + QDir::separator() + currentCompositionFileName, expectedUpgradedComposition);

//		VLog("expectedUpgradedComposition: %s", expectedUpgradedComposition->getGraphvizDeclaration().c_str());
//		VLog("actualUpgradedComposition: %s", actualUpgradedComposition->getGraphvizDeclaration().c_str());

		{
			set<VuoNode *> actual = actualUpgradedComposition->getBase()->getNodes();
			set<VuoNode *> expected = expectedUpgradedComposition->getBase()->getNodes();
			checkEqual(actual, expected);
		}
		{
			set<VuoCable *> actual = actualUpgradedComposition->getBase()->getCables();
			set<VuoCable *> expected = expectedUpgradedComposition->getBase()->getCables();
			checkEqual(actual, expected);
		}
		{
			vector<VuoPublishedPort *> actual = actualUpgradedComposition->getBase()->getPublishedInputPorts();
			vector<VuoPublishedPort *> expected = expectedUpgradedComposition->getBase()->getPublishedInputPorts();
			checkEqual(actual, expected);
		}
		{
			vector<VuoPublishedPort *> actual = actualUpgradedComposition->getBase()->getPublishedOutputPorts();
			vector<VuoPublishedPort *> expected = expectedUpgradedComposition->getBase()->getPublishedOutputPorts();
			checkEqual(actual, expected);
		}
		{
			set<VuoCable *> actual = actualUpgradedComposition->getBase()->getPublishedInputCables();
			set<VuoCable *> expected = expectedUpgradedComposition->getBase()->getPublishedInputCables();
			checkEqual(actual, expected);
		}
		{
			set<VuoCable *> actual = actualUpgradedComposition->getBase()->getPublishedOutputCables();
			set<VuoCable *> expected = expectedUpgradedComposition->getBase()->getPublishedOutputCables();
			checkEqual(actual, expected);
		}
	}
*/

	void testUpgradingAndBuildingAllNodes_data()
	{
		QTest::addColumn<QString>("compositionFileName");

		QDir compositionDir = getCompositionDir().filePath("allNodes");
		QStringList filter("*.vuo");
		QStringList compositionFileNames = compositionDir.entryList(filter);
		foreach (QString compositionFileName, compositionFileNames)
			QTest::newRow(compositionFileName.toUtf8()) << compositionFileName;
	}
	void testUpgradingAndBuildingAllNodes()
	{
		QFETCH(QString, compositionFileName);
		printf("%s\n", compositionFileName.toUtf8().data()); fflush(stdout);

		VuoCompilerComposition *compilerComposition = NULL;
		readAndUpgradeComposition(QString("allNodes") + QDir::separator() + compositionFileName, compilerComposition);

		/// @todo Remove after fixing https://b33p.net/kosada/node/8120
		foreach (VuoNode *node, compilerComposition->getBase()->getNodes())
		{
			if (node->getNodeClass()->getClassName() == "vuo.list.make.2.VuoGenericType1")
			{
				foreach (VuoCable *cable, compilerComposition->getBase()->getCables())
					if (cable->getFromNode() == node)
						compilerComposition->getBase()->removeCable(cable);

				compilerComposition->getBase()->removeNode(node);
			}
		}

		string dir, name, ext;
		VuoFileUtilities::splitPath(compositionFileName.toStdString(), dir, name, ext);
		string compiledCompositionPath = VuoFileUtilities::makeTmpFile(name, "bc");
		string linkedCompositionPath = VuoFileUtilities::makeTmpFile(name + "-linked", "");

		try
		{
			compiler->compileComposition(compilerComposition, compiledCompositionPath);
			compiler->linkCompositionToCreateExecutable(compiledCompositionPath, linkedCompositionPath, VuoCompiler::Optimization_FastBuild);
		}
		catch (const VuoCompilerException &e)
		{
			if (e.getErrors().size() == 1 && e.getErrors().at(0).getSummary() == "Node not installed")
			{
				bool compilationFailureExpected = compilerComposition->getBase()->getName().find("TestCompositionUpgrade:ExpectCompilationToFailDueToObsoleteNodes") != std::string::npos;
				if (!compilationFailureExpected)
					QFAIL(e.what());
			}
			else
				QFAIL(e.what());
		}

		QVERIFY(remove(compiledCompositionPath.c_str()) == 0);
		QVERIFY(remove(linkedCompositionPath.c_str()) == 0);
		delete compilerComposition;
	}

};


int main(int argc, char *argv[])
{
	VuoRendererComposition::createAutoreleasePool();

	// https://bugreports.qt-project.org/browse/QTBUG-29197
	qputenv("QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM", "1");

	QApplication app(argc, argv);
	TestCompositionUpgrade tc;
	return QTest::qExec(&tc, argc, argv);
}

#include "TestCompositionUpgrade.moc"
