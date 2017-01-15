/**
 * @file
 * TestCompilingAndLinking interface and implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "TestCompositionExecution.hh"

#include <Vuo/Vuo.h>
#include <stdlib.h>

// Be able to use these types in QTest::addColumn()
Q_DECLARE_METATYPE(VuoCompiler::Optimization);


/**
 * Tests the compiling and linking of compositions.
 */
class TestCompilingAndLinking : public TestCompositionExecution
{
	Q_OBJECT

private:

	VuoCompiler *compiler;
	string cachedDylib_builtIn;
	string cachedDylib_user;
	string cachedIndex_builtIn;
	string cachedIndex_user;

private slots:

	void initTestCase()
	{
		compiler = initCompiler();

		string cacheDir = VuoFileUtilities::getCachePath();
		cachedDylib_builtIn = cacheDir + "/libVuoResources.dylib";
		cachedDylib_user = cacheDir + "/libVuoResources-user.dylib";
		cachedIndex_builtIn = cacheDir + "/index.txt";
		cachedIndex_user = cacheDir + "/index-user.txt";
	}

	void cleanupTestCase()
	{
		delete compiler;
	}

	void testCompilingWithoutCrashing_data()
	{
		QTest::addColumn< QString >("compositionName");

		QTest::newRow("Trigger node with no other nodes and no cables.") << "Recur";
		QTest::newRow("Trigger node with a cable to itself.") << "Recur_Count_loop";
		QTest::newRow("Trigger port carrying a VuoInteger.") << "TriggerCarryingInteger";
		QTest::newRow("Trigger port carrying a VuoReal.") << "TriggerCarryingReal";
		QTest::newRow("Trigger port carrying a 32-bit float.") << "TriggerCarryingFloat";
		QTest::newRow("Trigger port carrying a VuoPoint2d.") << "TriggerCarryingPoint2d";
		QTest::newRow("Trigger port carrying a VuoPoint3d.") << "TriggerCarryingPoint3d";
		QTest::newRow("Trigger port carrying a VuoPoint4d.") << "TriggerCarryingPoint4d";
		QTest::newRow("Trigger port carrying a VuoMidiNote.") << "TriggerCarryingMIDINote";
		QTest::newRow("Passive output port carrying a VuoMidiNote.") << "OutputCarryingMIDINote";
		QTest::newRow("Passive cable carrying a struct passed by value.") << "CableCarryingStructByValue";
		QTest::newRow("Passive cable carrying a struct coerced to a vector.") << "CableCarryingVuoPoint2d";
		QTest::newRow("Passive cable carrying a struct coerced to a struct containing a vector and a singleton.") << "CableCarryingVuoPoint3d";
		QTest::newRow("Passive cable carrying a struct coerced to a struct containing two vectors.") << "CableCarryingVuoPoint4d";
		QTest::newRow("Event-only cable from a data-and-event trigger port to a data-and-event input port.") << "EventCableFromDataTrigger";
		QTest::newRow("Event-only cable from a data-and-event output port to a data-and-event input port.") << "EventCableFromDataOutput";
		QTest::newRow("Node with an input port and output port whose types are pointers to structs.") << "StructPointerPorts";
		QTest::newRow("Make List node with 0 items.") << "AddNoTerms";
		QTest::newRow("Published input port called 'refresh'.") << "RefreshPublishedInput";
	}
	void testCompilingWithoutCrashing()
	{
		QFETCH(QString, compositionName);

		string compositionPath = getCompositionPath(compositionName.toStdString() + ".vuo");
		string bcPath = VuoFileUtilities::makeTmpFile(compositionName.toUtf8().constData(), "bc");
		compiler->compileComposition(compositionPath, bcPath);
		remove(bcPath.c_str());
	}

