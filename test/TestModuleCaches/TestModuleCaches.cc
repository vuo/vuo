/**
 * @file
 * TestModuleCaches interface and implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "InstalledModulesChange.hh"
#include "ModuleCachesDiff.hh"
#include "TestCompositionExecution.hh"
#include <sstream>
#include <sys/time.h>

Q_DECLARE_METATYPE(InstalledModulesChange);
Q_DECLARE_METATYPE(ModuleCachesDiff);

/**
 * Tests that module caches rebuild at the right times.
 */
class TestModuleCaches : public QObject
{
	Q_OBJECT

private:

	void deleteAllModuleCaches()
	{
		VuoFileUtilities::deleteDir(VuoFileUtilities::getCachePath());
	}

	void deleteAllModuleCacheLockInfo(void)
	{
		for (auto i : VuoModuleCache::interprocessLockInfo)
			delete i.second;

		VuoModuleCache::interprocessLockInfo.clear();
	}

private slots:

	void init()
	{
		ModuleScope::deleteAllInstalledTestModules();

		for (ModuleScope::Scope s : { ModuleScope::System, ModuleScope::User })
		{
			try
			{
				ModuleScope scope(s);
				set<VuoFileUtilities::File *> files = VuoFileUtilities::findAllFilesInDirectory(scope.getInstalledModulesPath());
				auto visibleFile = std::find_if(files.begin(), files.end(),
												[](VuoFileUtilities::File *f) { return ! VuoStringUtilities::beginsWith(f->getRelativePath(), "."); });
				QVERIFY2(visibleFile == files.end(), string("To run this test, you must uninstall all modules in " + scope.getInstalledModulesPath()).c_str());
			}
			catch (VuoException &e)
			{
				if (s != ModuleScope::System)
					QFAIL(e.what());
			}
		}

		deleteAllModuleCaches();
	}

	void cleanup()
	{
		ModuleScope::deleteAllInstalledTestModules();
	}

	void testModuleCacheCreation()
	{
		string compositionPath = ModuleScope::registerCompositionDirectory("TestModuleCaches-testModuleCacheCreation");
		ModuleCachesDiff::setTarget(VuoCompiler::getProcessTarget());

		ModuleCachesDiff expectedDiff;
		expectedDiff.recordBaseline();

		VuoCompiler compiler(compositionPath);
		compiler.makeModuleCachesAvailable(true, false);

		expectedDiff.dylibsAdded = {
			ModuleScope::System,
			ModuleScope::User,
			ModuleScope::CompositionFamily,
			ModuleScope::Composition
		};
		expectedDiff.manifestShouldNotContain = {
			{ ModuleScope::System, {"*"} },
			{ ModuleScope::User, {"*"} },
			{ ModuleScope::CompositionFamily, {"*"} },
			{ ModuleScope::Composition, {"*"} }
		};
		expectedDiff.check();
	}

