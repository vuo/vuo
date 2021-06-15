/**
 * @file
 * TestModuleLoading interface and implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunreachable-code"
#pragma clang diagnostic ignored "-Wdocumentation"
#pragma clang diagnostic ignored "-Winvalid-constexpr"
#include <QtTest/QtTest>
#pragma clang diagnostic pop

#include <Vuo/Vuo.h>
#include "VuoRendererCommon.hh"
#include "TestCompositionExecution.hh"

/**
 * Tests for loading, installing, and modifying 3rd-party modules.
 */
class TestModuleLoading : public QObject
{
	Q_OBJECT

private:

	class TestModuleLoadingCompilerDelegate : public TestCompilerDelegate
	{
	private:
		map<string, VuoCompilerModule *> modulesAdded;
		map<string, VuoCompilerModule *> modulesModified;
		set<string> modulesRemoved;
		dispatch_queue_t modulesQueue;

		void loadedModules(const map<string, VuoCompilerModule *> &modulesAdded_,
						   const map<string, pair<VuoCompilerModule *, VuoCompilerModule *> > &modulesModified,
						   const map<string, VuoCompilerModule *> &modulesRemoved, VuoCompilerIssues *issues)
		{
			map<string, VuoCompilerModule *> modulesAdded = modulesAdded_;
			auto foundPublishedInputTriggerNodeClass = modulesAdded.find("vuo.event.spinOffEvent2");
			if (foundPublishedInputTriggerNodeClass != modulesAdded.end())
				modulesAdded.erase(foundPublishedInputTriggerNodeClass);

			if (modulesModified.empty() && modulesRemoved.empty())
				VUserLog("modulesAdded: %lu%s", modulesAdded.size(),
						 modulesAdded.size() == 1 ? (" (" + modulesAdded.begin()->first + ")").c_str() : "");
			if (! modulesModified.empty())
				VUserLog("modulesModified: %lu%s", modulesModified.size(),
						 modulesModified.size() == 1 ? (" (" + modulesModified.begin()->first + ")").c_str() : "");
			if (! modulesRemoved.empty())
				VUserLog("modulesRemoved: %lu%s", modulesRemoved.size(),
						 modulesRemoved.size() == 1 ? (" (" + modulesRemoved.begin()->first + ")").c_str() : "");
			if (! issues->isEmpty())
				VUserLog("%s", issues->getLongDescription(false).c_str());

			dispatch_sync(modulesQueue, ^{
							  this->modulesAdded.insert(modulesAdded.begin(), modulesAdded.end());
							  for (auto m : modulesModified) {
								  this->modulesModified[m.first] = m.second.second;
							  }
							  for (auto m : modulesRemoved) {
								this->modulesRemoved.insert(m.first);
							  }
						  });

			TestCompilerDelegate::loadedModules(modulesAdded, modulesModified, modulesRemoved, issues);
		}

	public:
		TestModuleLoadingCompilerDelegate(void)
		{
			modulesQueue = dispatch_queue_create("org.vuo.TestModuleLoading", NULL);
		}

		void loadModule(VuoCompiler *compiler, string nodeClassName, map<string, VuoCompilerModule *> &modulesAdded,
						map<string, VuoCompilerModule *> &modulesModified, set<string> &modulesRemoved)
		{
			loadModule(compiler, nodeClassName, nodeClassName, modulesAdded, modulesModified, modulesRemoved);
		}

		void loadModule(VuoCompiler *compiler, string nodeClassToRequest, string nodeClassToWaitOn, map<string, VuoCompilerModule *> &modulesAddedMap,
						map<string, VuoCompilerModule *> &modulesModifiedMap, set<string> &modulesRemoved)
		{
			waiting = true;
			moduleWaitingOn = nodeClassToWaitOn;
			compiler->getNodeClass(nodeClassToRequest);
			dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);
			getLastLoadedModules(modulesAddedMap, modulesModifiedMap, modulesRemoved);
		}

		void watchForModule(string nodeClassName)
		{
			waiting = true;
			moduleWaitingOn = nodeClassName;
		}