	void testLinkingWithoutCrashing_data()
	{
		QTest::addColumn< QString >("linkType");
		QTest::addColumn< VuoCompiler::Optimization >("optimization");

		QTest::newRow("Executable, small binary") << "EXECUTABLE" << VuoCompiler::Optimization_SmallBinary;
		QTest::newRow("Executable, fast build") << "EXECUTABLE" << VuoCompiler::Optimization_FastBuild;
		QTest::newRow("Combined dylib, small binary") << "COMBINED_DYLIB" << VuoCompiler::Optimization_SmallBinary;
		QTest::newRow("Combined dylib, fast build") << "COMBINED_DYLIB" << VuoCompiler::Optimization_FastBuild;
		QTest::newRow("Composition dylib and resource dylib") << "COMPOSITION_DYLIB_AND_RESOURCE_DYLIB" << VuoCompiler::Optimization_FastBuild;
	}
	void testLinkingWithoutCrashing()
	{
		QFETCH(QString, linkType);
		QFETCH(VuoCompiler::Optimization, optimization);

		string compositionPath = getCompositionPath("Recur.vuo");
		string dir, file, ext;
		VuoFileUtilities::splitPath(compositionPath, dir, file, ext);
		string compiledCompositionPath = VuoFileUtilities::makeTmpFile(file, "bc");
		string linkedCompositionPath = VuoFileUtilities::makeTmpFile(file, (linkType == "EXECUTABLE" ? "" : "dylib"));
		string linkedResourcePath = VuoFileUtilities::makeTmpFile(file + "-resource", "dylib");

		compiler->compileComposition(compositionPath, compiledCompositionPath);

		if (linkType == "EXECUTABLE")
		{
			compiler->linkCompositionToCreateExecutable(compiledCompositionPath, linkedCompositionPath, optimization);
		}
		else if (linkType == "COMBINED_DYLIB")
		{
			compiler->linkCompositionToCreateDynamicLibrary(compiledCompositionPath, linkedCompositionPath, optimization);
		}
		else if (linkType == "COMPOSITION_DYLIB_AND_RESOURCE_DYLIB")
		{
			vector<string> alreadyLinkedResourcePaths;
			set<string> alreadyLinkedResources;
			compiler->linkCompositionToCreateDynamicLibraries(compiledCompositionPath, linkedCompositionPath, linkedResourcePath,
															  alreadyLinkedResourcePaths, alreadyLinkedResources);
		}
		remove(compiledCompositionPath.c_str());

		QVERIFY2(access(linkedCompositionPath.c_str(), 0) == 0, ("Expected to find file " + linkedCompositionPath).c_str());
		remove(linkedCompositionPath.c_str());

		if (linkType == "COMPOSITION_DYLIB_AND_RESOURCE_DYLIB")
		{
			QVERIFY2(access(linkedResourcePath.c_str(), 0) == 0, ("Expected to find file " + linkedResourcePath).c_str());
			remove(linkedResourcePath.c_str());
		}
	}

	void testLinkingMultipleTimes()
	{
		string compositionPath = getCompositionPath("Recur_Count_Write.vuo");
		string dir, file, extension;
		VuoFileUtilities::splitPath(compositionPath, dir, file, extension);
		string bcPath = VuoFileUtilities::makeTmpFile(file, "bc");
		string exePath = VuoFileUtilities::makeTmpFile(file, "");

		remove(bcPath.c_str());
		remove(exePath.c_str());

		for (int i = 0; i < 2; ++i)
		{
			compiler->compileComposition(compositionPath, bcPath);
			compiler->linkCompositionToCreateExecutable(bcPath, exePath, VuoCompiler::Optimization_SmallBinary);
			ifstream file(exePath.c_str());
			QVERIFY2(file, qPrintable(QString("Failed to link on iteration %1").arg(i)));
			file.close();

			remove(bcPath.c_str());
			remove(exePath.c_str());
		}
	}