	void testModuleCacheUpdates_data()
	{
		QTest::addColumn< InstalledModulesChange >("baselineSetup");
		QTest::addColumn< InstalledModulesChange >("changeToTest");
		QTest::addColumn< QStringList >("compositionDirNames");
		QTest::addColumn< ModuleCachesDiff >("expectedDiff");

		QStringList defaultCompositionDirNames;

		// No change

		{
			InstalledModulesChange baselineSetup;
			InstalledModulesChange changeToTest;
			ModuleCachesDiff expectedDiff;
			QTest::newRow("No change to installed modules") << baselineSetup << changeToTest << defaultCompositionDirNames << expectedDiff;
		}

		// Installing in User Modules

		{
			InstalledModulesChange baselineSetup;
			InstalledModulesChange changeToTest;
			changeToTest.modulesToInstall = {
				ModuleLocation(ModuleScope::User, "vuo.test.fillRealList", "vuonode")
			};
			ModuleCachesDiff expectedDiff;
			expectedDiff.dylibsModified = {
				ModuleScope::User,
				ModuleScope::CompositionFamily,
				ModuleScope::Composition };
			expectedDiff.manifestShouldContain = {
				{ ModuleScope::User, {"vuo.test.fillRealList"} }
			};
			expectedDiff.manifestShouldNotContain = {
				{ ModuleScope::System, {"*"} },
				{ ModuleScope::User, {"VuoInteger", "VuoReal", "VuoList_VuoReal"} },
				{ ModuleScope::CompositionFamily, {"*"} },
				{ ModuleScope::Composition, {"*"} }
			};
			QTest::newRow("Compiled node class with built-in dependencies installed in User Modules") << baselineSetup << changeToTest << defaultCompositionDirNames << expectedDiff;
		}
		{
			InstalledModulesChange baselineSetup;
			InstalledModulesChange changeToTest;
			changeToTest.modulesToInstall = {
				ModuleLocation(ModuleScope::User, "vuo.test.passThrough", "c")
			};
			ModuleCachesDiff expectedDiff;
			expectedDiff.compiledModulesAdded = {
				ModuleLocation(ModuleScope::User, "vuo.test.passThrough", "vuonode")
			};
			expectedDiff.dylibsModified = {
				ModuleScope::User,
				ModuleScope::CompositionFamily,
				ModuleScope::Composition
			};
			expectedDiff.manifestShouldContain = {
				{ ModuleScope::User, {"vuo.test.passThrough"} }
			};
			expectedDiff.manifestShouldNotContain = {
				{ ModuleScope::System, {"*"} },
				{ ModuleScope::User, {"VuoText"} },
				{ ModuleScope::CompositionFamily, {"*"} },
				{ ModuleScope::Composition, {"*"} }
			};
			QTest::newRow("C node class with built-in dependencies installed in User Modules") << baselineSetup << changeToTest << defaultCompositionDirNames << expectedDiff;
		}
		{
			InstalledModulesChange baselineSetup;
			InstalledModulesChange changeToTest;
			changeToTest.modulesToInstall = {
				ModuleLocation(ModuleScope::User, "vuo.test.makeColorOpaque", "vuo")
			};
			ModuleCachesDiff expectedDiff;
			expectedDiff.compiledModulesAdded = {
				ModuleLocation(ModuleScope::User, "vuo.test.makeColorOpaque", "vuonode"),
				ModuleLocation(ModuleScope::User, "vuo.in.1.Color.VuoColor", "vuonode"),
				ModuleLocation(ModuleScope::User, "vuo.out.1.OpaqueColor.VuoColor", "vuonode")
			};
			expectedDiff.dylibsModified = {
				ModuleScope::User,
				ModuleScope::CompositionFamily,
				ModuleScope::Composition
			};
			expectedDiff.manifestShouldContain = {
				{ ModuleScope::User, {"vuo.test.makeColorOpaque", "vuo.in.1.Color.VuoColor", "vuo.out.1.OpaqueColor.VuoColor"} }
			};
			expectedDiff.manifestShouldNotContain = {
				{ ModuleScope::System, {"*"} },
				{ ModuleScope::User, {"VuoColor"} },
				{ ModuleScope::CompositionFamily, {"*"} },
				{ ModuleScope::Composition, {"*"} }
			};
			QTest::newRow("Subcomposition with generated dependencies installed in User Modules") << baselineSetup << changeToTest << defaultCompositionDirNames << expectedDiff;
		}

		// Installing in composition-local Modules

		{
			InstalledModulesChange baselineSetup;
			InstalledModulesChange changeToTest;
			changeToTest.modulesToInstall = {
				ModuleLocation(ModuleScope::CompositionFamily, "vuo.test.fillRealList", "vuonode")
			};
			ModuleCachesDiff expectedDiff;
			expectedDiff.dylibsModified = {
				ModuleScope::CompositionFamily,
				ModuleScope::Composition };
			expectedDiff.manifestShouldContain = {
				{ ModuleScope::CompositionFamily, {"vuo.test.fillRealList"} }
			};
			expectedDiff.manifestShouldNotContain = {
				{ ModuleScope::System, {"*"} },
				{ ModuleScope::User, {"*"} },
				{ ModuleScope::CompositionFamily, {"VuoInteger", "VuoReal", "VuoList_VuoReal"} },
				{ ModuleScope::Composition, {"*"} }
			};
			QTest::newRow("Compiled node class with built-in dependencies installed in composition-local Modules") << baselineSetup << changeToTest << defaultCompositionDirNames << expectedDiff;
		}
		{
			InstalledModulesChange baselineSetup;
			InstalledModulesChange changeToTest;
			changeToTest.modulesToInstall = {
				ModuleLocation(ModuleScope::CompositionFamily, "vuo.test.passThrough", "c")
			};
			ModuleCachesDiff expectedDiff;
			expectedDiff.compiledModulesAdded = {
				ModuleLocation(ModuleScope::CompositionFamily, "vuo.test.passThrough", "vuonode")
			};
			expectedDiff.dylibsModified = {
				ModuleScope::CompositionFamily,
				ModuleScope::Composition
			};
			expectedDiff.manifestShouldContain = {
				{ ModuleScope::CompositionFamily, {"vuo.test.passThrough"} }
			};
			expectedDiff.manifestShouldNotContain = {
				{ ModuleScope::System, {"*"} },
				{ ModuleScope::User, {"*"} },
				{ ModuleScope::CompositionFamily, {"VuoText"} },
				{ ModuleScope::Composition, {"*"} }
			};
			QTest::newRow("C node class with built-in dependencies installed in composition-local Modules") << baselineSetup << changeToTest << defaultCompositionDirNames << expectedDiff;
		}
		{
			InstalledModulesChange baselineSetup;
			InstalledModulesChange changeToTest;
			changeToTest.modulesToInstall = {
				ModuleLocation(ModuleScope::CompositionFamily, "vuo.test.makeColorOpaque", "vuo")
			};
			ModuleCachesDiff expectedDiff;
			expectedDiff.compiledModulesAdded = {
				ModuleLocation(ModuleScope::CompositionFamily, "vuo.test.makeColorOpaque", "vuonode"),
				ModuleLocation(ModuleScope::CompositionFamily, "vuo.in.1.Color.VuoColor", "vuonode"),
				ModuleLocation(ModuleScope::CompositionFamily, "vuo.out.1.OpaqueColor.VuoColor", "vuonode")
			};
			expectedDiff.dylibsModified = {
				ModuleScope::CompositionFamily,
				ModuleScope::Composition
			};
			expectedDiff.manifestShouldContain = {
				{ ModuleScope::CompositionFamily, {"vuo.test.makeColorOpaque", "vuo.in.1.Color.VuoColor", "vuo.out.1.OpaqueColor.VuoColor"} }
			};
			expectedDiff.manifestShouldNotContain = {
				{ ModuleScope::System, {"*"} },
				{ ModuleScope::User, {"*"} },
				{ ModuleScope::CompositionFamily, {"VuoColor"} },
				{ ModuleScope::Composition, {"*"} }
			};
			QTest::newRow("Subcomposition with generated dependencies installed in composition-local Modules") << baselineSetup << changeToTest << defaultCompositionDirNames << expectedDiff;
		}

		// Generic modules

		{
			InstalledModulesChange baselineSetup;
			InstalledModulesChange changeToTest;
			changeToTest.modulesToInstall = {
				ModuleLocation(ModuleScope::User, "vuo.test.selectItems", "vuonode")
			};
			ModuleCachesDiff expectedDiff;
			expectedDiff.dylibsModified = {
				ModuleScope::User,
				ModuleScope::CompositionFamily,
				ModuleScope::Composition };
			expectedDiff.manifestShouldContain = {
				{ ModuleScope::User, {"vuo.test.selectItems"} }
			};
			expectedDiff.manifestShouldNotContain = {
				{ ModuleScope::User, {"VuoList_VuoBoolean", "VuoList_VuoGenericType1"} },
				{ ModuleScope::CompositionFamily, {"*"} },
				{ ModuleScope::Composition, {"*"} }
			};
			QTest::newRow("Compiled node class with generic types installed in User Modules") << baselineSetup << changeToTest << defaultCompositionDirNames << expectedDiff;
		}
		{
			InstalledModulesChange baselineSetup;
			InstalledModulesChange changeToTest;
			changeToTest.modulesToInstall = {
				ModuleLocation(ModuleScope::CompositionFamily, "vuo.test.rotateImageList", "vuo")
			};
			ModuleCachesDiff expectedDiff;
			expectedDiff.compiledModulesAdded = {
				ModuleLocation(ModuleScope::CompositionFamily, "vuo.test.rotateImageList", "vuonode"),
				ModuleLocation(ModuleScope::CompositionFamily, "vuo.list.add.VuoImage", "vuonode"),
				ModuleLocation(ModuleScope::CompositionFamily, "vuo.list.get.first.VuoInteger", "vuonode"),
				ModuleLocation(ModuleScope::CompositionFamily, "vuo.list.make.2.VuoInteger", "vuonode"),
				ModuleLocation(ModuleScope::CompositionFamily, "vuo.list.take.VuoImage", "vuonode"),
				ModuleLocation(ModuleScope::CompositionFamily, "vuo.in.1.List.VuoList_VuoImage", "vuonode"),
				ModuleLocation(ModuleScope::CompositionFamily, "vuo.out.1.RotatedList.VuoList_VuoImage", "vuonode")
			};
			expectedDiff.dylibsModified = {
				ModuleScope::CompositionFamily,
				ModuleScope::Composition
			};
			expectedDiff.manifestShouldContain = {
				{ ModuleScope::CompositionFamily, {"vuo.test.rotateImageList"} }
			};
			expectedDiff.manifestShouldNotContain = {
				{ ModuleScope::System, {"*"} },
				{ ModuleScope::User, {"*"} },
				{ ModuleScope::CompositionFamily, {"vuo.list.get.first.VuoGenericType1"} },
				{ ModuleScope::Composition, {"*"} }
			};
			QTest::newRow("Subcomposition with generic nodes installed in composition-local Modules") << baselineSetup << changeToTest << defaultCompositionDirNames << expectedDiff;
		}

		// Modifying an installed module

		{
			InstalledModulesChange baselineSetup;
			baselineSetup.modulesToInstall = {
				ModuleLocation(ModuleScope::User, "vuo.test.fillRealList", "vuonode")
			};
			InstalledModulesChange changeToTest;
			changeToTest.modulesToModify = {
				ModuleLocation(ModuleScope::User, "vuo.test.fillRealList", "vuonode")
			};
			ModuleCachesDiff expectedDiff;
			expectedDiff.dylibsModified = {
				ModuleScope::User,
				ModuleScope::CompositionFamily,
				ModuleScope::Composition
			};
			expectedDiff.manifestShouldContain = {
				{ ModuleScope::User, {"vuo.test.fillRealList"} }
			};
			QTest::newRow("Compiled node class with built-in dependencies modified in User Modules") << baselineSetup << changeToTest << defaultCompositionDirNames << expectedDiff;
		}
		{
			InstalledModulesChange baselineSetup;
			baselineSetup.modulesToInstall = {
				ModuleLocation(ModuleScope::CompositionFamily, "vuo.test.makeColorOpaque", "vuo")
			};
			InstalledModulesChange changeToTest;
			changeToTest.modulesToModify = {
				ModuleLocation(ModuleScope::CompositionFamily, "vuo.test.makeColorOpaque", "vuo")
			};
			ModuleCachesDiff expectedDiff;
			expectedDiff.compiledModulesModified = {
				ModuleLocation(ModuleScope::CompositionFamily, "vuo.test.makeColorOpaque", "vuonode")
			};
			expectedDiff.dylibsModified = {
				ModuleScope::CompositionFamily,
				ModuleScope::Composition
			};
			expectedDiff.manifestShouldContain = {
				{ ModuleScope::CompositionFamily, {"vuo.test.makeColorOpaque"} }
			};
			QTest::newRow("Subcomposition with generated dependencies modified in composition-local Modules") << baselineSetup << changeToTest << defaultCompositionDirNames << expectedDiff;
		}

		// Deleting an installed module

		{
			InstalledModulesChange baselineSetup;
			baselineSetup.modulesToInstall = {
				ModuleLocation(ModuleScope::User, "vuo.test.fillRealList", "vuonode")
			};
			InstalledModulesChange changeToTest;
			changeToTest.modulesToDelete = {
				ModuleLocation(ModuleScope::User, "vuo.test.fillRealList", "vuonode")
			};
			ModuleCachesDiff expectedDiff;
			expectedDiff.dylibsModified = {
				ModuleScope::User,
				ModuleScope::CompositionFamily,
				ModuleScope::Composition
			};
			expectedDiff.manifestShouldNotContain = {
				{ ModuleScope::User, {"vuo.test.fillRealList"} }
			};
			QTest::newRow("Compiled node class with built-in dependencies deleted from User Modules") << baselineSetup << changeToTest << defaultCompositionDirNames << expectedDiff;
		}
		{
			InstalledModulesChange baselineSetup;
			baselineSetup.modulesToInstall = {
				ModuleLocation(ModuleScope::CompositionFamily, "vuo.test.makeColorOpaque", "vuo")
			};
			InstalledModulesChange changeToTest;
			changeToTest.modulesToDelete = {
				ModuleLocation(ModuleScope::CompositionFamily, "vuo.test.makeColorOpaque", "vuo")
			};
			ModuleCachesDiff expectedDiff;
			expectedDiff.compiledModulesRemoved = {
				ModuleLocation(ModuleScope::CompositionFamily, "vuo.test.makeColorOpaque", "vuonode")
			};
			expectedDiff.dylibsModified = {
				ModuleScope::CompositionFamily,
				ModuleScope::Composition
			};
			expectedDiff.manifestShouldNotContain = {
				{ ModuleScope::CompositionFamily, {"vuo.test.makeColorOpaque"} }
			};
			QTest::newRow("Subcomposition with generated dependencies deleted from composition-local Modules") << baselineSetup << changeToTest << defaultCompositionDirNames << expectedDiff;
		}

		// Overriding an installed module

		{
			InstalledModulesChange baselineSetup;
			baselineSetup.modulesToInstall = {
				ModuleLocation(ModuleScope::User, "vuo.test.makeColorOpaque", "vuo")
			};
			InstalledModulesChange changeToTest;
			changeToTest.modulesToOverride = {
				ModuleLocation(ModuleScope::User, "vuo.test.makeColorOpaque", "vuo")
			};
			ModuleCachesDiff expectedDiff;
			expectedDiff.overriddenCompiledModulesAdded = {
				ModuleLocation(ModuleScope::User, "vuo.test.makeColorOpaque", "vuonode")
			};
			expectedDiff.dylibsModified = {
				ModuleScope::User,
				ModuleScope::CompositionFamily,
				ModuleScope::Composition
			};
			expectedDiff.manifestShouldContain = {
				{ ModuleScope::User, {"vuo.test.makeColorOpaque"} }
			};
			QTest::newRow("Subcomposition in User Modules overridden") << baselineSetup << changeToTest << defaultCompositionDirNames << expectedDiff;
		}
		{
			InstalledModulesChange baselineSetup;
			baselineSetup.modulesToInstall = {
				ModuleLocation(ModuleScope::CompositionFamily, "vuo.test.passThrough", "c")
			};
			InstalledModulesChange changeToTest;
			changeToTest.modulesToOverride = {
				ModuleLocation(ModuleScope::CompositionFamily, "vuo.test.passThrough", "c")
			};
			ModuleCachesDiff expectedDiff;
			expectedDiff.overriddenCompiledModulesAdded = {
				ModuleLocation(ModuleScope::CompositionFamily, "vuo.test.passThrough", "vuonode")
			};
			expectedDiff.dylibsModified = {
				ModuleScope::CompositionFamily,
				ModuleScope::Composition
			};
			expectedDiff.manifestShouldContain = {
				{ ModuleScope::CompositionFamily, {"vuo.test.passThrough"} }
			};
			QTest::newRow("C node class in composition-local Modules overridden") << baselineSetup << changeToTest << defaultCompositionDirNames << expectedDiff;
		}
		{
			InstalledModulesChange baselineSetup;
			baselineSetup.modulesToInstall = {
				ModuleLocation(ModuleScope::User, "vuo.test.makeColorOpaque", "vuo")
			};
			baselineSetup.modulesToOverride = {
				ModuleLocation(ModuleScope::User, "vuo.test.makeColorOpaque", "vuo")
			};
			InstalledModulesChange changeToTest;
			changeToTest.modulesToOverride = {
				ModuleLocation(ModuleScope::User, "vuo.test.makeColorOpaque", "vuo")
			};
			ModuleCachesDiff expectedDiff;
			expectedDiff.overriddenCompiledModulesModified = {
				ModuleLocation(ModuleScope::User, "vuo.test.makeColorOpaque", "vuonode")
			};
			expectedDiff.dylibsModified = {
				ModuleScope::User,
				ModuleScope::CompositionFamily,
				ModuleScope::Composition
			};
			expectedDiff.manifestShouldContain = {
				{ ModuleScope::User, {"vuo.test.makeColorOpaque"} }
			};
			QTest::newRow("Subcomposition in User Modules overridden then overridden again") << baselineSetup << changeToTest << defaultCompositionDirNames << expectedDiff;
		}
		{
			InstalledModulesChange baselineSetup;
			baselineSetup.modulesToInstall = {
				ModuleLocation(ModuleScope::User, "vuo.test.makeColorOpaque", "vuo")
			};
			baselineSetup.modulesToOverride = {
				ModuleLocation(ModuleScope::User, "vuo.test.makeColorOpaque", "vuo")
			};
			InstalledModulesChange changeToTest;
			changeToTest.modulesToRevert = {
				ModuleLocation(ModuleScope::User, "vuo.test.makeColorOpaque", "vuo")
			};
			ModuleCachesDiff expectedDiff;
			expectedDiff.overriddenCompiledModulesRemoved = {
				ModuleLocation(ModuleScope::User, "vuo.test.makeColorOpaque", "vuonode")
			};
			expectedDiff.dylibsModified = {
				ModuleScope::User,
				ModuleScope::CompositionFamily,
				ModuleScope::Composition
			};
			expectedDiff.manifestShouldContain = {
				{ ModuleScope::User, {"vuo.test.makeColorOpaque"} }
			};
			QTest::newRow("Subcomposition in User Modules overridden then reverted") << baselineSetup << changeToTest << defaultCompositionDirNames << expectedDiff;
		}
		{
			InstalledModulesChange baselineSetup;
			baselineSetup.modulesToInstall = {
				ModuleLocation(ModuleScope::User, "vuo.test.makeColorOpaque", "vuo")
			};
			baselineSetup.modulesToOverride = {
				ModuleLocation(ModuleScope::User, "vuo.test.makeColorOpaque", "vuo")
			};
			baselineSetup.modulesToRevert = {
				ModuleLocation(ModuleScope::User, "vuo.test.makeColorOpaque", "vuo")
			};
			InstalledModulesChange changeToTest;
			changeToTest.modulesToOverride = {
				ModuleLocation(ModuleScope::User, "vuo.test.makeColorOpaque", "vuo")
			};
			ModuleCachesDiff expectedDiff;
			expectedDiff.overriddenCompiledModulesAdded = {
				ModuleLocation(ModuleScope::User, "vuo.test.makeColorOpaque", "vuonode")
			};
			expectedDiff.dylibsModified = {
				ModuleScope::User,
				ModuleScope::CompositionFamily,
				ModuleScope::Composition
			};
			expectedDiff.manifestShouldContain = {
				{ ModuleScope::User, {"vuo.test.makeColorOpaque"} }
			};
			QTest::newRow("Subcomposition in User Modules overridden then reverted then overridden") << baselineSetup << changeToTest << defaultCompositionDirNames << expectedDiff;
		}

		// Multiple modules installed

		{
			InstalledModulesChange baselineSetup;
			baselineSetup.modulesToInstall = {
				ModuleLocation(ModuleScope::User, "vuo.test.rotateImageList", "vuo"),
				ModuleLocation(ModuleScope::CompositionFamily, "vuo.test.makeColorOpaque", "vuo")
			};
			InstalledModulesChange changeToTest;
			changeToTest.modulesToModify = {
				ModuleLocation(ModuleScope::User, "vuo.test.rotateImageList", "vuo")
			};
			ModuleCachesDiff expectedDiff;
			expectedDiff.compiledModulesModified = {
				ModuleLocation(ModuleScope::User, "vuo.test.rotateImageList", "vuonode")
			};
			expectedDiff.dylibsModified = {
				ModuleScope::User,
				ModuleScope::CompositionFamily,
				ModuleScope::Composition
			};
			expectedDiff.manifestShouldContain = {
				{ ModuleScope::User, {"vuo.test.rotateImageList", "vuo.in.1.List.VuoList_VuoImage", "vuo.out.1.RotatedList.VuoList_VuoImage"} },
				{ ModuleScope::CompositionFamily, {"vuo.test.makeColorOpaque", "vuo.in.1.Color.VuoColor", "vuo.out.1.OpaqueColor.VuoColor"} }
			};
			QTest::newRow("With modules installed in User Modules and composition-local Modules, module in User Modules modified") << baselineSetup << changeToTest << defaultCompositionDirNames << expectedDiff;
		}
		{
			InstalledModulesChange baselineSetup;
			baselineSetup.modulesToInstall = {
				ModuleLocation(ModuleScope::User, "vuo.test.rotateImageList", "vuo"),
				ModuleLocation(ModuleScope::CompositionFamily, "vuo.test.makeColorOpaque", "vuo")
			};
			InstalledModulesChange changeToTest;
			changeToTest.modulesToModify = {
				ModuleLocation(ModuleScope::CompositionFamily, "vuo.test.makeColorOpaque", "vuo")
			};
			ModuleCachesDiff expectedDiff;
			expectedDiff.compiledModulesModified = {
				ModuleLocation(ModuleScope::CompositionFamily, "vuo.test.makeColorOpaque", "vuonode")
			};
			expectedDiff.dylibsModified = {
				ModuleScope::CompositionFamily,
				ModuleScope::Composition
			};
			expectedDiff.manifestShouldContain = {
				{ ModuleScope::User, {"vuo.test.rotateImageList", "vuo.in.1.List.VuoList_VuoImage", "vuo.out.1.RotatedList.VuoList_VuoImage"} },
				{ ModuleScope::CompositionFamily, {"vuo.test.makeColorOpaque", "vuo.in.1.Color.VuoColor", "vuo.out.1.OpaqueColor.VuoColor"} }
			};
			QTest::newRow("With modules installed in User Modules and composition-local Modules, module in composition-local Modules modified") << baselineSetup << changeToTest << defaultCompositionDirNames << expectedDiff;
		}
		{
			string dir1 = "TestModuleCaches-testModuleCacheUpdates1";
			string dir2 = "TestModuleCaches-testModuleCacheUpdates2";
			QStringList compositionDirNames = { QString::fromStdString(dir1), QString::fromStdString(dir2) };

			InstalledModulesChange baselineSetup;
			baselineSetup.modulesToInstall = {
				ModuleLocation(ModuleScope(ModuleScope::CompositionFamily, dir1), "vuo.test.makeColorOpaque", "vuo")
			};
			InstalledModulesChange changeToTest;
			changeToTest.modulesToInstall = {
				ModuleLocation(ModuleScope(ModuleScope::CompositionFamily, dir2), "vuo.test.rotateImageList", "vuo")
			};
			ModuleCachesDiff expectedDiff;
			expectedDiff.compiledModulesAdded = {
				ModuleLocation(ModuleScope(ModuleScope::CompositionFamily, dir2), "vuo.test.rotateImageList", "vuonode"),
				ModuleLocation(ModuleScope(ModuleScope::CompositionFamily, dir2), "vuo.list.add.VuoImage", "vuonode"),
				ModuleLocation(ModuleScope(ModuleScope::CompositionFamily, dir2), "vuo.list.get.first.VuoInteger", "vuonode"),
				ModuleLocation(ModuleScope(ModuleScope::CompositionFamily, dir2), "vuo.list.make.2.VuoInteger", "vuonode"),
				ModuleLocation(ModuleScope(ModuleScope::CompositionFamily, dir2), "vuo.list.take.VuoImage", "vuonode"),
				ModuleLocation(ModuleScope(ModuleScope::CompositionFamily, dir2), "vuo.in.1.List.VuoList_VuoImage", "vuonode"),
				ModuleLocation(ModuleScope(ModuleScope::CompositionFamily, dir2), "vuo.out.1.RotatedList.VuoList_VuoImage", "vuonode")
			};
			expectedDiff.dylibsModified = {
				ModuleScope(ModuleScope::CompositionFamily, dir2),
				ModuleScope(ModuleScope::Composition, dir2)
			};
			expectedDiff.manifestShouldContain = {
				{ ModuleScope(ModuleScope::CompositionFamily, dir1), {"vuo.test.makeColorOpaque", "vuo.in.1.Color.VuoColor", "vuo.out.1.OpaqueColor.VuoColor"} },
				{ ModuleScope(ModuleScope::CompositionFamily, dir2), {"vuo.test.rotateImageList", "vuo.in.1.List.VuoList_VuoImage", "vuo.out.1.RotatedList.VuoList_VuoImage"} }
			};
			QTest::newRow("With one module installed in one composition-local Modules, second module installed in second composition-local Modules") << baselineSetup << changeToTest << compositionDirNames << expectedDiff;
		}
		{
			string dir1 = "TestModuleCaches-testModuleCacheUpdates1";
			string dir2 = "TestModuleCaches-testModuleCacheUpdates2";
			QStringList compositionDirNames = { QString::fromStdString(dir1), QString::fromStdString(dir2) };

			InstalledModulesChange baselineSetup;
			baselineSetup.modulesToInstall = {
				ModuleLocation(ModuleScope(ModuleScope::CompositionFamily, dir1), "vuo.test.makeColorOpaque", "vuo"),
				ModuleLocation(ModuleScope(ModuleScope::CompositionFamily, dir2), "vuo.test.rotateImageList", "vuo")
			};
			InstalledModulesChange changeToTest;
			changeToTest.modulesToModify = {
				ModuleLocation(ModuleScope(ModuleScope::CompositionFamily, dir1), "vuo.test.makeColorOpaque", "vuo")
			};
			ModuleCachesDiff expectedDiff;
			expectedDiff.compiledModulesModified = {
				ModuleLocation(ModuleScope(ModuleScope::CompositionFamily, dir1), "vuo.test.makeColorOpaque", "vuonode")
			};
			expectedDiff.dylibsModified = {
				ModuleScope(ModuleScope::CompositionFamily, dir1),
				ModuleScope(ModuleScope::Composition, dir1)
			};
			expectedDiff.manifestShouldContain = {
				{ ModuleScope(ModuleScope::CompositionFamily, dir1), {"vuo.test.makeColorOpaque", "vuo.in.1.Color.VuoColor", "vuo.out.1.OpaqueColor.VuoColor"} },
				{ ModuleScope(ModuleScope::CompositionFamily, dir2), {"vuo.test.rotateImageList", "vuo.in.1.List.VuoList_VuoImage", "vuo.out.1.RotatedList.VuoList_VuoImage"} }
			};
			QTest::newRow("With modules installed in two different composition-local Modules, one module modified") << baselineSetup << changeToTest << compositionDirNames << expectedDiff;
		}
	}
	void testModuleCacheUpdates()
	{
		QFETCH(InstalledModulesChange, baselineSetup);
		QFETCH(InstalledModulesChange, changeToTest);
		QFETCH(QStringList, compositionDirNames);
		QFETCH(ModuleCachesDiff, expectedDiff);

		// Set up the baseline state of module caches.

		map<string, VuoCompiler *> compilerForCompositionDirName;
		map<string, TestCompilerDelegate *> delegateForCompositionDirName;

		auto setUpCompiler = [&compilerForCompositionDirName, &delegateForCompositionDirName] (string compositionDirName)
		{
			string compositionDirNameToRegister = ! compositionDirName.empty() ? compositionDirName : "TestModuleCaches-testModuleCacheUpdates";
			string compositionPath = ModuleScope::registerCompositionDirectory(compositionDirNameToRegister);
			VuoCompiler *compiler = new VuoCompiler(compositionPath);
			compilerForCompositionDirName[compositionDirName] = compiler;

			TestCompilerDelegate *delegate = new TestCompilerDelegate;
			compiler->setDelegate(delegate);
			delegateForCompositionDirName[compositionDirName] = delegate;
		};

		if (compositionDirNames.empty())
			setUpCompiler("");
		else
			for (QString dir : compositionDirNames)
				setUpCompiler(dir.toStdString());

		ModuleCachesDiff::setTarget(VuoCompiler::getProcessTarget());

		baselineSetup.doChange(compilerForCompositionDirName, delegateForCompositionDirName);
		for (auto i : compilerForCompositionDirName)
			i.second->makeModuleCachesAvailable(true, false);
		expectedDiff.recordBaseline();

		sleep(1);  // Make sure files modified after this have different timestamps.

		// Install/modify/uninstall modules, and compare the altered module caches to the baseline.

		changeToTest.doChange(compilerForCompositionDirName, delegateForCompositionDirName);
		for (auto i : compilerForCompositionDirName)
			i.second->makeModuleCachesAvailable(true, false);
		expectedDiff.check();

		for (QString dir : compositionDirNames)
		{
			delete compilerForCompositionDirName[dir.toStdString()];
			delete delegateForCompositionDirName[dir.toStdString()];
		}
	}

