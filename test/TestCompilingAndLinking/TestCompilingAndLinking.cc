/**
 * @file
 * TestCompilingAndLinking interface and implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
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
	string cacheDylib_user;
	string cacheIndex_user;
	string cacheDylib_composition;
	string cacheIndex_composition;

private slots:

	void initTestCase()
	{
		cacheDylib_user = VuoFileUtilities::getCachePath() + "/User/libVuoModuleCache-installed.dylib";
		cacheIndex_user = VuoFileUtilities::getCachePath() + "/User/moduleCache-installed.txt";
		cacheDylib_composition = VuoCompiler::getCachePathForComposition(VuoFileUtilities::getTmpDir() + "/TestCompilingAndLinking") + "/libVuoModuleCache-installed.dylib";
		cacheIndex_composition = VuoCompiler::getCachePathForComposition(VuoFileUtilities::getTmpDir() + "/TestCompilingAndLinking") + "/moduleCache-installed.txt";
	}

	void init()
	{
		compiler = initCompiler();
	}

	void cleanup()
	{
		delete compiler;
		VuoCompiler::reset();
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
		QTest::newRow("Missing node class.") << "WrongNodeClassName";
	}
	void testCompilingWithoutCrashing()
	{
		QFETCH(QString, compositionName);

		string compositionPath = getCompositionPath(compositionName.toStdString() + ".vuo");
		string bcPath = VuoFileUtilities::makeTmpFile(compositionName.toUtf8().constData(), "bc");
		VuoCompilerIssues *issues = new VuoCompilerIssues();
		try {
			compiler->compileComposition(compositionPath, bcPath, true, issues);
		} catch (...) {}
		delete issues;

		// Also test compiling without populating VuoCompilerIssues.
		try {
			compiler->compileComposition(compositionPath, bcPath, true, nullptr);
		} catch (...) {}

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

		VuoCompilerIssues *issues = new VuoCompilerIssues();
		compiler->compileComposition(compositionPath, compiledCompositionPath, true, issues);
		delete issues;

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
			VuoRunningCompositionLibraries runningCompositionLibraries;
			runningCompositionLibraries.setDeleteResourceLibraries(true);
			compiler->linkCompositionToCreateDynamicLibraries(compiledCompositionPath, linkedCompositionPath, &runningCompositionLibraries);
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
			VuoCompilerIssues *issues = new VuoCompilerIssues();
			compiler->compileComposition(compositionPath, bcPath, true, issues);
			compiler->linkCompositionToCreateExecutable(bcPath, exePath, VuoCompiler::Optimization_SmallBinary);
			delete issues;
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

		VuoCompilerIssues *issues = new VuoCompilerIssues();
		compiler->compileComposition(compositionPath, compiledCompositionPath, true, issues);
		delete issues;
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

		// Time 2 iterations with the same modules loaded (no VuoCompiler::reset() call in between).
		// 1st iteration should load the modules, 2nd should reuse them.
		struct timeval t1 = VuoTimeUtilities::getCurrentTime();
		for (int iteration = 0; iteration < 2; ++iteration)
		{
			VuoCompilerIssues *issues = new VuoCompilerIssues();
			compiler->compileComposition(compositionPath, compiledCompositionPath, true, issues);
			delete issues;

			QVERIFY(VuoFileUtilities::fileExists(compiledCompositionPath));
			remove(compiledCompositionPath.c_str());
		}
		struct timeval t2 = VuoTimeUtilities::getCurrentTime();
		struct timeval elapsed = VuoTimeUtilities::getElapsedTime(t1, t2);
		QTest::setBenchmarkResult(elapsed.tv_sec*1000 + elapsed.tv_usec/1000, QTest::WalltimeMilliseconds);
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

		// Simulate the most common case: the cache already exists when the process starts.
		compiler->useModuleCache(true, false);

		// Time 2 iterations with the same modules loaded (no VuoCompiler::reset() call in between).
		// 1st iteration should load the modules, 2nd should reuse them.
		struct timeval t1 = VuoTimeUtilities::getCurrentTime();
		for (int iteration = 0; iteration < 2; ++iteration)
		{
			VuoCompilerIssues *issues = new VuoCompilerIssues();
			compiler->compileComposition(compositionPath, compiledCompositionPath, true, issues);
			delete issues;

			VuoRunningCompositionLibraries runningCompositionLibraries;
			compiler->linkCompositionToCreateDynamicLibraries(compiledCompositionPath, linkedCompositionPath, &runningCompositionLibraries);

			QVERIFY(VuoFileUtilities::fileExists(linkedCompositionPath));
			remove(compiledCompositionPath.c_str());
			remove(linkedCompositionPath.c_str());
		}
		struct timeval t2 = VuoTimeUtilities::getCurrentTime();
		struct timeval elapsed = VuoTimeUtilities::getElapsedTime(t1, t2);
		QTest::setBenchmarkResult(elapsed.tv_sec*1000 + elapsed.tv_usec/1000, QTest::WalltimeMilliseconds);
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
		compiler->useModuleCache(true, false);

		string compositionString = VuoFileUtilities::readFileToString(compositionPath);
		VuoCompilerComposition *composition = VuoCompilerComposition::newCompositionFromGraphvizDeclaration(compositionString, compiler);
		VuoCompilerIssues *issues = new VuoCompilerIssues();
		compiler->compileComposition(composition, compiledCompositionPath, true, issues);
		delete issues;
		VuoRunningCompositionLibraries runningCompositionLibraries;
		runningCompositionLibraries.setDeleteResourceLibraries(true);
		compiler->linkCompositionToCreateDynamicLibraries(compiledCompositionPath, linkedCompositionPath, &runningCompositionLibraries);
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

			VuoCompilerIssues *issues = new VuoCompilerIssues();
			compiler->compileComposition(composition, compiledCompositionPath, true, issues);
			delete issues;

			compiler->linkCompositionToCreateDynamicLibraries(compiledCompositionPath, linkedCompositionPath, &runningCompositionLibraries);
			QVERIFY(VuoFileUtilities::fileExists(linkedCompositionPath));
			remove(compiledCompositionPath.c_str());
			remove(linkedCompositionPath.c_str());

			VuoCompilerCompositionDiff *diffInfo = new VuoCompilerCompositionDiff();
			diffInfo->diff(oldCompositionString, composition, compiler);
			delete diffInfo;
		}
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

		compiler->useModuleCache(true, false);

		QBENCHMARK {
			VuoCompilerIssues *issues = new VuoCompilerIssues();
			VuoRunner *runner = VuoCompiler::newSeparateProcessRunnerFromCompositionFile(compositionPath, issues);
			QVERIFY(runner);
			delete issues;
			delete runner;
		}
	}

	void testCheckForEventFlowErrorsPerformance_data()
	{
		QTest::addColumn<QString>("compositionName");

		QTest::newRow("50 nodes in a line from 1 trigger") << "LineOfSameNodeClass";
		QTest::newRow("50 nodes in a diamond series from 1 trigger") << "DiamondOfSameNodeClass";
		QTest::newRow("50 nodes in a line from 50 triggers") << "LineOfSameNodeClassMultiTriggers";
		QTest::newRow("50 nodes each from one of 50 triggers") << "EachPairOfSameNodeClassMultiTriggers";
		QTest::newRow("100 nodes in a line from 1 trigger") << "LineOfSameNodeClass100";
		QTest::newRow("200 nodes in a line from 1 trigger") << "LineOfSameNodeClass200";
		QTest::newRow("400 nodes in a line from 1 trigger") << "LineOfSameNodeClass400";
		QTest::newRow("50 nodes passing through from published input to output") << "PublishedPassthrough50";
	}
	void testCheckForEventFlowErrorsPerformance()
	{
		QFETCH(QString, compositionName);

		string compositionPath = getCompositionPath(compositionName.toStdString() + ".vuo");
		VuoCompilerGraphvizParser *parser = VuoCompilerGraphvizParser::newParserFromCompositionFile(compositionPath, compiler);
		VuoCompilerComposition composition(new VuoComposition(), parser);
		delete parser;

		QBENCHMARK {
			// Force the VuoCompilerGraph to be regenerated. Because of the way that QBENCHMARK repeats the test,
			// it would sometimes use the cache and sometimes regenerate, making the results difficult to interpret.
			composition.graphHash = 0;

			VuoCompilerIssues *issues = new VuoCompilerIssues();
			composition.checkForEventFlowIssues(issues);
			delete issues;
		}
	}

private:

	void doesCacheGetRecreated(bool shouldResetCompiler, bool &user, bool &composition)
	{
		unsigned long cachedDylibLastModified_user = (VuoFileUtilities::fileExists(cacheDylib_user) ?
														  VuoFileUtilities::getFileLastModifiedInSeconds(cacheDylib_user) : 0);
		unsigned long cachedIndexLastModified_user = (VuoFileUtilities::fileExists(cacheIndex_user) ?
														  VuoFileUtilities::getFileLastModifiedInSeconds(cacheIndex_user) : 0);
		unsigned long cachedDylibLastModified_composition = (VuoFileUtilities::fileExists(cacheDylib_composition) ?
																 VuoFileUtilities::getFileLastModifiedInSeconds(cacheDylib_composition) : 0);
		unsigned long cachedIndexLastModified_composition = (VuoFileUtilities::fileExists(cacheIndex_composition) ?
																 VuoFileUtilities::getFileLastModifiedInSeconds(cacheIndex_composition) : 0);

		sleep(1);  // Make sure that, if the cache gets rebuilt, the file times returned below will be greater than those returned above.

		if (shouldResetCompiler)
			compiler = new VuoCompiler(VuoFileUtilities::getTmpDir() + "/TestCompilingAndLinking/unused");  // Not initCompiler() — don't want module search path added, since we'll be installing those modules in the test.

		compiler->useModuleCache(true, false);

		if (shouldResetCompiler)
		{
			delete compiler;
			compiler = NULL;
			VuoCompiler::reset();
		}

		user = (VuoFileUtilities::fileExists(cacheDylib_user) &&
				VuoFileUtilities::fileExists(cacheIndex_user) &&
				VuoFileUtilities::getFileLastModifiedInSeconds(cacheDylib_user) > cachedDylibLastModified_user &&
				VuoFileUtilities::getFileLastModifiedInSeconds(cacheIndex_user) > cachedIndexLastModified_user);
		composition = (VuoFileUtilities::fileExists(cacheDylib_composition) &&
					   VuoFileUtilities::fileExists(cacheIndex_composition) &&
					   VuoFileUtilities::getFileLastModifiedInSeconds(cacheDylib_composition) > cachedDylibLastModified_composition &&
					   VuoFileUtilities::getFileLastModifiedInSeconds(cacheIndex_composition) > cachedIndexLastModified_composition);
	}

private slots:

	void testCreationOfCachedResourcesInitially()
	{
		string nodeDir               = string(BINARY_DIR) + "/test/TestCompilingAndLinking";
		string compositionDir = VuoFileUtilities::getTmpDir() + "/TestCompilingAndLinking";
		string compositionModulesDir = compositionDir + "/Modules";
		VuoFileUtilities::makeDir(compositionModulesDir);

		string controlNodeClass = "vuo.test.triggerCarryingReal.vuonode";  // A module needs to be installed for caches to be created.
		VuoFileUtilities::copyFile(nodeDir + "/" + controlNodeClass, VuoFileUtilities::getUserModulesPath() + "/" + controlNodeClass);
		VuoFileUtilities::copyFile(nodeDir + "/" + controlNodeClass, compositionModulesDir + "/" + controlNodeClass);

		string nodeClass = "vuo.test.doNothing.vuonode";
		string nodeClassInNodeDir = nodeDir + "/" + nodeClass;
		string nodeClassInCompositionModulesDir = compositionModulesDir + "/" + nodeClass;

		VuoFileUtilities::deleteFile(cacheDylib_user);
		VuoFileUtilities::deleteFile(cacheIndex_user);
		VuoFileUtilities::deleteFile(cacheDylib_composition);
		VuoFileUtilities::deleteFile(cacheIndex_composition);

		delete compiler;
		compiler = NULL;
		VuoCompiler::reset();

		bool recreated_user = false;
		bool recreated_composition = false;

		// Cache files do not exist.
		doesCacheGetRecreated(true, recreated_user, recreated_composition);
		QVERIFY(recreated_user);
		QVERIFY(recreated_composition);

		// Cache files are up to date.
		doesCacheGetRecreated(true, recreated_user, recreated_composition);
		QVERIFY(! recreated_user);
		QVERIFY(! recreated_composition);

		// Cache dylib for user modules does not exist.
		VuoFileUtilities::deleteFile(cacheDylib_user);
		doesCacheGetRecreated(true, recreated_user, recreated_composition);
		QVERIFY(recreated_user);
		QVERIFY(! recreated_composition);

		// Cache dylib for composition modules does not exist.
		VuoFileUtilities::deleteFile(cacheDylib_composition);
		doesCacheGetRecreated(true, recreated_user, recreated_composition);
		QVERIFY(! recreated_user);
		QVERIFY(recreated_composition);

		// Node class added to composition modules folder.
		VuoFileUtilities::copyFile(nodeClassInNodeDir, nodeClassInCompositionModulesDir);
		doesCacheGetRecreated(true, recreated_user, recreated_composition);
		QVERIFY(! recreated_user);
		QVERIFY(recreated_composition);

		// Node class updated in composition modules folder.
		string escapedNodeClassInModuleDir = "\"" + nodeClassInCompositionModulesDir + "\"";
		system(("/usr/bin/touch " + escapedNodeClassInModuleDir).c_str());
		doesCacheGetRecreated(true, recreated_user, recreated_composition);
		QVERIFY(! recreated_user);
		QVERIFY(recreated_composition);

		// Node class deleted from composition modules folder.
		VuoFileUtilities::deleteFile(nodeClassInCompositionModulesDir);
		doesCacheGetRecreated(true, recreated_user, recreated_composition);
		QVERIFY(! recreated_user);
		QVERIFY(recreated_composition);

		VuoFileUtilities::deleteDir(compositionDir);
		VuoFileUtilities::deleteFile(VuoFileUtilities::getUserModulesPath() + "/" + controlNodeClass);
	}

	void testRecreationOfCachedResources()
	{
		string nodeDir               = string(BINARY_DIR) + "/test/TestCompilingAndLinking";
		string compositionDir        = VuoFileUtilities::getTmpDir() + "/TestCompilingAndLinking";
		string compositionModulesDir = compositionDir + "/Modules";
		VuoFileUtilities::makeDir(compositionModulesDir);

		string controlNodeClass = "vuo.test.triggerCarryingReal.vuonode";  // A module needs to be installed for caches to be created.
		VuoFileUtilities::copyFile(nodeDir + "/" + controlNodeClass, VuoFileUtilities::getUserModulesPath() + "/" + controlNodeClass);
		VuoFileUtilities::copyFile(nodeDir + "/" + controlNodeClass, compositionModulesDir + "/" + controlNodeClass);

		string nodeClass = "vuo.test.doNothing.vuonode";
		string nodeClassInNodeDir = nodeDir + "/" + nodeClass;
		string nodeClassInCompositionModulesDir = compositionModulesDir + "/" + nodeClass;

		delete compiler;
		VuoCompiler::reset();
		compiler = new VuoCompiler(compositionDir + "/unused");
		TestCompilerDelegate delegate;
		compiler->setDelegate(&delegate);

		bool recreated_user = false;
		bool recreated_composition = false;

		// Create caches initially, if needed.
		doesCacheGetRecreated(false, recreated_user, recreated_composition);
		sleep(2);

		// Node class added to composition modules folder.
		delegate.installModule(nodeClassInNodeDir, nodeClassInCompositionModulesDir);
		doesCacheGetRecreated(false, recreated_user, recreated_composition);
		QVERIFY(! recreated_user);
		QVERIFY(recreated_composition);

		// Node class updated in composition modules folder.
		delegate.installModule(nodeClassInNodeDir, nodeClassInCompositionModulesDir);
		doesCacheGetRecreated(false, recreated_user, recreated_composition);
		QVERIFY(! recreated_user);
		QVERIFY(recreated_composition);

		// Node class deleted from composition modules folder.
		delegate.uninstallModule(nodeClassInCompositionModulesDir);
		doesCacheGetRecreated(false, recreated_user, recreated_composition);
		QVERIFY(! recreated_user);
		QVERIFY(recreated_composition);

		VuoFileUtilities::deleteDir(compositionDir);
		VuoFileUtilities::deleteFile(VuoFileUtilities::getUserModulesPath() + "/" + controlNodeClass);
	}

	void testFallbackWithoutCache()
	{
		TestCompilerDelegate delegate;
		compiler->setDelegate(&delegate);

		string nodeClass = "vuo.test.triggerCarryingReal.vuonode";
		string nodeDir   = string(BINARY_DIR) + "/test/TestCompilingAndLinking";
		delegate.installModule(nodeDir + "/" + nodeClass, VuoFileUtilities::getUserModulesPath() + "/" + nodeClass);

		// Simulate the compiler's having tried and failed to create the cache.
		compiler->getNodeClasses();
		for (vector< vector<VuoCompiler::Environment *> >::iterator i = compiler->environments.begin(); i != compiler->environments.end(); ++i)
		{
			for (vector<VuoCompiler::Environment *>::iterator j = i->begin(); j != i->end(); ++j)
			{
				(*j)->isModuleCacheInitialized = true;
				(*j)->isModuleCacheableDataDirty = false;
			}
		}

		VuoFileUtilities::deleteFile(cacheDylib_user);

		string compositionPath = getCompositionPath("TriggerCarryingReal.vuo");
		string dir, file, ext;
		VuoFileUtilities::splitPath(compositionPath, dir, file, ext);
		string compiledCompositionPath = VuoFileUtilities::makeTmpFile(file, "bc");
		string linkedCompositionPath = VuoFileUtilities::makeTmpFile(file, "");

		VuoCompilerIssues *issues = new VuoCompilerIssues();
		compiler->compileComposition(compositionPath, compiledCompositionPath, true, issues);
		compiler->linkCompositionToCreateExecutable(compiledCompositionPath, linkedCompositionPath, VuoCompiler::Optimization_FastBuild);
		delete issues;
		VuoFileUtilities::deleteFile(compiledCompositionPath);

		QVERIFY(! VuoFileUtilities::fileExists(cacheDylib_user));

		QVERIFY(VuoFileUtilities::fileExists(linkedCompositionPath));
		VuoFileUtilities::deleteFile(linkedCompositionPath);

		VuoFileUtilities::deleteFile(VuoFileUtilities::getUserModulesPath() + "/" + nodeClass);
	}

	void testCacheCreationPerformance()
	{
		// Delete the cache to force it to be recreated.
		QDir cacheDir(VuoFileUtilities::getCachePath().c_str());
		cacheDir.removeRecursively();

		delete compiler;
		compiler = NULL;
		VuoCompiler::reset();

		QBENCHMARK {
			bool recreated_user = false;
			bool recreated_composition = false;
			doesCacheGetRecreated(true, recreated_user, recreated_composition);
			QVERIFY(recreated_user);
			QVERIFY(recreated_composition);
		}
	}

	void testCacheCheckingPerformance()
	{
		delete compiler;
		compiler = NULL;
		VuoCompiler::reset();

		// Make sure the cache exists, but force it to be rechecked.
		bool recreated_user = false;
		bool recreated_composition = false;
		doesCacheGetRecreated(true, recreated_user, recreated_composition);

		QBENCHMARK {
			recreated_user = false;
			recreated_composition = false;
			doesCacheGetRecreated(true, recreated_user, recreated_composition);
			QVERIFY(! recreated_user);
			QVERIFY(! recreated_composition);
		}
	}
};


QTEST_APPLESS_MAIN(TestCompilingAndLinking)
#include "TestCompilingAndLinking.moc"