	void testCompilingAndLinkingWithGenericNodes_data()
	{
		QTest::addColumn< QString >("compositionName");

		QTest::newRow("Generic node specialized with the type that replaces VuoGenericType (VuoInteger)") << "Recur_Hold_Add_Write_loop";
		QTest::newRow("Generic node specialized with a type included by all node classes (VuoText)") << "StoreRecentText";
		QTest::newRow("Generic node specialized with a type defined within a node set (VuoBlendMode)") << "HoldBlendMode";
		QTest::newRow("Generic node specialized with a list type (VuoList_VuoBlendMode)") << "HoldListOfBlendModes";
		QTest::newRow("Generic node specialized with a list type (VuoList_VuoText)") << "HoldListOfTexts";
		QTest::newRow("Generic node specialized with 2 different types") << "ReceiveOscTextAndReal";
		QTest::newRow("Generic node, not specialized") << "HoldAnyType";
		QTest::newRow("Generic node, 1st of 2 types not specialized") << "ReceiveOscReal";
		QTest::newRow("Generic node, 2nd of 2 types not specialized") << "ReceiveOscText";
		QTest::newRow("More unique generic types in the composition than in any one node class") << "HoldAnyTypeX2";
		QTest::newRow("Generic 'Make List' node") << "AddAnyType";
		QTest::newRow("Generic node, not specialized, incompatible with the default backing type") << "AddPoints";
	}
	void testCompilingAndLinkingWithGenericNodes()
	{
		QFETCH(QString, compositionName);
		printf("	%s\n", compositionName.toUtf8().data()); fflush(stdout);

		string compositionPath = getCompositionPath(compositionName.toStdString() + ".vuo");
		string dir, file, ext;
		VuoFileUtilities::splitPath(compositionPath, dir, file, ext);
		string compiledCompositionPath = VuoFileUtilities::makeTmpFile(file, "bc");
		string linkedCompositionPath = VuoFileUtilities::makeTmpFile(file, "");

		compiler->compileComposition(compositionPath, compiledCompositionPath);
		compiler->linkCompositionToCreateExecutable(compiledCompositionPath, linkedCompositionPath, VuoCompiler::Optimization_SmallBinary);
		QVERIFY(VuoFileUtilities::fileExists(linkedCompositionPath));

		remove(compiledCompositionPath.c_str());
		remove(linkedCompositionPath.c_str());
	}

	void testCompilingPerformance_data()
	{
		QTest::addColumn< QString >("compositionName");

		QTest::newRow("Empty composition") << "Empty";
		QTest::newRow("50 nodes of the same non-generic node class, in a line from 1 trigger") << "LineOfSameNodeClass";
		QTest::newRow("50 nodes of different non-generic node classes, in a line from 1 trigger") << "LineOfDiffNodeClass";
		QTest::newRow("50 nodes of the same generic node class specialized to the same type, in a line from 1 trigger") << "LineOfSameNodeClassSameType";
		QTest::newRow("50 nodes of the same generic node class specialized to different types, in a line from 1 trigger") << "LineOfSameNodeClassDiffType";
		QTest::newRow("50 nodes-with-drawers of the same generic node class specialized to different types, in a line from 1 trigger") << "LineOfDrawersOfDiffType";
		QTest::newRow("50 nodes-with-triggers of the same non-generic node class, not connected") << "UnconnectedTriggers";
		QTest::newRow("50 nodes of the same non-generic node class, in a diamond series from 1 trigger") << "DiamondOfSameNodeClass";
		QTest::newRow("50 nodes of the same non-generic node class, in a line from 50 triggers") << "LineOfSameNodeClassMultiTriggers";
		QTest::newRow("50 nodes of the same non-generic node class, each from one of 50 triggers") << "EachPairOfSameNodeClassMultiTriggers";
		QTest::newRow("100 nodes of the same non-generic node class, in a line from 1 trigger") << "LineOfSameNodeClass100";
		QTest::newRow("200 nodes of the same non-generic node class, in a line from 1 trigger") << "LineOfSameNodeClass200";
		QTest::newRow("400 nodes of the same non-generic node class, in a line from 1 trigger") << "LineOfSameNodeClass400";
	}
	void testCompilingPerformance()
	{
		QFETCH(QString, compositionName);
		printf("	%s\n", compositionName.toUtf8().data()); fflush(stdout);

		string compositionPath = getCompositionPath(compositionName.toStdString() + ".vuo");
		string dir, file, ext;
		VuoFileUtilities::splitPath(compositionPath, dir, file, ext);
		string compiledCompositionPath = VuoFileUtilities::makeTmpFile(file, "bc");

		QBENCHMARK {
			compiler->compileComposition(compositionPath, compiledCompositionPath);

			QVERIFY(VuoFileUtilities::fileExists(compiledCompositionPath));
			remove(compiledCompositionPath.c_str());
		}
	}