	void testFallbackWithoutModuleCaches()
	{
		// Create regular files where the module cache directories should be so the compiler can't use them.

		string compositionDir = VUO_SOURCE_DIR "/test/TestModuleCaches/node-TestModuleCaches";
		string compositionPath = compositionDir + "/vuo.test.rotateImageList.vuo";

		vector<shared_ptr<VuoModuleCache>> moduleCaches = {
			VuoModuleCache::newSystemCache(),
			VuoModuleCache::newUserCache(),
			VuoModuleCache::newCache(compositionDir),
			VuoModuleCache::newCache(compositionPath)
		};

		for (shared_ptr<VuoModuleCache> moduleCache : moduleCaches)
		{
			VuoFileUtilities::deleteDir(moduleCache->cacheDirectoryPath);
			VuoFileUtilities::writeStringToFile("", moduleCache->cacheDirectoryPath);
		}

		moduleCaches.clear();

		ModuleCachesDiff expectedDiff;
		expectedDiff.recordBaseline();

		// Make sure a composition can still be compiled and run.

		VuoCompiler compiler(compositionPath);
		TestCompilerDelegate delegate;
		compiler.setDelegate(&delegate);

		InstalledModulesChange change;
		change.doChange({{"", &compiler}}, {{"", &delegate}});

		VuoCompilerIssues issues;
		VuoRunner *runner = VuoCompiler::newSeparateProcessRunnerFromCompositionFile(compositionPath, &issues);
		QVERIFY2(issues.isEmpty(), issues.getLongDescription(false).c_str());

		runner->start();
		runner->firePublishedInputPortEvent(runner->getPublishedInputPorts().front());
		runner->waitForFiredPublishedInputPortEvent();
		runner->stop();

		delete runner;

		// Make sure the compiler didn't thwart this test by creating module caches.

		expectedDiff.check();

		deleteAllModuleCaches();
	}