		void waitForWatchedModule()
		{
			dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);
		}

		void getLastLoadedModules(map<string, VuoCompilerModule *> &modulesAdded,
								  map<string, VuoCompilerModule *> &modulesModified, set<string> &modulesRemoved)
		{
			dispatch_sync(modulesQueue, ^{
							  modulesAdded = this->modulesAdded;
							  modulesModified = this->modulesModified;
							  modulesRemoved = this->modulesRemoved;

							  this->modulesAdded.clear();
							  this->modulesModified.clear();
							  this->modulesRemoved.clear();
						  });
		}

		void clearLastLoadedModules(void)
		{
			dispatch_sync(modulesQueue, ^{
							  modulesAdded.clear();
							  modulesModified.clear();
							  modulesRemoved.clear();
						  });
		}
	};

	QString getUserModulesPath(void)
	{
		return QString::fromStdString( VuoFileUtilities::getUserModulesPath() );
	}

	QString getCustomModulesPath(void)
	{
		return QString::fromStdString( VuoFileUtilities::getTmpDir() + "/TestModuleLoading/Modules" );
	}

	QString getUserCachedModulesPath(void)
	{
		string arch = VuoCompiler::getTargetArch(VuoCompiler::getProcessTarget());
		return QString::fromStdString( VuoFileUtilities::getCachePath() + "/User/Modules/" + arch );
	}

	QString getCustomCachedModulesPath(void)
	{
		string cachedModulesName = VuoFileUtilities::getTmpDir() + "/TestModuleLoading";
		string arch = VuoCompiler::getTargetArch(VuoCompiler::getProcessTarget());
		return QString::fromStdString( VuoCompiler::getCachePathForComposition(cachedModulesName) + "/Modules/" + arch );
	}

	void uninstallModules(void)
	{
		QString copyPath[4] = { getUserModulesPath(), getCustomModulesPath(), getUserCachedModulesPath(), getCustomCachedModulesPath() };

		for (int i = 0; i < 4; ++i)
		{
			set<VuoFileUtilities::File *> files = VuoFileUtilities::findAllFilesInDirectory(copyPath[i].toStdString());
			for (set<VuoFileUtilities::File *>::iterator j = files.begin(); j != files.end(); ++j)
			{
				if (VuoStringUtilities::beginsWith((*j)->basename(), "vuo.test.TestModuleLoading."))
					VuoFileUtilities::deleteFile((*j)->path());
				delete *j;
			}
		}
	}

	string getNodeClassName(const string &fileName)
	{
		string dir, nodeClassName, ext;
		VuoFileUtilities::splitPath(fileName, dir, nodeClassName, ext);
		return nodeClassName;
	}

	void getNodeClassPaths(const string &fileName, const string &copyPathDescription,
						   string &originalPath, string &installedDirPath, string &installedPath, string &parentCompositionPath, string &cachedPath)
	{
		string cachedModulesPath;
		if (copyPathDescription == "user")
		{
			installedDirPath = getUserModulesPath().toStdString();
			cachedModulesPath = getUserCachedModulesPath().toStdString();
			parentCompositionPath = "";
		}
		else if (copyPathDescription == "custom")
		{
			installedDirPath = getCustomModulesPath().toStdString();

			string parentCompositionDir, file, ext;
			VuoFileUtilities::splitPath(installedDirPath, parentCompositionDir, file, ext);
			VuoFileUtilities::canonicalizePath(parentCompositionDir);
			parentCompositionPath = parentCompositionDir + "/unused";

			cachedModulesPath = getCustomCachedModulesPath().toStdString();
		}
		else
			QFAIL(("Unknown copyPathDescription: " + copyPathDescription).c_str());

		if (VuoStringUtilities::endsWith(fileName, ".vuonode"))
			originalPath = string(BINARY_DIR) + "/test/TestModuleLoading/" + fileName;
		else
			originalPath = "node-TestModuleLoading/" + fileName;
		installedPath = installedDirPath + "/" + fileName;

		string nodeClassName = VuoCompiler::getModuleKeyForPath(fileName);
		cachedPath = cachedModulesPath + "/" + nodeClassName + ".vuonode";
	}