	void testCompilingAndLinkingPerformance_data()
	{
		QTest::addColumn< QString >("compositionName");

		QTest::newRow("Empty composition") << "Empty";
		QTest::newRow("50 nodes of the same non-generic node class, in a line from 1 trigger") << "LineOfSameNodeClass";
		QTest::newRow("50 nodes of different non-generic node classes, in a line from 1 trigger") << "LineOfDiffNodeClass";
		QTest::newRow("50 nodes of the same generic node class specialized to the same type, in a line from 1 trigger") << "LineOfSameNodeClassSameType";
		QTest::newRow("50 nodes of the same generic node class specialized to different types, in a line from 1 trigger") << "LineOfSameNodeClassDiffType";
		QTest::newRow("50 nodes-with-drawers of the same generic node class specialized to different types, in a line from 1 trigger") << "LineOfDrawersOfDiffType";
		QTest::newRow("50 nodes-with-triggers of the same non-generic node class, not connected") << "UnconnectedTriggers";
		QTest::newRow("50 nodes of the same non-generic node class, in a diamond series from 1 trigger") << "DiamondOfSameNodeClass";
		QTest::newRow("50 nodes of the same non-generic node class, in a line from 50 triggers") << "LineOfSameNodeClassMultiTriggers";
		QTest::newRow("50 nodes of the same non-generic node class, each from one of 50 triggers") << "EachPairOfSameNodeClassMultiTriggers";
		QTest::newRow("100 nodes of the same non-generic node class, in a line from 1 trigger") << "LineOfSameNodeClass100";
		QTest::newRow("200 nodes of the same non-generic node class, in a line from 1 trigger") << "LineOfSameNodeClass200";
		QTest::newRow("400 nodes of the same non-generic node class, in a line from 1 trigger") << "LineOfSameNodeClass400";
	}
	void testCompilingAndLinkingPerformance()
	{
		QFETCH(QString, compositionName);
		printf("	%s\n", compositionName.toUtf8().data()); fflush(stdout);

		string compositionPath = getCompositionPath(compositionName.toStdString() + ".vuo");
		string dir, file, ext;
		VuoFileUtilities::splitPath(compositionPath, dir, file, ext);
		string compiledCompositionPath = VuoFileUtilities::makeTmpFile(file, "bc");
		string linkedCompositionPath = VuoFileUtilities::makeTmpFile(file, "dylib");
		string linkedResourcePath = VuoFileUtilities::makeTmpFile(file + "-resource", "dylib");

		// Simulate the most common case: the cache already exists when the process starts.
		compiler->getCachedResources();
		compiler->hasTriedCachedResources[0] = false;
		compiler->hasTriedCachedResources[1] = false;
		compiler->getCachedResources();

		QBENCHMARK {
			compiler->compileComposition(compositionPath, compiledCompositionPath);

			vector<string> alreadyLinkedResourcePaths;
			set<string> alreadyLinkedResources;
			compiler->linkCompositionToCreateDynamicLibraries(compiledCompositionPath, linkedCompositionPath, linkedResourcePath,
															  alreadyLinkedResourcePaths, alreadyLinkedResources);

			QVERIFY(VuoFileUtilities::fileExists(linkedCompositionPath));
			remove(compiledCompositionPath.c_str());
			remove(linkedCompositionPath.c_str());
			remove(linkedResourcePath.c_str());
		}
	}