	void testPublishedNodeClassesCached_data()
	{
		QTest::addColumn<QString>("compositionName");
		QTest::addColumn<QString>("publishedInputNodeClassName");
		QTest::addColumn<QString>("publishedOutputNodeClassName");

		QTest::newRow("Published inputs and outputs") << "PublishedInputsAndOutputs" << "vuo.in.2.Image.NoiseAmount.VuoImage.VuoReal" << "vuo.out.1.FrostedImage.VuoImage";
		QTest::newRow("Published inputs only") << "PublishedInputsOnly" << "vuo.in.1.SendChannels.VuoList_VuoAudioSamples" << "vuo.out.0";
		QTest::newRow("Many published inputs") << "ManyPublishedInputs" << "vuo.in.17.Image.Saturation.Vibrance.HueShift.Temperature.Tint.Contrast.Brightness.Exposure.Gamma.Mask.Shape.Radius.Angle.Symmetric.ExpandBounds.Quality.VuoImage.VuoReal.VuoReal.VuoReal.VuoReal.VuoReal.VuoReal.VuoReal.VuoReal.VuoReal.VuoImage.VuoBlurShape.VuoReal.VuoReal.VuoBoolean.VuoBoolean.VuoReal" << "vuo.out.1.OutputImage.VuoImage";
	}
	void testPublishedNodeClassesCached()
	{
		QFETCH(QString, compositionName);
		QFETCH(QString, publishedInputNodeClassName);
		QFETCH(QString, publishedOutputNodeClassName);

		string origCompositionPath = TestCompositionExecution::getCompositionPath(compositionName.toStdString() + ".vuo");
		string compositionPath = ModuleScope::registerCompositionDirectory("TestModuleCaches-testPublishedNodeClassesCached");
		VuoFileUtilities::copyFile(origCompositionPath, compositionPath);
		ModuleCachesDiff::setTarget(VuoCompiler::getProcessTarget());

		// The first time a composition containing the published node classes is compiled,
		// make sure the published node classes are written to the cache.
		{
			ModuleCachesDiff expectedDiff;
			if (! publishedInputNodeClassName.isEmpty())
				expectedDiff.compiledModulesAdded.push_back(ModuleLocation(ModuleScope::Composition, publishedInputNodeClassName.toStdString(), "vuonode"));
			if (! publishedOutputNodeClassName.isEmpty())
				expectedDiff.compiledModulesAdded.push_back(ModuleLocation(ModuleScope::Composition, publishedOutputNodeClassName.toStdString(), "vuonode"));
			expectedDiff.dylibsAdded = {
				ModuleScope::System,
				ModuleScope::User,
				ModuleScope::CompositionFamily,
				ModuleScope::Composition
			};

			expectedDiff.recordBaseline();

			VuoCompilerIssues issues;
			VuoRunner *runner = VuoCompiler::newSeparateProcessRunnerFromCompositionFile(compositionPath, &issues);
			QVERIFY2(issues.isEmpty(), issues.getLongDescription(false).c_str());

			runner->start();
			runner->firePublishedInputPortEvent(runner->getPublishedInputPorts().front());
			runner->waitForFiredPublishedInputPortEvent();
			runner->stop();
			delete runner;

			expectedDiff.check();
		}

		// The second time the composition is compiled,
		// make sure the compiler uses the cached node classes instead of recompiling them.
		{
			ModuleCachesDiff expectedDiff;
			expectedDiff.recordBaseline();

			VuoCompilerIssues issues;
			VuoRunner *runner = VuoCompiler::newSeparateProcessRunnerFromCompositionFile(compositionPath, &issues);
			QVERIFY2(issues.isEmpty(), issues.getLongDescription(false).c_str());

			runner->start();
			runner->firePublishedInputPortEvent(runner->getPublishedInputPorts().front());
			runner->waitForFiredPublishedInputPortEvent();
			runner->stop();
			delete runner;

			expectedDiff.check();
		}
	}

