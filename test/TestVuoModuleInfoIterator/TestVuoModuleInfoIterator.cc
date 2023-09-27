/**
 * @file
 * TestVuoModuleInfoIterator interface and implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include <Vuo/Vuo.h>

typedef map<string, vector<string>> modulePathList;

// Be able to use these types in QTest::addColumn()
Q_DECLARE_METATYPE(modulePathList);
Q_DECLARE_METATYPE(string);
Q_DECLARE_METATYPE(set<string>);

/**
 * Tests the TestVuoModuleInfoIterator class.
 */
class TestVuoModuleInfoIterator : public QObject
{
	Q_OBJECT

private slots:

	void testIteration_data()
	{
		QTest::addColumn< modulePathList >("allModulePaths");
		QTest::addColumn< string >("overriddenModulesPath");
		QTest::addColumn< set<string> >("searchModuleKeys");
		QTest::addColumn< QStringList >("expectedModulePaths");

		string noOverriddenModulesPath;
		set<string> noSearchModuleKeys;

		{
			modulePathList noModulePaths;
			QStringList expectedModulePaths;
			QTest::newRow("no modules") << noModulePaths << noOverriddenModulesPath << noSearchModuleKeys << expectedModulePaths;
		}
		{
			modulePathList allModulePaths = {
				{ "/1", { "vuo.test.a.vuonode", "vuo.test.b.vuonode" } }
			};
			QStringList expectedModulePaths = {
				"/1/vuo.test.a.vuonode",
				"/1/vuo.test.b.vuonode"
			};
			QTest::newRow("1 module directory") << allModulePaths << noOverriddenModulesPath << noSearchModuleKeys << expectedModulePaths;
		}
		{
			modulePathList allModulePaths = {
				{ "/1", {} },
				{ "/2", { "vuo.test.a.vuonode", "vuo.test.b.vuonode" } }
			};
			QStringList expectedModulePaths = {
				"/2/vuo.test.a.vuonode",
				"/2/vuo.test.b.vuonode"
			};
			QTest::newRow("2 module directories, 1 of them empty") << allModulePaths << noOverriddenModulesPath << noSearchModuleKeys << expectedModulePaths;
		}
		{
			modulePathList allModulePaths = {
				{ "/1", { "vuo.test.a.vuonode", "vuo.test.b.vuonode" } },
				{ "/2", { "vuo.test.c.vuonode", "vuo.test.d.vuonode" } }
			};
			set<string> searchModuleKeys = {
				"vuo.test.a",
				"vuo.test.d",
				"vuo.test.e"
			};
			QStringList expectedModulePaths = {
				"/1/vuo.test.a.vuonode",
				"/2/vuo.test.d.vuonode"
			};
			QTest::newRow("2 module directories, search module keys specified") << allModulePaths << noOverriddenModulesPath << searchModuleKeys << expectedModulePaths;
		}
		{
			string overriddenModulesPath = "/o";
			modulePathList allModulePaths = {
				{ "/1", { "vuo.test.a.vuonode", "vuo.test.b.vuonode" } },
				{ "/2", { "vuo.test.c.vuonode", "vuo.test.d.vuonode" } },
				{ "/o", { "vuo.test.b.vuonode", "vuo.test.d.vuonode" } }
			};
			QStringList expectedModulePaths = {
				"/1/vuo.test.a.vuonode",
				"/o/vuo.test.b.vuonode",
				"/2/vuo.test.c.vuonode",
				"/o/vuo.test.d.vuonode"
			};
			QTest::newRow("2 module directories, some modules overridden") << allModulePaths << overriddenModulesPath << noSearchModuleKeys << expectedModulePaths;
		}
	}
	void testIteration()
	{
		QFETCH(modulePathList, allModulePaths);
		QFETCH(string, overriddenModulesPath);
		QFETCH(set<string>, searchModuleKeys);
		QFETCH(QStringList, expectedModulePaths);

		map<string, map<string, VuoModuleInfo *>> allModuleInfos;
		for (auto i : allModulePaths)
		{
			string searchPath = i.first;
			for (string relativePath : i.second)
			{
				VuoModuleInfo *moduleInfo = new VuoModuleInfo(nullptr, searchPath, relativePath, false, false);
				allModuleInfos[searchPath][moduleInfo->getModuleKey()] = moduleInfo;
			}
		}

		VuoModuleInfoIterator *moduleInfoIter = searchModuleKeys.empty() ?
													new VuoModuleInfoIterator(&allModuleInfos, overriddenModulesPath) :
													new VuoModuleInfoIterator(&allModuleInfos, overriddenModulesPath, searchModuleKeys);

		QStringList actualModulePaths;
		VuoModuleInfo *moduleInfo;
		while ((moduleInfo = moduleInfoIter->next()))
			actualModulePaths.append(QString::fromStdString(moduleInfo->getFile()->path()));

		actualModulePaths.sort();
		expectedModulePaths.sort();

		QCOMPARE(actualModulePaths, expectedModulePaths);

		delete moduleInfoIter;
	}

};

QTEST_APPLESS_MAIN(TestVuoModuleInfoIterator)
#include "TestVuoModuleInfoIterator.moc"