	void testLiveCodingPerformance_data()
	{
		QTest::addColumn< QString >("compositionName");

		QTest::newRow("Empty composition") << "Empty";
		QTest::newRow("50 nodes of the same non-generic node class, in a line from 1 trigger") << "LineOfSameNodeClass";
		QTest::newRow("50 nodes of different non-generic node classes, in a line from 1 trigger") << "LineOfDiffNodeClass";
		QTest::newRow("50 nodes of the same generic node class specialized to the same type, in a line from 1 trigger") << "LineOfSameNodeClassSameType";
		QTest::newRow("50 nodes of the same generic node class specialized to different types, in a line from 1 trigger") << "LineOfSameNodeClassDiffType";
		QTest::newRow("50 nodes-with-drawers of the same generic node class specialized to different types, in a line from 1 trigger") << "LineOfDrawersOfDiffType";
		QTest::newRow("50 nodes-with-triggers of the same non-generic node class, not connected") << "UnconnectedTriggers";
		QTest::newRow("50 nodes of the same non-generic node class, in a diamond series from 1 trigger") << "DiamondOfSameNodeClass";
		QTest::newRow("50 nodes of the same non-generic node class, in a line from 50 triggers") << "LineOfSameNodeClassMultiTriggers";
		QTest::newRow("50 nodes of the same non-generic node class, each from one of 50 triggers") << "EachPairOfSameNodeClassMultiTriggers";
		QTest::newRow("100 nodes of the same non-generic node class, in a line from 1 trigger") << "LineOfSameNodeClass100";
		QTest::newRow("200 nodes of the same non-generic node class, in a line from 1 trigger") << "LineOfSameNodeClass200";
		QTest::newRow("400 nodes of the same non-generic node class, in a line from 1 trigger") << "LineOfSameNodeClass400";
	}
	void testLiveCodingPerformance()
	{
		QFETCH(QString, compositionName);
		printf("	%s\n", compositionName.toUtf8().data()); fflush(stdout);

		string compositionPath = getCompositionPath(compositionName.toStdString() + ".vuo");
		string dir, file, ext;
		VuoFileUtilities::splitPath(compositionPath, dir, file, ext);
		string compiledCompositionPath = VuoFileUtilities::makeTmpFile(file, "bc");
		string linkedCompositionPath = VuoFileUtilities::makeTmpFile(file, "dylib");

		// Simulate the most common case: the cache already exists when the process starts.
		compiler->getCachedResources();
		compiler->hasTriedCachedResources[0] = false;
		compiler->hasTriedCachedResources[1] = false;
		compiler->getCachedResources();

		string compositionString = VuoFileUtilities::readFileToString(compositionPath);
		VuoCompilerComposition *composition = VuoCompilerComposition::newCompositionFromGraphvizDeclaration(compositionString, compiler);
		compiler->compileComposition(composition, compiledCompositionPath);
		vector<string> alreadyLinkedResourcePaths;
		set<string> alreadyLinkedResources;
		string linkedResourcePath = VuoFileUtilities::makeTmpFile(file, "dylib");
		compiler->linkCompositionToCreateDynamicLibraries(compiledCompositionPath, linkedCompositionPath, linkedResourcePath,
														  alreadyLinkedResourcePaths, alreadyLinkedResources);
		QVERIFY(VuoFileUtilities::fileExists(linkedCompositionPath));
		remove(compiledCompositionPath.c_str());
		remove(linkedCompositionPath.c_str());

		VuoCompilerNodeClass *fireOnStartNodeClass = compiler->getNodeClass("vuo.event.fireOnStart");
		VuoNode *fireOnStartNode = fireOnStartNodeClass->newNode("FireOnStartTest");
		composition->getBase()->addNode(fireOnStartNode);
		string oldCompositionString = compositionString;
		compositionString = composition->getGraphvizDeclaration();

		QBENCHMARK {
			// Simulate the VuoCompiler-related parts of VuoEditorComposition::updateRunningComposition().

			composition = VuoCompilerComposition::newCompositionFromGraphvizDeclaration(compositionString, compiler);

			compiler->compileComposition(composition, compiledCompositionPath);

			linkedResourcePath = VuoFileUtilities::makeTmpFile(file, "dylib");
			compiler->linkCompositionToCreateDynamicLibraries(compiledCompositionPath, linkedCompositionPath, linkedResourcePath,
															  alreadyLinkedResourcePaths, alreadyLinkedResources);
			QVERIFY(VuoFileUtilities::fileExists(linkedCompositionPath));
			remove(compiledCompositionPath.c_str());
			remove(linkedCompositionPath.c_str());

			composition->diffAgainstOlderComposition(oldCompositionString, compiler, set<VuoCompilerComposition::NodeReplacement>());
		}

		for (vector<string>::iterator i = alreadyLinkedResourcePaths.begin(); i != alreadyLinkedResourcePaths.end(); ++i)
			remove((*i).c_str());
	}