	void testModuleCacheCleanupOnInitialization()
	{
		string compositionPath = ModuleScope::registerCompositionDirectory("TestModuleCaches-testModuleCacheCleanupOnInitialization");
		ModuleCachesDiff::setTarget(VuoCompiler::getProcessTarget());

		// Simulate out-of-date module caches created by some earlier process.

		for (ModuleScope::Scope s : { ModuleScope::User, ModuleScope::CompositionFamily })
		{
			ModuleScope scope(s);
			string fakeDylibPath = scope.getModuleCache()->getDylibPath(VuoCompiler::getProcessTarget());
			VuoFileUtilities::writeStringToFile("", fakeDylibPath);
		}

		ModuleCachesDiff expectedDiff;
		expectedDiff.recordBaseline();

		// Rebuild the module caches.

		VuoCompiler compiler(compositionPath);
		compiler.makeModuleCachesAvailable(true, false);

		// Check that the old module cache dylibs were deleted.

		expectedDiff.dylibsAdded = {
			ModuleScope::System,
			ModuleScope::Composition
		};
		expectedDiff.dylibsModified = {
			ModuleScope::User,
			ModuleScope::CompositionFamily,
		};
		expectedDiff.check();
	}

	void testModuleCacheCleanupAfterUse_data()
	{
		QTest::addColumn<QString>("mode");
		QTest::addColumn<QString>("compositionName");

		QTest::newRow("Module cache is rebuilt after linking a composition that doesn't use it")      << "executable"       << "BuiltInNodesOnly";
		QTest::newRow("Module cache is rebuilt after linking a composition that uses it")             << "executable"       << "UseFillRealList";
		QTest::newRow("Module cache is rebuilt after running a composition that doesn't use it")      << "live-edit-run"    << "BuiltInNodesOnly";
		QTest::newRow("Module cache is rebuilt after running a composition that uses it")             << "live-edit-run"    << "UseFillRealList";
		QTest::newRow("Module cache is rebuilt after live-editing a composition that doesn't use it") << "live-edit-update" << "BuiltInNodesOnly";
		QTest::newRow("Module cache is rebuilt after live-editing a composition that uses it")        << "live-edit-update" << "UseFillRealList";
	}
	void testModuleCacheCleanupAfterUse()
	{
		QFETCH(QString, mode);
		QFETCH(QString, compositionName);

		// Set up the baseline state of module caches, with one custom module installed.

		string origCompositionPath = TestCompositionExecution::getCompositionPath(compositionName.toStdString() + ".vuo");
		string compositionPath = ModuleScope::registerCompositionDirectory("TestModuleCaches-testModuleCacheCleanupAfterUse");
		if (mode == "executable" || mode == "live-edit-run")
			VuoFileUtilities::copyFile(origCompositionPath, compositionPath);
		else if (mode == "live-edit-update")
			VuoFileUtilities::writeStringToFile("digraph G {}", compositionPath);

		VuoCompiler *compiler = new VuoCompiler(compositionPath);
		TestCompilerDelegate *delegate = new TestCompilerDelegate;
		compiler->setDelegate(delegate);

		ModuleCachesDiff::setTarget(VuoCompiler::getProcessTarget());

		ModuleLocation ml(ModuleScope::User, "vuo.test.fillRealList", "vuonode");
		delegate->installModule(ml.getSourcePath(), ml.getInstalledModulePath());

		compiler->makeModuleCachesAvailable(true, false);

		ModuleCachesDiff expectedDiff;
		expectedDiff.recordBaseline();

		sleep(1);  // Make sure files modified after this have different timestamps.

		// Link and possibly run a composition, which may or may not contain the custom module.

		string compiledCompositionPath = VuoFileUtilities::makeTmpFile(compositionName.toStdString(), "bc");
		VuoCompilerIssues issues;
		compiler->compileComposition(compositionPath, compiledCompositionPath, true, &issues);

		if (mode == "executable")
		{
			string linkedCompositionPath = VuoFileUtilities::makeTmpFile(compositionName.toStdString(), "");
			compiler->linkCompositionToCreateExecutable(compiledCompositionPath, linkedCompositionPath, VuoCompiler::Optimization_ModuleCaches);
			VuoFileUtilities::deleteFile(linkedCompositionPath);
		}
		else if (mode == "live-edit-run" || mode == "live-edit-update")
		{
			string linkedCompositionPath = VuoFileUtilities::makeTmpFile(compositionName.toStdString(), "dylib");
			std::shared_ptr<VuoRunningCompositionLibraries> runningCompositionLibraries = std::make_shared<VuoRunningCompositionLibraries>();
			runningCompositionLibraries->setDeleteResourceLibraries(true);
			compiler->linkCompositionToCreateDynamicLibraries(compiledCompositionPath, linkedCompositionPath, runningCompositionLibraries.get());
			VuoRunner *runner = VuoRunner::newSeparateProcessRunnerFromDynamicLibrary(compiler->getCompositionLoaderPath(), linkedCompositionPath, runningCompositionLibraries, "", false, true);
			runner->start();

			if (mode == "live-edit-update")
			{
				string oldCompositionString = VuoFileUtilities::readFileToString(compositionPath);
				VuoFileUtilities::copyFile(origCompositionPath, compositionPath);
				string newCompositionString = VuoFileUtilities::readFileToString(compositionPath);
				VuoCompilerComposition *newComposition = VuoCompilerComposition::newCompositionFromGraphvizDeclaration(newCompositionString, compiler);
				VuoCompilerCompositionDiff diffInfo;
				string compositionDiff = diffInfo.diff(oldCompositionString, newComposition, compiler);
				compiler->compileComposition(compositionPath, compiledCompositionPath, true, &issues);
				compiler->linkCompositionToCreateDynamicLibraries(compiledCompositionPath, linkedCompositionPath, runningCompositionLibraries.get());
				runner->replaceComposition(linkedCompositionPath, compositionDiff);
			}

			runner->stop();
			delete runner;
		}

		VuoFileUtilities::deleteFile(compiledCompositionPath);

		// Install another custom module to make the module caches rebuild.

		ModuleLocation ml2(ModuleScope::User, "vuo.test.passThrough", "c");
		delegate->installModule(ml2.getSourcePath(), ml2.getInstalledModulePath());

		compiler->makeModuleCachesAvailable(true, false);

		// Check that the old module cache dylibs were deleted.

		expectedDiff.dylibsModified = {
			ModuleScope::User,
			ModuleScope::CompositionFamily,
			ModuleScope::Composition
		};
		expectedDiff.compiledModulesAdded = {
			ModuleLocation(ModuleScope::User, "vuo.test.passThrough", "vuonode"),
		};
		expectedDiff.check();

		delete compiler;
		delete delegate;
	}

