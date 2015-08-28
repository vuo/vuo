/**
 * @file
 * TestCompositionUpgrade interface and implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <sstream>
#include <Vuo/Vuo.h>

#include "TestCompositionExecution.hh"

#include "VuoModuleUpgradeManager.hh"


// Be able to use these types in QTest::addColumn()
Q_DECLARE_METATYPE(string);
Q_DECLARE_METATYPE(vector<string>);


/**
 * Tests that compositions give correct output when executed.
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

	void testUpgradingAndBuildingComposition_data()
	{
		QTest::addColumn<QString>("compositionFileName");

		QDir compositionDir = getCompositionDir();
		QStringList filter("*.vuo");
		QStringList compositionFileNames = compositionDir.entryList(filter);
		foreach (QString compositionFileName, compositionFileNames)
			QTest::newRow(compositionFileName.toUtf8()) << compositionFileName;
	}
	void testUpgradingAndBuildingComposition()
	{
		QFETCH(QString, compositionFileName);

		string dir, name, extension;
		VuoFileUtilities::splitPath(compositionFileName.toStdString(), dir, name, extension);

		string compositionPath = getCompositionPath(compositionFileName.toStdString());
		string compositionString = VuoFileUtilities::readFileToString(compositionPath);
		QVERIFY(!compositionString.empty());

		bool compositionWasUpgraded = false;
		VuoCompilerComposition *compilerComposition = VuoModuleUpgradeManager::parseComposition(compositionString, compiler, compositionWasUpgraded);
		QVERIFY(compositionWasUpgraded);

		string compiledCompositionPath = VuoFileUtilities::makeTmpFile(name, "bc");
		string linkedCompositionPath = VuoFileUtilities::makeTmpFile(name + "-linked", "");

		bool compilationFailedWithObsoleteNodes = false;
		try
		{
			compiler->compileComposition(compilerComposition, compiledCompositionPath);
			compiler->linkCompositionToCreateExecutable(compiledCompositionPath, linkedCompositionPath);
		}
		catch (std::runtime_error e)
		{
			if (VuoStringUtilities::beginsWith(e.what(), "This composition contains nodes that are neither built in to this version of Vuo nor installed on this computer"))
				compilationFailedWithObsoleteNodes = true;
		}
		QCOMPARE(compilationFailedWithObsoleteNodes, compilerComposition->getBase()->getName() == "TestCompositionUpgrade:ExpectCompilationToFailDueToObsoleteNodes");

		remove(compiledCompositionPath.c_str());
		remove(linkedCompositionPath.c_str());
		delete compilerComposition;
	}

};

QTEST_APPLESS_MAIN(TestCompositionUpgrade)
#include "TestCompositionUpgrade.moc"