	void testCompilingAndLinkingInSeparateProcessPerformance_data()
	{
		QTest::addColumn< QString >("compositionName");

		QTest::newRow("Empty composition") << "Empty";
		QTest::newRow("50 nodes of the same non-generic node class, in a line from 1 trigger") << "LineOfSameNodeClass";
		QTest::newRow("50 nodes of different non-generic node classes, in a line from 1 trigger") << "LineOfDiffNodeClass";
		QTest::newRow("50 nodes of the same generic node class specialized to the same type, in a line from 1 trigger") << "LineOfSameNodeClassSameType";
		QTest::newRow("50 nodes of the same generic node class specialized to different types, in a line from 1 trigger") << "LineOfSameNodeClassDiffType";
		QTest::newRow("50 nodes-with-drawers of the same generic node class specialized to different types, in a line from 1 trigger") << "LineOfDrawersOfDiffType";
		QTest::newRow("50 nodes-with-triggers of the same non-generic node class, not connected") << "UnconnectedTriggers";
		QTest::newRow("50 nodes of the same non-generic node class, in a diamond series from 1 trigger") << "DiamondOfSameNodeClass";
		QTest::newRow("50 nodes of the same non-generic node class, in a line from 50 triggers") << "LineOfSameNodeClassMultiTriggers";
		QTest::newRow("50 nodes of the same non-generic node class, each from one of 50 triggers") << "EachPairOfSameNodeClassMultiTriggers";
		QTest::newRow("100 nodes of the same non-generic node class, in a line from 1 trigger") << "LineOfSameNodeClass100";
		QTest::newRow("200 nodes of the same non-generic node class, in a line from 1 trigger") << "LineOfSameNodeClass200";
		QTest::newRow("400 nodes of the same non-generic node class, in a line from 1 trigger") << "LineOfSameNodeClass400";
	}
	void testCompilingAndLinkingInSeparateProcessPerformance()
	{
		QFETCH(QString, compositionName);
		printf("	%s\n", compositionName.toUtf8().data()); fflush(stdout);

		string compositionPath = getCompositionPath(compositionName.toStdString() + ".vuo");

		compiler->getCachedResources();

		QBENCHMARK {
			VuoRunner *runner = VuoRunner::newSeparateProcessRunnerFromCompositionFile(compositionPath, false, true);
			QVERIFY(runner);
			delete runner;
		}
	}

private:

	void doesCacheGetRecreated(bool &builtIn, bool &user)
	{
		unsigned long cachedDylibLastModified_builtIn = (VuoFileUtilities::fileExists(cachedDylib_builtIn) ?
															 VuoFileUtilities::getFileLastModifiedInSeconds(cachedDylib_builtIn) : 0);
		unsigned long cachedIndexLastModified_builtIn = (VuoFileUtilities::fileExists(cachedIndex_builtIn) ?
															 VuoFileUtilities::getFileLastModifiedInSeconds(cachedIndex_builtIn) : 0);
		unsigned long cachedDylibLastModified_user = (VuoFileUtilities::fileExists(cachedDylib_user) ?
														  VuoFileUtilities::getFileLastModifiedInSeconds(cachedDylib_user) : 0);
		unsigned long cachedIndexLastModified_user = (VuoFileUtilities::fileExists(cachedIndex_user) ?
														  VuoFileUtilities::getFileLastModifiedInSeconds(cachedIndex_user) : 0);

		VuoCompiler::hasTriedCachedResources[0] = false;
		VuoCompiler::hasTriedCachedResources[1] = false;
		delete VuoCompiler::sharedEnvironment;
		VuoCompiler::sharedEnvironment = NULL;
		VuoCompiler c;  // Not created with TestCompositionExecution::initCompiler() — don't want extra module folders.
		c.getCachedResources();

		builtIn = (VuoFileUtilities::fileExists(cachedDylib_builtIn) &&
				   VuoFileUtilities::fileExists(cachedIndex_builtIn) &&
				   VuoFileUtilities::getFileLastModifiedInSeconds(cachedDylib_builtIn) > cachedDylibLastModified_builtIn &&
				   VuoFileUtilities::getFileLastModifiedInSeconds(cachedIndex_builtIn) > cachedIndexLastModified_builtIn);
		user = (VuoFileUtilities::fileExists(cachedDylib_user) &&
				   VuoFileUtilities::fileExists(cachedIndex_user) &&
				   VuoFileUtilities::getFileLastModifiedInSeconds(cachedDylib_user) > cachedDylibLastModified_user &&
				   VuoFileUtilities::getFileLastModifiedInSeconds(cachedIndex_user) > cachedIndexLastModified_user);
	}

private slots:

	void testCreationOfCachedResources()
	{
		string nodeClass = "vuo.test.doNothing.vuonode";
		string nodeDir = QDir::current().absolutePath().toStdString() + "/node-TestCompilingAndLinking";
		string moduleDir = VuoFileUtilities::getUserModulesPath();
		string nodeClassInModuleDir = moduleDir + "/" + nodeClass;

		remove(cachedDylib_builtIn.c_str());
		remove(cachedDylib_user.c_str());
		remove(cachedIndex_builtIn.c_str());
		remove(cachedIndex_user.c_str());
		remove(nodeClassInModuleDir.c_str());

		bool recreated_builtIn = false;
		bool recreated_user = false;

		// Cached files do not exist.
		doesCacheGetRecreated(recreated_builtIn, recreated_user);
		QVERIFY(recreated_builtIn);
		QVERIFY(recreated_user);

		// Cached files are up to date.
		doesCacheGetRecreated(recreated_builtIn, recreated_user);
		QVERIFY(! recreated_builtIn);
		QVERIFY(! recreated_user);

		// Cached dylib for built-in modules does not exist.
		remove(cachedDylib_builtIn.c_str());
		doesCacheGetRecreated(recreated_builtIn, recreated_user);
		QVERIFY(recreated_builtIn);
		QVERIFY(! recreated_user);

		// Cached dylib for user modules does not exist.
		remove(cachedDylib_user.c_str());
		doesCacheGetRecreated(recreated_builtIn, recreated_user);
		QVERIFY(! recreated_builtIn);
		QVERIFY(recreated_user);

		// Node class added to Modules folder.
		string escapedNodeClassInNodeDir = "\"" + nodeDir + "/" + nodeClass + "\"";
		string escapedModuleDir = "\"" + moduleDir + "\"";
		system(("/bin/cp " + escapedNodeClassInNodeDir + " " + escapedModuleDir).c_str());
		doesCacheGetRecreated(recreated_builtIn, recreated_user);
		QVERIFY(! recreated_builtIn);
		QVERIFY(recreated_user);

		// Node class updated in Modules folder.
		sleep(2);  // Make sure getFileLastModifiedInSeconds() is larger for the node class than the cache files.
		string escapedNodeClassInModuleDir = "\"" + nodeClassInModuleDir + "\"";
		system(("/usr/bin/touch " + escapedNodeClassInModuleDir).c_str());
		doesCacheGetRecreated(recreated_builtIn, recreated_user);
		QVERIFY(! recreated_builtIn);
		QVERIFY(recreated_user);

		// Node class deleted from Modules folder.
		remove(nodeClassInModuleDir.c_str());
		doesCacheGetRecreated(recreated_builtIn, recreated_user);
		QVERIFY(! recreated_builtIn);
		QVERIFY(recreated_user);
	}

	void testFallbackWithoutCache()
	{
		VuoCompiler compiler2;

		// Simulate the compiler's having tried and failed to create the cache.
		compiler2.hasTriedCachedResources[0] = true;
		compiler2.hasTriedCachedResources[1] = true;

		string compositionPath = getCompositionPath("Recur.vuo");
		string dir, file, ext;
		VuoFileUtilities::splitPath(compositionPath, dir, file, ext);
		string compiledCompositionPath = VuoFileUtilities::makeTmpFile(file, "bc");
		string linkedCompositionPath = VuoFileUtilities::makeTmpFile(file, "");

		compiler2.compileComposition(compositionPath, compiledCompositionPath);
		compiler2.linkCompositionToCreateExecutable(compiledCompositionPath, linkedCompositionPath, VuoCompiler::Optimization_FastBuild);
		remove(compiledCompositionPath.c_str());

		QVERIFY2(access(linkedCompositionPath.c_str(), 0) == 0, ("Expected to find file " + linkedCompositionPath).c_str());
		remove(linkedCompositionPath.c_str());
	}

	void testCacheCreationPerformance()
	{
		VuoCompiler compiler2;

		// Delete the cache to force it to be recreated.
		remove(cachedDylib_builtIn.c_str());
		remove(cachedDylib_user.c_str());
		remove(cachedIndex_builtIn.c_str());
		remove(cachedIndex_user.c_str());

		QBENCHMARK {
			bool recreated_builtIn = false;
			bool recreated_user = false;
			doesCacheGetRecreated(recreated_builtIn, recreated_user);
			QVERIFY(recreated_builtIn);
			QVERIFY(recreated_user);
		}
	}

	void testCacheCheckingPerformance()
	{
		VuoCompiler compiler2;

		// Make sure the cache exists, but force it to be rechecked.
		compiler2.getCachedResources();

		QBENCHMARK {
			bool recreated_builtIn = false;
			bool recreated_user = false;
			doesCacheGetRecreated(recreated_builtIn, recreated_user);
			QVERIFY(! recreated_builtIn);
			QVERIFY(! recreated_user);
		}
	}
};


QTEST_APPLESS_MAIN(TestCompilingAndLinking)
#include "TestCompilingAndLinking.moc"