	void testModuleCacheCleanupAfterInterferenceFromOtherCompiler()
	{
		// Install a composition-family module and have the first compiler rebuild module caches.

		string origCompositionPath = TestCompositionExecution::getCompositionPath("UseFillRealList.vuo");
		string compositionPath = ModuleScope::registerCompositionDirectory("TestModuleCaches-testModuleCacheCleanupAfterInterferenceFromOtherCompiler");
		VuoFileUtilities::copyFile(origCompositionPath, compositionPath);

		VuoCompiler *compiler = new VuoCompiler(compositionPath);
		TestCompilerDelegate *delegate = new TestCompilerDelegate;
		compiler->setDelegate(delegate);

		ModuleCachesDiff::setTarget(VuoCompiler::getProcessTarget());

		ModuleLocation ml(ModuleScope::CompositionFamily, "vuo.test.fillRealList", "vuonode");
		delegate->installModule(ml.getSourcePath(), ml.getInstalledModulePath());

		compiler->makeModuleCachesAvailable(true, false);

		// Install a user module and have a second compiler rebuild module caches.

		VuoCompiler *otherCompiler = new VuoCompiler;
		TestCompilerDelegate *otherDelegate = new TestCompilerDelegate;
		otherCompiler->setDelegate(otherDelegate);

		ModuleLocation ml2(ModuleScope::User, "vuo.test.passThrough", "c");
		otherDelegate->installModule(ml2.getSourcePath(), ml2.getInstalledModulePath());

		otherCompiler->makeModuleCachesAvailable(true, false);

		ModuleCachesDiff expectedDiff;
		expectedDiff.recordBaseline();

		// Have the first compiler rebuild module caches.

		compiler->makeModuleCachesAvailable(true, false);

		expectedDiff.dylibsModified = {
			ModuleScope::CompositionFamily,
			ModuleScope::Composition
		};
		expectedDiff.check();
	}