private slots:

	void init()
	{
		uninstallModules();
		VuoFileUtilities::makeDir(getCustomModulesPath().toStdString());
	}

	void cleanup()
	{
		VuoFileUtilities::deleteDir(getCustomModulesPath().toStdString());
		VuoFileUtilities::deleteDir(getCustomCachedModulesPath().toStdString());
		VuoCompiler::reset();
	}

	void testLoadingModulesInitially_data()
	{
		QTest::addColumn<QString>("installedFileName");
		QTest::addColumn<QString>("copyPathDescription");
		QTest::addColumn<bool>("addCopyPathAsSearchPath");
		QTest::addColumn<bool>("shouldLoadAllModules");

		QString installedFileDescription[2] = { "compiled node class", "subcomposition" };
		QString installedFileName[2] = { "vuo.test.TestModuleLoading.nodeClass.vuonode", "vuo.test.TestModuleLoading.subcomposition.vuo" };
		QString copyPathDescription[2] = { "user", "custom" };
		bool addCopyPathAsSearchPath[2] = { false, false };
		bool shouldLoadAllModules[2] = { false, true };

		for (int i = 0; i < 2; ++i)
		{
			for (int j = 0; j < 2; ++j)
			{
				for (int k = 0; k < 2; ++k)
				{
					QString dataTag = installedFileDescription[i] + ", " + copyPathDescription[j] + ", " + (shouldLoadAllModules[k] ? "" : "not ") + "loading all modules";
					QTest::newRow(dataTag.toUtf8().data()) << installedFileName[i] << copyPathDescription[j] << addCopyPathAsSearchPath[j] << shouldLoadAllModules[k];
				}
			}
		}
	}
	void testLoadingModulesInitially()
	{
		QFETCH(QString, installedFileName);
		QFETCH(QString, copyPathDescription);
		QFETCH(bool, addCopyPathAsSearchPath);
		QFETCH(bool, shouldLoadAllModules);

		string nodeClassName = getNodeClassName(installedFileName.toStdString());

		string originalPath, installedDirPath, installedPath, parentCompositionPath, cachedPath;
		getNodeClassPaths(installedFileName.toStdString(), copyPathDescription.toStdString(), originalPath, installedDirPath, installedPath, parentCompositionPath, cachedPath);

		// Install the file.
		VuoFileUtilities::copyFile(originalPath, installedPath);

		// Initialize the compiler.
		VuoCompiler *compiler = new VuoCompiler(parentCompositionPath);
		TestModuleLoadingCompilerDelegate delegate;
		compiler->setDelegate(&delegate);
		compiler->setLoadAllModules(shouldLoadAllModules);
		if (addCopyPathAsSearchPath)
			compiler->addModuleSearchPath(installedDirPath);

		// Load modules for the first time.
		map<string, VuoCompilerModule *> modulesAdded;
		map<string, VuoCompilerModule *> modulesModified;
		set<string> modulesRemoved;
		delegate.loadModule(compiler, nodeClassName, modulesAdded, modulesModified, modulesRemoved);
		QCOMPARE(modulesAdded.size(), (size_t)1);
		QVERIFY(modulesAdded[nodeClassName]);
		QCOMPARE(modulesModified.size(), (size_t)0);
		QCOMPARE(modulesRemoved.size(), (size_t)0);

		map<string, VuoCompilerNodeClass *> nodeClasses = compiler->getNodeClasses();
		if (shouldLoadAllModules)
			QVERIFY(nodeClasses.size() > 1);
		else
		{
			auto foundPublishedInputTriggerNodeClass = nodeClasses.find("vuo.event.spinOffEvent2");
			if (foundPublishedInputTriggerNodeClass != nodeClasses.end())
				nodeClasses.erase(foundPublishedInputTriggerNodeClass);

			QCOMPARE(nodeClasses.size(), (size_t)1);
		}

		delete compiler;
		VuoFileUtilities::deleteFile(cachedPath);
		VuoFileUtilities::deleteFile(installedPath);
	}

	void testAddingAndRemovingModules_data()
	{
		QTest::addColumn<QString>("installedFileName");
		QTest::addColumn<QString>("copyPathDescription");
		QTest::addColumn<bool>("addCopyPathAsSearchPath");
		QTest::addColumn<bool>("shouldLoadAllModules");

		QString installedFileDescription[2] = { "compiled node class", "subcomposition" };
		QString installedFileName[2] = { "vuo.test.TestModuleLoading.nodeClass.vuonode", "vuo.test.TestModuleLoading.subcomposition.vuo" };
		QString copyPathDescription[2] = { "user", "custom" };
		bool addCopyPathAsSearchPath[2] = { false, false };
		bool shouldLoadAllModules[2] = { false, true };

		for (int i = 0; i < 2; ++i)
		{
			for (int j = 0; j < 2; ++j)
			{
				for (int k = 0; k < 2; ++k)
				{
					QString dataTag = installedFileDescription[i] + ", " + copyPathDescription[j] + ", " + (shouldLoadAllModules[k] ? "" : "not ") + "loading all modules";
					QTest::newRow(dataTag.toUtf8().data()) << installedFileName[i] << copyPathDescription[j] << addCopyPathAsSearchPath[j] << shouldLoadAllModules[k];
				}
			}
		}
	}
	void testAddingAndRemovingModules()
	{
		QFETCH(QString, installedFileName);
		QFETCH(QString, copyPathDescription);
		QFETCH(bool, addCopyPathAsSearchPath);
		QFETCH(bool, shouldLoadAllModules);

		string nodeClassName = getNodeClassName(installedFileName.toStdString());
		string controlNodeClassName = "vuo.test.TestModuleLoading.control";

		string originalPath, installedDirPath, installedPath, parentCompositionPath, cachedPath;
		getNodeClassPaths(installedFileName.toStdString(), copyPathDescription.toStdString(), originalPath, installedDirPath, installedPath, parentCompositionPath, cachedPath);
		string controlOriginalPath, controlInstalledDirPath, controlInstalledPath, unused1, unused2;
		getNodeClassPaths(controlNodeClassName + ".vuonode", copyPathDescription.toStdString(), controlOriginalPath, controlInstalledDirPath, controlInstalledPath, unused1, unused2);

		// Install a node class as a control, to make sure it's unaffected.
		VuoFileUtilities::copyFile(controlOriginalPath, controlInstalledPath);

		// Initialize the compiler.
		VuoCompiler *compiler = new VuoCompiler(parentCompositionPath);
		TestModuleLoadingCompilerDelegate delegate;
		compiler->setDelegate(&delegate);
		compiler->setLoadAllModules(shouldLoadAllModules);
		if (addCopyPathAsSearchPath)
			compiler->addModuleSearchPath(installedDirPath);

		// Load modules for the first time.
		map<string, VuoCompilerModule *> modulesAdded;
		map<string, VuoCompilerModule *> modulesModified;
		set<string> modulesRemoved;
		delegate.watchForModule(controlNodeClassName);
		if (shouldLoadAllModules)
			compiler->getNodeClasses();
		VuoCompilerNodeClass *controlNodeClass = compiler->getNodeClass(controlNodeClassName);
		QVERIFY(controlNodeClass);
		delegate.waitForWatchedModule();

		VuoCompilerNodeClass *nodeClassBeforeInstalled = compiler->getNodeClass(nodeClassName);
		QVERIFY(! nodeClassBeforeInstalled);

		delegate.clearLastLoadedModules();
		modulesAdded.clear();
		modulesModified.clear();
		modulesRemoved.clear();

		// Install the file being tested.
		delegate.installModule(originalPath, installedPath);

		// Modules are reloaded.
		if (shouldLoadAllModules)
			compiler->getNodeClass("vuo.event.fireOnStart");
		else
			compiler->getNodeClass(nodeClassName);
		delegate.getLastLoadedModules(modulesAdded, modulesModified, modulesRemoved);

		QCOMPARE(modulesAdded.size(), (size_t)1);
		QVERIFY(modulesAdded[nodeClassName]);
		QCOMPARE(modulesModified.size(), (size_t)0);
		QCOMPARE(modulesRemoved.size(), (size_t)0);

		VuoCompilerNodeClass *controlNodeClass2 = compiler->getNodeClass(controlNodeClassName);
		QVERIFY(controlNodeClass == controlNodeClass2);

		// Uninstall the file being tested.
		delegate.uninstallModule(installedPath);

		// Modules are reloaded.
		VuoCompilerNodeClass *nodeClassAfterUninstalled = compiler->getNodeClass(nodeClassName);
		QVERIFY(! nodeClassAfterUninstalled);

		delegate.getLastLoadedModules(modulesAdded, modulesModified, modulesRemoved);

		QCOMPARE(modulesAdded.size(), (size_t)0);
		QCOMPARE(modulesModified.size(), (size_t)0);
		QCOMPARE(modulesRemoved.size(), (size_t)1);
		QVERIFY(modulesRemoved.find(nodeClassName) != modulesRemoved.end());

		QVERIFY(! VuoFileUtilities::fileExists(cachedPath));

		VuoCompilerNodeClass *controlNodeClass3 = compiler->getNodeClass(controlNodeClassName);
		QVERIFY(controlNodeClass == controlNodeClass3);

		delete compiler;
		VuoFileUtilities::deleteFile(controlInstalledPath);
	}

	void testModifyingModules_data()
	{
		QTest::addColumn<QString>("installedFileName");
		QTest::addColumn<QString>("copyPathDescription");
		QTest::addColumn<bool>("addCopyPathAsSearchPath");
		QTest::addColumn<bool>("shouldLoadAllModules");

		QString installedFileDescription[2] = { "compiled node class", "subcomposition" };
		QString installedFileName[2] = { "vuo.test.TestModuleLoading.nodeClass.vuonode", "vuo.test.TestModuleLoading.subcomposition.vuo" };
		QString copyPathDescription[2] = { "user", "custom" };
		bool addCopyPathAsSearchPath[2] = { false, false };
		bool shouldLoadAllModules[2] = { false, true };

		for (int i = 0; i < 2; ++i)
		{
			for (int j = 0; j < 2; ++j)
			{
				for (int k = 0; k < 2; ++k)
				{
					QString dataTag = installedFileDescription[i] + ", " + copyPathDescription[j] + ", " + (shouldLoadAllModules[k] ? "" : "not ") + "loading all modules";
					QTest::newRow(dataTag.toUtf8().data()) << installedFileName[i] << copyPathDescription[j] << addCopyPathAsSearchPath[j] << shouldLoadAllModules[k];
				}
			}
		}
	}
	void testModifyingModules()
	{
		QFETCH(QString, installedFileName);
		QFETCH(QString, copyPathDescription);
		QFETCH(bool, addCopyPathAsSearchPath);
		QFETCH(bool, shouldLoadAllModules);

		string nodeClassName = getNodeClassName(installedFileName.toStdString());

		string originalPath, installedDirPath, installedPath, parentCompositionPath, cachedPath;
		getNodeClassPaths(installedFileName.toStdString(), copyPathDescription.toStdString(), originalPath, installedDirPath, installedPath, parentCompositionPath, cachedPath);

		// Install the file.
		VuoFileUtilities::copyFile(originalPath, installedPath);

		// Initialize the compiler.
		VuoCompiler *compiler = new VuoCompiler(parentCompositionPath);
		TestModuleLoadingCompilerDelegate delegate;
		compiler->setDelegate(&delegate);
		compiler->setLoadAllModules(shouldLoadAllModules);
		if (addCopyPathAsSearchPath)
			compiler->addModuleSearchPath(installedDirPath);

		// Load modules for the first time.
		map<string, VuoCompilerModule *> modulesAdded;
		map<string, VuoCompilerModule *> modulesModified;
		set<string> modulesRemoved;
		delegate.loadModule(compiler, nodeClassName, modulesAdded, modulesModified, modulesRemoved);
		QCOMPARE(modulesAdded.size(), (size_t)1);
		QVERIFY(modulesAdded[nodeClassName]);
		QCOMPARE(modulesModified.size(), (size_t)0);
		QCOMPARE(modulesRemoved.size(), (size_t)0);

		delegate.clearLastLoadedModules();

		// Reinstall the file.
		sleep(2);
		delegate.installModuleWithSuperficialChange(originalPath, installedPath);

		// Modules are reloaded.
		modulesAdded.clear();
		if (shouldLoadAllModules)
			compiler->getNodeClass("vuo.event.fireOnStart");
		else
			compiler->getNodeClass(nodeClassName);
		delegate.getLastLoadedModules(modulesAdded, modulesModified, modulesRemoved);

		QCOMPARE(modulesModified.size(), (size_t)1);
		QVERIFY(modulesModified[nodeClassName]);
		QCOMPARE(modulesAdded.size(), (size_t)0);
		QCOMPARE(modulesRemoved.size(), (size_t)0);

		delete compiler;
		VuoFileUtilities::deleteFile(cachedPath);
		VuoFileUtilities::deleteFile(installedPath);
	}

	void testDeletingCachedModulesWithoutSourceFiles_data()
	{
		QTest::addColumn<QString>("copyPathDescription");
		QTest::addColumn<bool>("addCopyPathAsSearchPath");
		QTest::addColumn<bool>("shouldLoadAllModules");

		QString copyPathDescription[2] = { "user", "custom" };
		bool addCopyPathAsSearchPath[2] = { false, false };
		bool shouldLoadAllModules[2] = { false, true };

		for (int j = 0; j < 2; ++j)
		{
			for (int k = 0; k < 2; ++k)
			{
				QString dataTag = copyPathDescription[j] + ", " + (shouldLoadAllModules[k] ? "" : "not ") + "loading all modules";
				QTest::newRow(dataTag.toUtf8().data()) << copyPathDescription[j] << addCopyPathAsSearchPath[j] << shouldLoadAllModules[k];
			}
		}
	}
	void testDeletingCachedModulesWithoutSourceFiles()
	{
		QFETCH(QString, copyPathDescription);
		QFETCH(bool, addCopyPathAsSearchPath);
		QFETCH(bool, shouldLoadAllModules);

		QString installedFileName = "vuo.test.TestModuleLoading.nodeClass.vuonode";
		string nodeClassName = getNodeClassName(installedFileName.toStdString());

		string originalPath, installedDirPath, installedPath, parentCompositionPath, cachedPath;
		getNodeClassPaths(installedFileName.toStdString(), copyPathDescription.toStdString(), originalPath, installedDirPath, installedPath, parentCompositionPath, cachedPath);

		string cachedDirPath, file, ext;
		VuoFileUtilities::splitPath(cachedPath, cachedDirPath, file, ext);
		VuoFileUtilities::makeDir(cachedDirPath);

		// Install the cached module.
		VuoFileUtilities::copyFile(originalPath, cachedPath);
		QVERIFY(VuoFileUtilities::fileExists(cachedPath));

		// Initialize the compiler.
		VuoCompiler *compiler = new VuoCompiler(parentCompositionPath);
		TestModuleLoadingCompilerDelegate delegate;
		compiler->setDelegate(&delegate);
		compiler->setLoadAllModules(shouldLoadAllModules);
		if (addCopyPathAsSearchPath)
			compiler->addModuleSearchPath(installedDirPath);

		// Load modules for the first time.
		map<string, VuoCompilerModule *> modulesAdded;
		map<string, VuoCompilerModule *> modulesModified;
		set<string> modulesRemoved;
		compiler->getNodeClass(nodeClassName);
		delegate.getLastLoadedModules(modulesAdded, modulesModified, modulesRemoved);
		QVERIFY(modulesAdded.find(nodeClassName) == modulesAdded.end());
		QVERIFY(modulesModified.find(nodeClassName) == modulesModified.end());
		QVERIFY(modulesRemoved.find(nodeClassName) == modulesRemoved.end());

		QVERIFY(! VuoFileUtilities::fileExists(cachedPath));

		delete compiler;
	}

	void testAllModulesLoaded()
	{
		QString installedFileName[2] = { "vuo.test.TestModuleLoading.control.vuonode", "vuo.test.TestModuleLoading.nodeClass.vuonode" };
		QString copyPathDescription[2] = { "user", "custom" };
		string nodeClassName[2];
		string originalPath[2];
		string installedDirPath[2];
		string installedPath[2];
		string parentCompositionPath[2];
		string cachedPath[2];

		for (int i = 0; i < 2; ++i)
		{
			nodeClassName[i] = getNodeClassName(installedFileName[i].toStdString());
			getNodeClassPaths(installedFileName[i].toStdString(), copyPathDescription[i].toStdString(), originalPath[i], installedDirPath[i], installedPath[i], parentCompositionPath[i], cachedPath[i]);
		}

		// Install the files, one in the user modules folder and the other in the custom modules folder.
		VuoFileUtilities::copyFile(originalPath[0], installedPath[0]);
		VuoFileUtilities::copyFile(originalPath[1], installedPath[1]);

		// Initialize the first compiler.
		VuoCompiler *compiler1 = new VuoCompiler();
		TestModuleLoadingCompilerDelegate delegate1;
		compiler1->setDelegate(&delegate1);
		compiler1->setLoadAllModules(true);

		// Load all modules.
		map<string, VuoCompilerModule *> modulesAdded1;
		map<string, VuoCompilerModule *> modulesModified;
		set<string> modulesRemoved;
		compiler1->getNodeClasses();
		sleep(1);
		delegate1.getLastLoadedModules(modulesAdded1, modulesModified, modulesRemoved);
		QVERIFY(modulesAdded1[ "vuo.event.fireOnStart" ]);
		QVERIFY(modulesAdded1[ "vuo.app.launch" ]);
		QVERIFY(modulesAdded1[ nodeClassName[0] ]);
		QVERIFY(modulesAdded1.find(nodeClassName[1]) == modulesAdded1.end());

		// Destroy the first compiler.
		delete compiler1;
		VuoCompiler::reset();

		// Initialize the second compiler, adding the composition-local module search path.
		VuoCompiler *compiler2 = new VuoCompiler(parentCompositionPath[1]);
		TestModuleLoadingCompilerDelegate delegate2;
		compiler2->setDelegate(&delegate2);
		compiler2->setLoadAllModules(true);

		// Load all modules.
		map<string, VuoCompilerModule *> modulesAdded2;
		compiler2->getNodeClasses();
		sleep(1);
		delegate2.getLastLoadedModules(modulesAdded2, modulesModified, modulesRemoved);
		QVERIFY(modulesAdded2[ "vuo.event.fireOnStart" ]);
		QVERIFY(modulesAdded2[ "vuo.app.launch" ]);
		QVERIFY(modulesAdded2[ nodeClassName[0] ]);
		QVERIFY(modulesAdded2[ nodeClassName[1] ]);
		QCOMPARE(modulesAdded1.size() + 1, modulesAdded2.size());

		// Destroy the second compiler.
		delete compiler2;
		VuoFileUtilities::deleteFile(installedPath[0]);
		VuoFileUtilities::deleteFile(installedPath[1]);
	}

	void testDependenciesLoaded_data()
	{
		QTest::addColumn<QString>("installedFileName");
		QTest::addColumn<QString>("dependencyFileName");
		QTest::addColumn<QString>("copyPathDescription");
		QTest::addColumn<QString>("depCopyPathDescription");

		QString installedFileDescription[4] = { "compiled node class", "compiled node class", "subcomposition", "subcomposition" };
		QString dependencyFileDescription[4] = { "compiled node class", "subcomposition", "compiled node class", "subcomposition" };
		QString installedFileName[4] = { "vuo.test.TestModuleLoading.dependsOnNodeClass.vuonode",
										 "vuo.test.TestModuleLoading.dependsOnSubcomposition.vuonode",
										 "vuo.test.TestModuleLoading.dependsOnNodeClass.vuo",
										 "vuo.test.TestModuleLoading.dependsOnSubcomposition.vuo" };
		QString dependencyFileName[4] = { "vuo.test.TestModuleLoading.nodeClass.vuonode",
										  "vuo.test.TestModuleLoading.subcomposition.vuo",
										  "vuo.test.TestModuleLoading.nodeClass.vuonode",
										  "vuo.test.TestModuleLoading.subcomposition.vuo" };
		QString copyPathDescription[3] = { "user", "custom", "custom" };
		QString depCopyPathDescription[3] = { "user", "custom", "user" };

		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 3; ++j)
			{
				QString dataTag = installedFileDescription[i] + " (" + copyPathDescription[j] + ") depends on " + dependencyFileDescription[i] + " (" + depCopyPathDescription[j] + ")";
				QTest::newRow(dataTag.toUtf8().data()) << installedFileName[i] << dependencyFileName[i] << copyPathDescription[j] << depCopyPathDescription[j];
			}
		}
	}
	void testDependenciesLoaded()
	{
		QFETCH(QString, installedFileName);
		QFETCH(QString, dependencyFileName);
		QFETCH(QString, copyPathDescription);
		QFETCH(QString, depCopyPathDescription);

		string nodeClassName = getNodeClassName(installedFileName.toStdString());
		string depNodeClassName = getNodeClassName(dependencyFileName.toStdString());

		string originalPath, installedDirPath, installedPath, parentCompositionPath, cachedPath;
		getNodeClassPaths(installedFileName.toStdString(), copyPathDescription.toStdString(), originalPath, installedDirPath, installedPath, parentCompositionPath, cachedPath);
		string depOriginalPath, depInstalledDirPath, depInstalledPath, depParentCompositionPath, depCachedPath;
		getNodeClassPaths(dependencyFileName.toStdString(), depCopyPathDescription.toStdString(), depOriginalPath, depInstalledDirPath, depInstalledPath, depParentCompositionPath, depCachedPath);

		string compilerCompositionPath = (copyPathDescription == "custom" ? parentCompositionPath : depParentCompositionPath);

		// Install the module and its dependency.
		VuoFileUtilities::copyFile(originalPath, installedPath);
		VuoFileUtilities::copyFile(depOriginalPath, depInstalledPath);

		// Initialize the compiler.
		VuoCompiler *compiler = new VuoCompiler(compilerCompositionPath);
		TestModuleLoadingCompilerDelegate delegate;
		compiler->setDelegate(&delegate);
		compiler->setLoadAllModules(false);

		// Load the module. Its dependency should be loaded along with it.
		map<string, VuoCompilerModule *> modulesAdded;
		map<string, VuoCompilerModule *> modulesModified;
		set<string> modulesRemoved;
		delegate.loadModule(compiler, nodeClassName, modulesAdded, modulesModified, modulesRemoved);
		QVERIFY(modulesAdded[nodeClassName]);
		QEXPECT_FAIL("compiled node class (user) depends on subcomposition (user)", "https://b33p.net/kosada/node/14316", Abort);
		QEXPECT_FAIL("compiled node class (custom) depends on subcomposition (custom)", "https://b33p.net/kosada/node/14316", Abort);
		QEXPECT_FAIL("compiled node class (custom) depends on subcomposition (user)", "https://b33p.net/kosada/node/14316", Abort);
		QVERIFY(modulesAdded[depNodeClassName]);
		QCOMPARE(modulesAdded.size(), (size_t)2);

		delete compiler;
	}

	void testDependentModulesUnloaded_data()
	{
		QTest::addColumn<QString>("installedFileName");
		QTest::addColumn<QString>("dependencyFileName");
		QTest::addColumn<QString>("copyPathDescription");
		QTest::addColumn<QString>("depCopyPathDescription");

		QString installedFileDescription[4] = { "compiled node class", "compiled node class", "subcomposition", "subcomposition" };
		QString dependencyFileDescription[4] = { "compiled node class", "subcomposition", "compiled node class", "subcomposition" };
		QString installedFileName[4] = { "vuo.test.TestModuleLoading.dependsOnNodeClass.vuonode",
										 "vuo.test.TestModuleLoading.dependsOnSubcomposition.vuonode",
										 "vuo.test.TestModuleLoading.dependsOnNodeClass.vuo",
										 "vuo.test.TestModuleLoading.dependsOnSubcomposition.vuo" };
		QString dependencyFileName[4] = { "vuo.test.TestModuleLoading.nodeClass.vuonode",
										  "vuo.test.TestModuleLoading.subcomposition.vuo",
										  "vuo.test.TestModuleLoading.nodeClass.vuonode",
										  "vuo.test.TestModuleLoading.subcomposition.vuo" };
		QString copyPathDescription[3] = { "user", "custom", "custom" };
		QString depCopyPathDescription[3] = { "user", "custom", "user" };

		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 3; ++j)
			{
				QString dataTag = installedFileDescription[i] + " (" + copyPathDescription[j] + ") depends on " + dependencyFileDescription[i] + " (" + depCopyPathDescription[j] + ")";
				QTest::newRow(dataTag.toUtf8().data()) << installedFileName[i] << dependencyFileName[i] << copyPathDescription[j] << depCopyPathDescription[j];
			}
		}
	}
	void testDependentModulesUnloaded()
	{
		QFETCH(QString, installedFileName);
		QFETCH(QString, dependencyFileName);
		QFETCH(QString, copyPathDescription);
		QFETCH(QString, depCopyPathDescription);

		string nodeClassName = getNodeClassName(installedFileName.toStdString());
		string depNodeClassName = getNodeClassName(dependencyFileName.toStdString());

		string originalPath, installedDirPath, installedPath, parentCompositionPath, cachedPath;
		getNodeClassPaths(installedFileName.toStdString(), copyPathDescription.toStdString(), originalPath, installedDirPath, installedPath, parentCompositionPath, cachedPath);
		string depOriginalPath, depInstalledDirPath, depInstalledPath, depParentCompositionPath, depCachedPath;
		getNodeClassPaths(dependencyFileName.toStdString(), depCopyPathDescription.toStdString(), depOriginalPath, depInstalledDirPath, depInstalledPath, depParentCompositionPath, depCachedPath);

		string compilerCompositionPath = (copyPathDescription == "custom" ? parentCompositionPath : depParentCompositionPath);

		// Install the module and its dependency.
		VuoFileUtilities::copyFile(originalPath, installedPath);
		VuoFileUtilities::copyFile(depOriginalPath, depInstalledPath);

		// Initialize the compiler.
		VuoCompiler *compiler = new VuoCompiler(compilerCompositionPath);
		TestModuleLoadingCompilerDelegate delegate;
		compiler->setDelegate(&delegate);
		compiler->setLoadAllModules(false);

		// Load the module and its dependency.
		map<string, VuoCompilerModule *> modulesAdded;
		map<string, VuoCompilerModule *> modulesModified;
		set<string> modulesRemoved;
		delegate.loadModule(compiler, depNodeClassName, modulesAdded, modulesModified, modulesRemoved);
		delegate.loadModule(compiler, nodeClassName, modulesAdded, modulesModified, modulesRemoved);

		// Modify the dependency. The module should be modified along with it.
		sleep(2);
		delegate.installModuleWithSuperficialChange(depOriginalPath, depInstalledPath);
		bool found = false;
		for (int tries = 0; tries < 2; ++tries)
		{
			modulesModified.clear();
			delegate.getLastLoadedModules(modulesAdded, modulesModified, modulesRemoved);
			if (modulesModified.find(nodeClassName) != modulesModified.end())
			{
				found = true;
				break;
			}
			sleep(1);
		}
		if (! found)
		{
			QEXPECT_FAIL("compiled node class (user) depends on subcomposition (user)", "https://b33p.net/kosada/node/14316", Abort);
			QEXPECT_FAIL("compiled node class (custom) depends on subcomposition (custom)", "https://b33p.net/kosada/node/14316", Abort);
			QEXPECT_FAIL("compiled node class (custom) depends on subcomposition (user)", "https://b33p.net/kosada/node/14316", Abort);
			QVERIFY(false);
		}

		// Remove the dependency. The module should be removed along with it.
		delegate.uninstallModule(depInstalledPath);
		found = false;
		for (int tries = 0; tries < 2; ++tries)
		{
			modulesRemoved.clear();
			delegate.getLastLoadedModules(modulesAdded, modulesModified, modulesRemoved);
			if (modulesRemoved.find(nodeClassName) != modulesRemoved.end())
			{
				found = true;
				break;
			}
			sleep(1);
		}
		if (! found)
			QFAIL("");
	}

	void testOverridingModules_data()
	{
		QTest::addColumn<QString>("initialSourceCode");
		QTest::addColumn<QString>("firstOverride");
		QTest::addColumn<QString>("secondOverride");
		QTest::addColumn<QString>("finalSourceCode");
		QTest::addColumn<bool>("finalNodeClassSameAsLastOverride");

		QString initialSourceCode =
			"digraph G { }";
		QString overridingSourceCode[] = {
			"digraph G { FireOnStart [type=\"vuo.event.fireOnStart\" label=\"Fire on Start|<refresh>refresh\\l|<started>started\\r\"]; }",
			"digraph G { FirePeriodically [type=\"vuo.time.firePeriodically2\" label=\"Fire Periodically|<refresh>refresh\\l|<seconds>seconds\\l|<fired>fired\\r\"]; }"
		};
		QString lastOverrideModified =
			"digraph G { FirePeriodically [type=\"vuo.time.firePeriodically2\" label=\"Fire Periodically|<refresh>refresh\\l|<seconds>seconds\\l|<fired>fired\\r\" _seconds=\"2.0\"]; }";

		QTest::newRow("save last override") << initialSourceCode << overridingSourceCode[0] << overridingSourceCode[1] << overridingSourceCode[1] << true;
		QTest::newRow("save last override with input port value modified") << initialSourceCode << overridingSourceCode[0] << overridingSourceCode[1] << lastOverrideModified << false;
		QTest::newRow("revert overrides") << initialSourceCode << overridingSourceCode[0] << overridingSourceCode[1] << initialSourceCode << false;
	}
	void testOverridingModules()
	{
		QFETCH(QString, initialSourceCode);
		QFETCH(QString, firstOverride);
		QFETCH(QString, secondOverride);
		QFETCH(QString, finalSourceCode);
		QFETCH(bool, finalNodeClassSameAsLastOverride);

		string nodeClassName = "vuo.test.TestModuleLoading.override";
		string originalDependency = "vuo.event.spinOffEvent2";
		string overridingDependencies[] = { "vuo.event.fireOnStart", "vuo.time.firePeriodically2" };

		string originalPath, installedDirPath, installedPath, parentCompositionPath, cachedPath;
		getNodeClassPaths(nodeClassName + ".vuo", "user", originalPath, installedDirPath, installedPath, parentCompositionPath, cachedPath);

		// Install the original subcomposition.
		VuoFileUtilities::writeStringToFile(initialSourceCode.toStdString(), installedPath);

		// Initialize the compiler.
		VuoCompiler *compiler = new VuoCompiler(parentCompositionPath);
		TestModuleLoadingCompilerDelegate delegate;
		compiler->setDelegate(&delegate);

		// Load modules for the first time.
		delegate.watchForModule(nodeClassName);
		compiler->getNodeClasses();
		delegate.waitForWatchedModule();

		VuoCompilerNodeClass *originalNodeClass = compiler->getNodeClass(nodeClassName);
		QVERIFY(originalNodeClass);
		QVERIFY(originalNodeClass->getDependencies().size() == 1);
		QVERIFY(*originalNodeClass->getDependencies().begin() == originalDependency);
		VuoCompilerNodeClass *prevNodeClass = originalNodeClass;

		delegate.clearLastLoadedModules();

		for (int overrideAndSaveIteration = 0; overrideAndSaveIteration < 2; ++overrideAndSaveIteration)
		{
			map<string, VuoCompilerModule *> modulesAdded;
			map<string, VuoCompilerModule *> modulesModified;
			set<string> modulesRemoved;

			// Override the installed subcomposition with an in-memory version, and that with another in-memory version.
			for (int overrideIteration = 0; overrideIteration < 2; ++overrideIteration)
			{
				delegate.watchForModule(nodeClassName);
				string overrideSourceCode = (overrideIteration == 0 ? firstOverride : secondOverride).toStdString();
				compiler->overrideInstalledNodeClass(installedPath, overrideSourceCode);
				delegate.waitForWatchedModule();

				delegate.getLastLoadedModules(modulesAdded, modulesModified, modulesRemoved);
				QVERIFY(modulesAdded.empty());
				QCOMPARE(modulesModified.size(), (size_t)1);
				QVERIFY(modulesRemoved.empty());

				VuoCompilerNodeClass *overridingNodeClass = dynamic_cast<VuoCompilerNodeClass *>( modulesModified[nodeClassName] );
				QVERIFY(overridingNodeClass);
				QVERIFY(overridingNodeClass != prevNodeClass);
				prevNodeClass = overridingNodeClass;

				set<string> dependencies = overridingNodeClass->getDependencies();
				QVERIFY(dependencies.find(overridingDependencies[overrideIteration]) != dependencies.end());
			}

			// Save the latest in-memory version to file (with or without modifying a constant input port value),
			// replacing the installed subcomposition.
			sleep(1);
			VuoFileUtilities::writeStringToFile(finalSourceCode.toStdString(), installedPath);
			sleep(1);
			VuoCompilerNodeClass *savedNodeClass = compiler->getNodeClass(nodeClassName);
			QVERIFY(finalNodeClassSameAsLastOverride ? savedNodeClass == prevNodeClass : savedNodeClass != prevNodeClass);
			prevNodeClass = savedNodeClass;

			delegate.getLastLoadedModules(modulesAdded, modulesModified, modulesRemoved);
			QVERIFY(modulesAdded.empty());
			QCOMPARE(modulesModified.size(), (size_t)(finalNodeClassSameAsLastOverride ? 0 : 1));
			QVERIFY(modulesRemoved.empty());
		}

		delete compiler;
		VuoFileUtilities::deleteFile(installedPath);
	}

	void testRemovingOverriddenModules()
	{
		string nodeClassName = "vuo.test.TestModuleLoading.subcomposition";
		string overridingSourceCode =
			"digraph G { FireOnStart [type=\"vuo.event.fireOnStart\" label=\"Fire on Start|<refresh>refresh\\l|<started>started\\r\"]; }";

		string originalPath, installedDirPath, installedPath, parentCompositionPath, cachedPath;
		getNodeClassPaths(nodeClassName + ".vuo", "user", originalPath, installedDirPath, installedPath, parentCompositionPath, cachedPath);

		// Install the original subcomposition.
		VuoFileUtilities::copyFile(originalPath, installedPath);

		// Initialize the compiler.
		VuoCompiler *compiler = new VuoCompiler(parentCompositionPath);
		TestModuleLoadingCompilerDelegate delegate;
		compiler->setDelegate(&delegate);

		// Load the subcomposition module.
		delegate.watchForModule(nodeClassName);
		VuoCompilerNodeClass *originalNodeClass = compiler->getNodeClass(nodeClassName);
		delegate.waitForWatchedModule();
		QVERIFY(originalNodeClass);

		// Override the installed subcomposition with an in-memory version.
		delegate.watchForModule(nodeClassName);
		compiler->overrideInstalledNodeClass(installedPath, overridingSourceCode);
		delegate.waitForWatchedModule();

		delegate.clearLastLoadedModules();

		// Uninstall the subcomposition.
		delegate.watchForModule(nodeClassName);
		VuoFileUtilities::deleteFile(installedPath);
		delegate.waitForWatchedModule();

		map<string, VuoCompilerModule *> modulesAdded;
		map<string, VuoCompilerModule *> modulesModified;
		set<string> modulesRemoved;

		delegate.getLastLoadedModules(modulesAdded, modulesModified, modulesRemoved);
		QVERIFY(modulesAdded.empty());
		QVERIFY(modulesModified.empty());
		QCOMPARE(modulesRemoved.size(), (size_t)1);
	}

};

int main(int argc, char *argv[])
{
	qInstallMessageHandler(VuoRendererCommon::messageHandler);
	TestModuleLoading tc;
	QTEST_SET_MAIN_SOURCE_PATH
	return QTest::qExec(&tc, argc, argv);
}

#include "TestModuleLoading.moc"
