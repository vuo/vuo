/**
 * @file
 * TestCompilingAndLinking interface and implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
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

private slots:

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

	void testCompilingAndLinkingWithLocalModules_data()
	{
		QTest::addColumn<QString>("compositionPath");
		QTest::addColumn<QString>("whenToSetPath");
		QTest::addColumn<bool>("isTopLevelComposition");

		// Top-level composition depends on subcomposition
		{
			QString compositionPath = QDir(getCompositionDir().filePath("CompDependsOnSubcomp")).filePath("CompDependsOnSubcomp.vuo");
			QTest::newRow("top-level composition: VuoCompiler") << compositionPath << "VuoCompiler" << true;
			QTest::newRow("top-level composition: setCompositionPath") << compositionPath << "setCompositionPath" << true;
			QTest::newRow("top-level composition: compileComposition") << compositionPath << "compileComposition" << true;
		}

		// Subcomposition depends on another subcomposition
		{
			QString compositionPath = QDir(QDir(getCompositionDir().filePath("SubcompDependsOnSubcomp")).filePath("Modules")).filePath("test.subcomp2.vuo");
			QTest::newRow("subcomposition: VuoCompiler") << compositionPath << "VuoCompiler" << false;
			QTest::newRow("subcomposition: setCompositionPath") << compositionPath << "setCompositionPath" << false;
			QTest::newRow("subcomposition: compileComposition") << compositionPath << "compileComposition" << false;
		}
	}
	void testCompilingAndLinkingWithLocalModules()
	{
		QFETCH(QString, compositionPath);
		QFETCH(QString, whenToSetPath);
		QFETCH(bool, isTopLevelComposition);

		delete compiler;

		if (whenToSetPath.toStdString() == "VuoCompiler")
			compiler = new VuoCompiler(compositionPath.toStdString());
		else
			compiler = new VuoCompiler();

		if (whenToSetPath.toStdString() == "setCompositionPath")
			compiler->setCompositionPath(compositionPath.toStdString());

		string dir, file, ext;
		VuoFileUtilities::splitPath(compositionPath.toStdString(), dir, file, ext);
		string compiledCompositionPath = VuoFileUtilities::makeTmpFile(file, "bc");
		string linkedCompositionPath = VuoFileUtilities::makeTmpFile(file, "");

		VuoCompilerIssues *issues = new VuoCompilerIssues();
		compiler->compileComposition(compositionPath.toStdString(), compiledCompositionPath, isTopLevelComposition, issues);
		delete issues;
		compiler->linkCompositionToCreateExecutable(compiledCompositionPath, linkedCompositionPath, VuoCompiler::Optimization_SmallBinary);
		QVERIFY(VuoFileUtilities::fileExists(linkedCompositionPath));

		remove(compiledCompositionPath.c_str());
		remove(linkedCompositionPath.c_str());
	}

	void testCompilingPerformance_data()
	{
		QTest::addColumn< QString >("compositionName");

		QTest::newRow("Empty")                                << "Empty";
		QTest::newRow("LineOfSameNodeClass")                  << "LineOfSameNodeClass";
		QTest::newRow("LineOfDiffNodeClass")                  << "LineOfDiffNodeClass";
		QTest::newRow("LineOfSameNodeClassSameType")          << "LineOfSameNodeClassSameType";
		QTest::newRow("LineOfSameNodeClassDiffType")          << "LineOfSameNodeClassDiffType";
		QTest::newRow("LineOfDrawersOfDiffType")              << "LineOfDrawersOfDiffType";
		QTest::newRow("UnconnectedTriggers")                  << "UnconnectedTriggers";
		QTest::newRow("DiamondOfSameNodeClass")               << "DiamondOfSameNodeClass";
		QTest::newRow("LineOfSameNodeClassMultiTriggers")     << "LineOfSameNodeClassMultiTriggers";
		QTest::newRow("EachPairOfSameNodeClassMultiTriggers") << "EachPairOfSameNodeClassMultiTriggers";
		QTest::newRow("LineOfSameNodeClass100")               << "LineOfSameNodeClass100";
		QTest::newRow("LineOfSameNodeClass200")               << "LineOfSameNodeClass200";
		QTest::newRow("LineOfSameNodeClass400")               << "LineOfSameNodeClass400";
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

		QTest::newRow("Empty")                                << "Empty";
		QTest::newRow("LineOfSameNodeClass")                  << "LineOfSameNodeClass";
		QTest::newRow("LineOfDiffNodeClass")                  << "LineOfDiffNodeClass";
		QTest::newRow("LineOfSameNodeClassSameType")          << "LineOfSameNodeClassSameType";
		QTest::newRow("LineOfSameNodeClassDiffType")          << "LineOfSameNodeClassDiffType";
		QTest::newRow("LineOfDrawersOfDiffType")              << "LineOfDrawersOfDiffType";
		QTest::newRow("UnconnectedTriggers")                  << "UnconnectedTriggers";
		QTest::newRow("DiamondOfSameNodeClass")               << "DiamondOfSameNodeClass";
		QTest::newRow("LineOfSameNodeClassMultiTriggers")     << "LineOfSameNodeClassMultiTriggers";
		QTest::newRow("EachPairOfSameNodeClassMultiTriggers") << "EachPairOfSameNodeClassMultiTriggers";
		QTest::newRow("LineOfSameNodeClass100")               << "LineOfSameNodeClass100";
		QTest::newRow("LineOfSameNodeClass200")               << "LineOfSameNodeClass200";
		QTest::newRow("LineOfSameNodeClass400")               << "LineOfSameNodeClass400";
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

		QTest::newRow("Empty")                                << "Empty";
		QTest::newRow("LineOfSameNodeClass")                  << "LineOfSameNodeClass";
		QTest::newRow("LineOfDiffNodeClass")                  << "LineOfDiffNodeClass";
		QTest::newRow("LineOfSameNodeClassSameType")          << "LineOfSameNodeClassSameType";
		QTest::newRow("LineOfSameNodeClassDiffType")          << "LineOfSameNodeClassDiffType";
		QTest::newRow("LineOfDrawersOfDiffType")              << "LineOfDrawersOfDiffType";
		QTest::newRow("UnconnectedTriggers")                  << "UnconnectedTriggers";
		QTest::newRow("DiamondOfSameNodeClass")               << "DiamondOfSameNodeClass";
		QTest::newRow("LineOfSameNodeClassMultiTriggers")     << "LineOfSameNodeClassMultiTriggers";
		QTest::newRow("EachPairOfSameNodeClassMultiTriggers") << "EachPairOfSameNodeClassMultiTriggers";
		QTest::newRow("LineOfSameNodeClass100")               << "LineOfSameNodeClass100";
		QTest::newRow("LineOfSameNodeClass200")               << "LineOfSameNodeClass200";
		QTest::newRow("LineOfSameNodeClass400")               << "LineOfSameNodeClass400";
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

		QTest::newRow("Empty")                                << "Empty";
		QTest::newRow("LineOfSameNodeClass")                  << "LineOfSameNodeClass";
		QTest::newRow("LineOfDiffNodeClass")                  << "LineOfDiffNodeClass";
		QTest::newRow("LineOfSameNodeClassSameType")          << "LineOfSameNodeClassSameType";
		QTest::newRow("LineOfSameNodeClassDiffType")          << "LineOfSameNodeClassDiffType";
		QTest::newRow("LineOfDrawersOfDiffType")              << "LineOfDrawersOfDiffType";
		QTest::newRow("UnconnectedTriggers")                  << "UnconnectedTriggers";
		QTest::newRow("DiamondOfSameNodeClass")               << "DiamondOfSameNodeClass";
		QTest::newRow("LineOfSameNodeClassMultiTriggers")     << "LineOfSameNodeClassMultiTriggers";
		QTest::newRow("EachPairOfSameNodeClassMultiTriggers") << "EachPairOfSameNodeClassMultiTriggers";
		QTest::newRow("LineOfSameNodeClass100")               << "LineOfSameNodeClass100";
		QTest::newRow("LineOfSameNodeClass200")               << "LineOfSameNodeClass200";
		QTest::newRow("LineOfSameNodeClass400")               << "LineOfSameNodeClass400";
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

		QTest::newRow("LineOfSameNodeClass")                  << "LineOfSameNodeClass";
		QTest::newRow("DiamondOfSameNodeClass")               << "DiamondOfSameNodeClass";
		QTest::newRow("LineOfSameNodeClassMultiTriggers")     << "LineOfSameNodeClassMultiTriggers";
		QTest::newRow("EachPairOfSameNodeClassMultiTriggers") << "EachPairOfSameNodeClassMultiTriggers";
		QTest::newRow("LineOfSameNodeClass100")               << "LineOfSameNodeClass100";
		QTest::newRow("LineOfSameNodeClass200")               << "LineOfSameNodeClass200";
		QTest::newRow("LineOfSameNodeClass400")               << "LineOfSameNodeClass400";
		QTest::newRow("PublishedPassthrough50")               << "PublishedPassthrough50";
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

	string getUserCacheDir(void)
	{
		return VuoFileUtilities::getCachePath() + "/User";
	}

	string getCompositionCacheDir(void)
	{
		return VuoCompiler::getCachePathForComposition(VuoFileUtilities::getTmpDir() + "/TestCompilingAndLinking");
	}

	string getUserCacheIndex(void)
	{
		return VuoFileUtilities::buildModuleCacheIndexPath(getUserCacheDir(), false, false);
	}

	string getCompositionCacheIndex(void)
	{
		return VuoFileUtilities::buildModuleCacheIndexPath(getCompositionCacheDir(), false, false);
	}

	void deleteAllUserCacheDylibs(void)
	{
		string path = VuoFileUtilities::buildModuleCacheDylibPath(getUserCacheDir(), false, false);
		VuoFileUtilities::deleteOtherRevisionsOfModuleCacheDylib(path);
	}

	void deleteAllCompositionCacheDylibs(void)
	{
		string path = VuoFileUtilities::buildModuleCacheDylibPath(getCompositionCacheDir(), false, false);
		VuoFileUtilities::deleteOtherRevisionsOfModuleCacheDylib(path);
	}

	void doesCacheGetRecreated(bool shouldResetCompiler, bool &user, bool &composition, string &cacheDylib_user, string &cacheDylib_composition)
	{
		unsigned long cacheDylibLastModified_user = (! cacheDylib_user.empty() && VuoFileUtilities::fileExists(cacheDylib_user) ?
														 VuoFileUtilities::getFileLastModifiedInSeconds(cacheDylib_user) : 0);
		unsigned long cacheIndexLastModified_user = (VuoFileUtilities::fileExists(getUserCacheIndex()) ?
														 VuoFileUtilities::getFileLastModifiedInSeconds(getUserCacheIndex()) : 0);
		unsigned long cacheDylibLastModified_composition = (! cacheDylib_composition.empty() && VuoFileUtilities::fileExists(cacheDylib_composition) ?
																VuoFileUtilities::getFileLastModifiedInSeconds(cacheDylib_composition) : 0);
		unsigned long cacheIndexLastModified_composition = (VuoFileUtilities::fileExists(getCompositionCacheIndex()) ?
																VuoFileUtilities::getFileLastModifiedInSeconds(getCompositionCacheIndex()) : 0);

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

		unsigned long newCacheDylibLastModified_user = 0;
		string newCacheDylib_user = VuoFileUtilities::findLatestRevisionOfModuleCacheDylib(getUserCacheDir(), false, false, newCacheDylibLastModified_user);
		QVERIFY(! newCacheDylib_user.empty());

		if (VuoFileUtilities::arePathsEqual(newCacheDylib_user, cacheDylib_user))
		{
			user = false;
			QVERIFY(newCacheDylibLastModified_user == cacheDylibLastModified_user);
			QVERIFY(VuoFileUtilities::getFileLastModifiedInSeconds(getUserCacheIndex()) == cacheIndexLastModified_user);
		}
		else
		{
			user = true;
			QVERIFY(! VuoFileUtilities::fileExists(cacheDylib_user));
			cacheDylib_user = newCacheDylib_user;
		}

		unsigned long newCacheDylibLastModified_composition = 0;
		string newCacheDylib_composition = VuoFileUtilities::findLatestRevisionOfModuleCacheDylib(getCompositionCacheDir(), false, false, newCacheDylibLastModified_composition);
		QVERIFY(! newCacheDylib_composition.empty());

		if (VuoFileUtilities::arePathsEqual(newCacheDylib_composition, cacheDylib_composition))
		{
			composition = false;
			QVERIFY(newCacheDylibLastModified_composition == cacheDylibLastModified_composition);
			QVERIFY(VuoFileUtilities::getFileLastModifiedInSeconds(getCompositionCacheIndex()) == cacheIndexLastModified_composition);
		}
		else
		{
			composition = true;
			QVERIFY(! VuoFileUtilities::fileExists(cacheDylib_composition));
			cacheDylib_composition = newCacheDylib_composition;
		}
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

		VuoFileUtilities::deleteFile(getUserCacheIndex());
		VuoFileUtilities::deleteFile(getCompositionCacheIndex());
		deleteAllUserCacheDylibs();
		deleteAllCompositionCacheDylibs();

		delete compiler;
		compiler = NULL;
		VuoCompiler::reset();

		bool recreated_user = false;
		bool recreated_composition = false;
		string cacheDylib_user;
		string cacheDylib_composition;

		// Cache files do not exist.
		doesCacheGetRecreated(true, recreated_user, recreated_composition, cacheDylib_user, cacheDylib_composition);
		QVERIFY(recreated_user);
		QVERIFY(recreated_composition);

		// Cache files are up to date.
		doesCacheGetRecreated(true, recreated_user, recreated_composition, cacheDylib_user, cacheDylib_composition);
		QVERIFY(! recreated_user);
		QVERIFY(! recreated_composition);

		// Cache dylib for user modules does not exist.
		VuoFileUtilities::deleteFile(cacheDylib_user);
		cacheDylib_user = "";
		doesCacheGetRecreated(true, recreated_user, recreated_composition, cacheDylib_user, cacheDylib_composition);
		QVERIFY(recreated_user);
		QVERIFY(recreated_composition);

		// Cache dylib for composition modules does not exist.
		VuoFileUtilities::deleteFile(cacheDylib_composition);
		cacheDylib_composition = "";
		doesCacheGetRecreated(true, recreated_user, recreated_composition, cacheDylib_user, cacheDylib_composition);
		QVERIFY(! recreated_user);
		QVERIFY(recreated_composition);

		// Node class added to composition modules folder.
		VuoFileUtilities::copyFile(nodeClassInNodeDir, nodeClassInCompositionModulesDir);
		doesCacheGetRecreated(true, recreated_user, recreated_composition, cacheDylib_user, cacheDylib_composition);
		QVERIFY(! recreated_user);
		QVERIFY(recreated_composition);

		// Node class updated in composition modules folder.
		string escapedNodeClassInModuleDir = "\"" + nodeClassInCompositionModulesDir + "\"";
		system(("/usr/bin/touch " + escapedNodeClassInModuleDir).c_str());
		doesCacheGetRecreated(true, recreated_user, recreated_composition, cacheDylib_user, cacheDylib_composition);
		QVERIFY(! recreated_user);
		QVERIFY(recreated_composition);

		// Node class deleted from composition modules folder.
		VuoFileUtilities::deleteFile(nodeClassInCompositionModulesDir);
		doesCacheGetRecreated(true, recreated_user, recreated_composition, cacheDylib_user, cacheDylib_composition);
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
		string cacheDylib_user;
		string cacheDylib_composition;

		// Create caches initially, if needed.
		doesCacheGetRecreated(false, recreated_user, recreated_composition, cacheDylib_user, cacheDylib_composition);
		sleep(2);

		// Node class added to composition modules folder.
		delegate.installModule(nodeClassInNodeDir, nodeClassInCompositionModulesDir);
		doesCacheGetRecreated(false, recreated_user, recreated_composition, cacheDylib_user, cacheDylib_composition);
		QVERIFY(! recreated_user);
		QVERIFY(recreated_composition);

		// Node class updated in composition modules folder.
		delegate.installModule(nodeClassInNodeDir, nodeClassInCompositionModulesDir);
		doesCacheGetRecreated(false, recreated_user, recreated_composition, cacheDylib_user, cacheDylib_composition);
		QVERIFY(! recreated_user);
		QVERIFY(recreated_composition);

		// Node class deleted from composition modules folder.
		delegate.uninstallModule(nodeClassInCompositionModulesDir);
		doesCacheGetRecreated(false, recreated_user, recreated_composition, cacheDylib_user, cacheDylib_composition);
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

		deleteAllUserCacheDylibs();

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

		unsigned long lastModified = 0;
		string cacheDylib_user = VuoFileUtilities::findLatestRevisionOfModuleCacheDylib(getUserCacheDir(), false, false, lastModified);
		QVERIFY(cacheDylib_user.empty());

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
			string cacheDylib_user;
			string cacheDylib_composition;

			doesCacheGetRecreated(true, recreated_user, recreated_composition, cacheDylib_user, cacheDylib_composition);

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
		string cacheDylib_user;
		string cacheDylib_composition;

		doesCacheGetRecreated(true, recreated_user, recreated_composition, cacheDylib_user, cacheDylib_composition);

		QBENCHMARK {
			doesCacheGetRecreated(true, recreated_user, recreated_composition, cacheDylib_user, cacheDylib_composition);

			QVERIFY(! recreated_user);
			QVERIFY(! recreated_composition);
		}
	}
};


QTEST_APPLESS_MAIN(TestCompilingAndLinking)
#include "TestCompilingAndLinking.moc"