	void testModuleCacheCleanupBasedOnTimestamp_data()
	{
		QTest::addColumn<QString>("relativeCacheDirectory");
		QTest::addColumn<int>("daysSinceLastAccess");
		QTest::addColumn<bool>("shouldBeDeleted");

		QTest::newRow("User scope, last accessed 29 days ago") << "User" << 29 << false;
		QTest::newRow("User scope, last accessed 30 days ago") << "User" << 30 << false;
		QTest::newRow("Composition-family scope, last accessed 29 days ago") << "꞉path꞉to꞉compositionDir" << 29 << false;
		QTest::newRow("Composition-family scope, last accessed 30 days ago") << "꞉path꞉to꞉compositionDir" << 30 << true;
		QTest::newRow("Composition scope, last accessed 29 days ago") << "꞉path꞉to꞉compositionDir꞉composition.vuo" << 29 << false;
		QTest::newRow("Composition scope, last accessed 30 days ago") << "꞉path꞉to꞉compositionDir꞉composition.vuo" << 30 << true;
	}
	void testModuleCacheCleanupBasedOnTimestamp()
	{
		QFETCH(QString, relativeCacheDirectory);
		QFETCH(int, daysSinceLastAccess);
		QFETCH(bool, shouldBeDeleted);

		string cacheDirectory = VuoFileUtilities::getCachePath() + "/" + relativeCacheDirectory.toStdString();
		VuoFileUtilities::makeDir(cacheDirectory);

		struct timeval currentTime;
		gettimeofday(&currentTime, NULL);

		struct timeval fileTimes[2];
		fileTimes[0].tv_sec = currentTime.tv_sec - daysSinceLastAccess * 24 * 60 * 60 - 1;
		fileTimes[0].tv_usec = 0;
		fileTimes[1].tv_sec = currentTime.tv_sec - 100 * 24 * 60 * 60;
		fileTimes[1].tv_usec = 0;
		utimes(cacheDirectory.c_str(), (const struct timeval *)&fileTimes);

		VuoModuleCache::deleteOldCaches();

		QCOMPARE(! VuoFileUtilities::dirExists(cacheDirectory), shouldBeDeleted);

		VuoFileUtilities::deleteDir(cacheDirectory);
	}

	void testModuleCacheCleanupBasedOnPid_data()
	{
		QTest::addColumn<QString>("relativeCacheDirectory");
		QTest::addColumn<bool>("shouldBeDeleted");

		ostringstream currentPidDir;
		currentPidDir << "pid-" << getpid();

		QTest::newRow("pid currently running") << QString::fromStdString(currentPidDir.str()) << false;
		QTest::newRow("pid not running") << "pid-999999" << true;
	}
	void testModuleCacheCleanupBasedOnPid()
	{
		QFETCH(QString, relativeCacheDirectory);
		QFETCH(bool, shouldBeDeleted);

		string cacheDirectory = VuoFileUtilities::getCachePath() + "/" + relativeCacheDirectory.toStdString();
		VuoFileUtilities::makeDir(cacheDirectory);

		VuoModuleCache::deleteOldCaches();

		QCOMPARE(! VuoFileUtilities::dirExists(cacheDirectory), shouldBeDeleted);

		VuoFileUtilities::deleteDir(cacheDirectory);
	}

	void testModuleCacheCreationPerformance_data()
	{
		QTest::addColumn< InstalledModulesChange >("baselineSetup");

		{
			InstalledModulesChange baselineSetup;
			QTest::newRow("No modules installed") << baselineSetup;
		}
		{
			vector<ModuleLocation> modulesToInstall;
			string moduleKey = "vuo.test.passThrough";
			for (int i = 0; i < 100; ++i)
			{
				ModuleLocation m(ModuleScope::User, moduleKey, "c");
				ostringstream installedModuleKey;
				installedModuleKey << moduleKey << i;
				m.setInstalledModuleKey(installedModuleKey.str());
				modulesToInstall.push_back(m);
			}

			InstalledModulesChange baselineSetup;
			baselineSetup.modulesToInstall = modulesToInstall;
			QTest::newRow("100 C node classes installed in User Modules") << baselineSetup;
		}
	}
	void testModuleCacheCreationPerformance()
	{
		QFETCH(InstalledModulesChange, baselineSetup);

		string compositionPath = ModuleScope::registerCompositionDirectory("TestModuleCaches-testModuleCacheCreationPerformance");
		ModuleCachesDiff::setTarget(VuoCompiler::getProcessTarget());

		VuoCompiler *compiler = new VuoCompiler(compositionPath);
		TestCompilerDelegate delegate;
		compiler->setDelegate(&delegate);

		baselineSetup.doChange({{"", compiler}}, {{"", &delegate}});

		ModuleCachesDiff expectedDiff;
		expectedDiff.recordBaseline();

		QBENCHMARK_ONCE {
			compiler->makeModuleCachesAvailable(true, false);
		}

		expectedDiff.dylibsAdded = {
			ModuleScope::System,
			ModuleScope::User,
			ModuleScope::CompositionFamily,
			ModuleScope::Composition
		};
		expectedDiff.check();

		delete compiler;
		VuoCompiler::reset();
		deleteAllModuleCacheLockInfo();
	}

	void testModuleCacheCheckingPerformance_data()
	{
		QTest::addColumn< InstalledModulesChange >("baselineSetup");

		{
			InstalledModulesChange baselineSetup;
			QTest::newRow("No modules installed") << baselineSetup;
		}
		{
			vector<ModuleLocation> modulesToInstall;
			string moduleKey = "vuo.test.passThrough";
			for (int i = 0; i < 100; ++i)
			{
				ModuleLocation m(ModuleScope::User, moduleKey, "c");
				ostringstream installedModuleKey;
				installedModuleKey << moduleKey << i;
				m.setInstalledModuleKey(installedModuleKey.str());
				modulesToInstall.push_back(m);
			}

			InstalledModulesChange baselineSetup;
			baselineSetup.modulesToInstall = modulesToInstall;
			QTest::newRow("100 C node classes installed in User Modules") << baselineSetup;
		}
	}
	void testModuleCacheCheckingPerformance()
	{
		QFETCH(InstalledModulesChange, baselineSetup);

		string compositionPath = ModuleScope::registerCompositionDirectory("TestModuleCaches-testModuleCacheCheckingPerformance");
		ModuleCachesDiff::setTarget(VuoCompiler::getProcessTarget());

		VuoCompiler *compiler = new VuoCompiler(compositionPath);
		TestCompilerDelegate delegate;
		compiler->setDelegate(&delegate);

		baselineSetup.doChange({{"", compiler}}, {{"", &delegate}});

		compiler->makeModuleCachesAvailable(true, false);

		ModuleCachesDiff expectedDiff;
		expectedDiff.recordBaseline();

		// Invalidate each module cache so it will be checked but not rebuilt.
		for (const vector<VuoCompilerEnvironment *> &envs : compiler->environments)
		{
			if (! envs[0]->isBuiltIn())
			{
				shared_ptr<VuoModuleCache> moduleCache = envs[0]->getModuleCache();
				string cacheDirectory = moduleCache->cacheDirectoryPath;

				moduleCache->invalidate();

				VuoModuleCache::LockInfo *lockInfo = moduleCache->interprocessLockInfo[cacheDirectory];
				lockInfo->dylibLockFile->unlock();
				delete lockInfo->dylibLockFile;
				lockInfo->dylibLockFile = nullptr;
				VuoFileUtilities::deleteFile(cacheDirectory + "/dylib.lock");
			}
		}

		QBENCHMARK_ONCE {
			compiler->makeModuleCachesAvailable(true, false);
		}

		// Make sure the compiler did check each module cache.
		for (const vector<VuoCompilerEnvironment *> &envs : compiler->environments)
		{
			if (! envs[0]->isBuiltIn())
			{
				shared_ptr<VuoModuleCache> moduleCache = envs[0]->getModuleCache();
				string cacheDirectory = moduleCache->cacheDirectoryPath;
				shared_ptr<VuoModuleCacheRevision> revision = moduleCache->useCurrentRevision();

				QVERIFY2(revision, ("cache not available: " + cacheDirectory).c_str());
				revision->disuse();

				QVERIFY2(VuoFileUtilities::fileExists(cacheDirectory + "/dylib.lock"), ("dylib.lock not recreated: " + cacheDirectory).c_str());
			}
		}

		// Make sure the compiler didn't rebuild any module caches.
		expectedDiff.check();

		delete compiler;
		VuoCompiler::reset();
		deleteAllModuleCacheLockInfo();
	}

};


QTEST_APPLESS_MAIN(TestModuleCaches)
#include "TestModuleCaches.moc"
