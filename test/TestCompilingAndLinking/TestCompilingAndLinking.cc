/**
 * @file
 * TestCompilingAndLinking interface and implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
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

	VuoCompiler *compiler = nullptr;

private slots:

	void testDependencies_data()
	{
		QTest::addColumn< QString >("compositionName");
		QTest::addColumn< QStringList >("expectedDirectDependencies");
		QTest::addColumn< QStringList >("expectedDependencies");

		QStringList alwaysDirectDependencies = { "vuo.event.spinOffEvent2" };
		QStringList alwaysIndirectDependencies = { "VuoHeap", "VuoApp", "zmq", "json-c", "objc", "c", "AppKit.framework" };

		QTest::newRow("Empty composition") << "Empty" << alwaysDirectDependencies << (alwaysDirectDependencies + alwaysIndirectDependencies);

		{
			QStringList direct = { "vuo.transform.make" };
			QStringList indirect = { "VuoPoint3d", "VuoTransform", "VuoList_VuoPoint3d", "VuoBoolean", "VuoList_VuoBoolean", "VuoReal", "VuoList_VuoReal",
									 "VuoText", "VuoList_VuoText", "VuoInteger", "VuoList_VuoInteger", "VuoTextCase", "VuoList_VuoTextCase",
									 "VuoPoint4d", "VuoList_VuoPoint4d", "VuoPoint2d", "VuoList_VuoPoint2d", "VuoTransform2d", "VuoList",
									 "Carbon.framework" };
			QTest::newRow("Node with singleton ports") << "Make3dTransform" << (alwaysDirectDependencies + direct) << (alwaysDirectDependencies + alwaysIndirectDependencies + direct + indirect);
		}

		{
			QStringList direct = { "vuo.list.cut.VuoBlendMode", "vuo.list.make.2.VuoBlendMode" };
			QStringList indirect = { "vuo.list.cut", "VuoBlendMode", "VuoList_VuoBlendMode", "VuoInteger", "VuoList_VuoInteger", "VuoList" };
			QTest::newRow("Node with singleton and list ports of related types") << "CutList" << (alwaysDirectDependencies + direct) << (alwaysDirectDependencies + alwaysIndirectDependencies + direct + indirect);
		}

		{
			QStringList direct = { "vuo.point.make.grid.2d" };
			QStringList indirect = { "VuoBoolean", "VuoList_VuoBoolean", "VuoCurve", "VuoList_VuoCurve", "VuoCurveEasing", "VuoList_VuoCurveEasing",
									 "VuoInteger", "VuoList_VuoInteger", "VuoLoopType", "VuoList_VuoLoopType", "VuoPoint2d", "VuoList_VuoPoint2d",
									 "VuoPoint3d", "VuoList_VuoPoint3d", "VuoRange", "VuoReal", "VuoList_VuoReal", "VuoText", "VuoList_VuoText",
									 "VuoTextCase", "VuoList_VuoTextCase", "VuoList", "Carbon.framework" };
			QTest::newRow("Node with singleton and list ports of unrelated types") << "MakePointsIn2dGrid" << (alwaysDirectDependencies + direct) << (alwaysDirectDependencies + alwaysIndirectDependencies + direct + indirect);
		}

		{
			QStringList direct = { "vuo.list.append.VuoIntegerRange", "vuo.list.make.2.VuoIntegerRange" };
			QStringList indirect = { "vuo.list.append", "VuoInteger", "VuoList_VuoInteger", "VuoIntegerRange", "VuoList_VuoIntegerRange",
									 "VuoReal", "VuoList_VuoReal", "VuoText", "VuoList_VuoText", "VuoTextCase", "VuoList_VuoTextCase", "VuoList",
									 "Carbon.framework" };
			QTest::newRow("Node with list ports and no singleton ports") << "AppendLists" << (alwaysDirectDependencies + direct) << (alwaysDirectDependencies + alwaysIndirectDependencies + direct + indirect);
		}

		{
			QStringList direct = { "vuo.in.2.Event.BlendMode.event.VuoBlendMode", "vuo.out.1.ListOfVerticalAlignmentelements.VuoList_VuoVerticalAlignment",
								   "VuoBlendMode", "VuoList_VuoVerticalAlignment" };
			QStringList indirect = { "VuoList_VuoBlendMode", "VuoVerticalAlignment", "VuoInteger", "VuoList_VuoInteger", "VuoList" };
			QTest::newRow("Published ports and no nodes") << "PublishedPorts" << (alwaysDirectDependencies + direct) << (alwaysDirectDependencies + alwaysIndirectDependencies + direct + indirect);
		}
	}
	void testDependencies()
	{
		QFETCH(QString, compositionName);
		QFETCH(QStringList, expectedDirectDependencies);
		QFETCH(QStringList, expectedDependencies);

		string compositionPath = getCompositionPath(compositionName.toStdString() + ".vuo");
		compiler = initCompiler(compositionPath);

		string compositionContents = VuoFileUtilities::readFileToString(compositionPath);
		VuoCompilerComposition *composition = VuoCompilerComposition::newCompositionFromGraphvizDeclaration(compositionContents, compiler);
		set<string> actualDirectDependenciesSet = compiler->getDirectDependenciesForComposition(composition);
		set<string> actualDependenciesSet = compiler->getDependenciesForComposition(composition);

		QStringList actualDirectDependencies;
		for (string s : actualDirectDependenciesSet)
			actualDirectDependencies.append(QString::fromStdString(s));
		QStringList actualDependencies;
		for (string s : actualDependenciesSet)
			actualDependencies.append(QString::fromStdString(s));

		expectedDirectDependencies.sort();
		expectedDependencies.sort();
		actualDirectDependencies.sort();
		actualDependencies.sort();

		QCOMPARE(actualDirectDependencies, expectedDirectDependencies);
		QCOMPARE(actualDependencies, expectedDependencies);

		delete compiler;
		compiler = nullptr;
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
		compiler = initCompiler(compositionPath);

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
		delete compiler;
		compiler = nullptr;
		VuoCompiler::reset();
	}

	void testLinkingWithoutCrashing_data()
	{
		QTest::addColumn< QString >("linkType");
		QTest::addColumn< VuoCompiler::Optimization >("optimization");

		QTest::newRow("Executable, no module caches") << "EXECUTABLE" << VuoCompiler::Optimization_NoModuleCaches;
		QTest::newRow("Executable, module caches") << "EXECUTABLE" << VuoCompiler::Optimization_ModuleCaches;
		QTest::newRow("Combined dylib, no module caches") << "COMBINED_DYLIB" << VuoCompiler::Optimization_NoModuleCaches;
		QTest::newRow("Combined dylib, module caches") << "COMBINED_DYLIB" << VuoCompiler::Optimization_ModuleCaches;
		QTest::newRow("Composition dylib and resource dylib") << "COMPOSITION_DYLIB_AND_RESOURCE_DYLIB" << VuoCompiler::Optimization_ModuleCaches;
	}
	void testLinkingWithoutCrashing()
	{
		QFETCH(QString, linkType);
		QFETCH(VuoCompiler::Optimization, optimization);

		string compositionPath = getCompositionPath("Recur.vuo");
		compiler = initCompiler(compositionPath);

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

		delete compiler;
		compiler = nullptr;
		VuoCompiler::reset();
	}

	void testLinkingMultipleTimes()
	{
		string compositionPath = getCompositionPath("Recur_Count_Write.vuo");
		compiler = initCompiler(compositionPath);

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
			compiler->linkCompositionToCreateExecutable(bcPath, exePath, VuoCompiler::Optimization_NoModuleCaches);
			delete issues;
			ifstream file(exePath.c_str());
			QVERIFY2(file, qPrintable(QString("Failed to link on iteration %1").arg(i)));
			file.close();

			remove(bcPath.c_str());
			remove(exePath.c_str());
		}

		delete compiler;
		compiler = nullptr;
		VuoCompiler::reset();
	}

	void testCompilingAndLinkingWithGenericNodes_data()
	{
		QTest::addColumn< QString >("compositionName");

		QTest::newRow("Generic node specialized with the type that replaces VuoGenericType (VuoInteger)") << "Recur_Hold_Add_Write_loop";
		QTest::newRow("Generic node specialized with a type included by all node classes (VuoText)") << "StoreRecentText";
		QTest::newRow("Generic node specialized with a type defined within a node set (VuoBlendMode)") << "HoldBlendMode";
		QTest::newRow("Generic node specialized with a list type (VuoList_VuoBlendMode)") << "HoldListOfBlendModes";
		QTest::newRow("Generic node specialized with a list type (VuoList_VuoText)") << "HoldListOfTexts";
		QTest::newRow("Generic node that uses a list type but not the singleton type") << "ShareListOfTexts";
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
		compiler = initCompiler(compositionPath);

		string dir, file, ext;
		VuoFileUtilities::splitPath(compositionPath, dir, file, ext);
		string compiledCompositionPath = VuoFileUtilities::makeTmpFile(file, "bc");
		string linkedCompositionPath = VuoFileUtilities::makeTmpFile(file, "");

		try
		{
			VuoCompilerIssues issues;
			compiler->compileComposition(compositionPath, compiledCompositionPath, true, &issues);
		}
		catch (VuoCompilerException &e)
		{
			QEXPECT_FAIL("Generic node specialized with a list type (VuoList_VuoBlendMode)", "The editor disallows specializing to a list type.", Abort);
			QEXPECT_FAIL("Generic node specialized with a list type (VuoList_VuoText)", "The editor disallows specializing to a list type.", Abort);
			QVERIFY2(false, e.getIssues()->getLongDescription(false).c_str());
		}

		compiler->linkCompositionToCreateExecutable(compiledCompositionPath, linkedCompositionPath, VuoCompiler::Optimization_NoModuleCaches);
		QVERIFY(VuoFileUtilities::fileExists(linkedCompositionPath));

		remove(compiledCompositionPath.c_str());
		remove(linkedCompositionPath.c_str());
		delete compiler;
		compiler = nullptr;
		VuoCompiler::reset();
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
		compiler->linkCompositionToCreateExecutable(compiledCompositionPath, linkedCompositionPath, VuoCompiler::Optimization_NoModuleCaches);
		QVERIFY(VuoFileUtilities::fileExists(linkedCompositionPath));

		remove(compiledCompositionPath.c_str());
		remove(linkedCompositionPath.c_str());
		delete compiler;
		compiler = nullptr;
		VuoCompiler::reset();
	}

	void testNodeClassThatDependsOnDylib_data()
	{
		QTest::addColumn< QString >("linkType");
		QTest::addColumn< VuoCompiler::Optimization >("optimization");

		QTest::newRow("C node class, dynamic libraries for live editing") << "COMPOSITION_DYLIB_AND_RESOURCE_DYLIB" << VuoCompiler::Optimization_ModuleCaches;
		QTest::newRow("C node class, dynamic library, module caches") << "COMBINED_DYLIB" << VuoCompiler::Optimization_ModuleCaches;
		QTest::newRow("C node class, dynamic library, no module caches") << "COMBINED_DYLIB" << VuoCompiler::Optimization_NoModuleCaches;
		QTest::newRow("C node class, executable, module caches") << "EXECUTABLE" << VuoCompiler::Optimization_ModuleCaches;
		QTest::newRow("C node class, executable, no module caches") << "EXECUTABLE" << VuoCompiler::Optimization_NoModuleCaches;
	}
	void testNodeClassThatDependsOnDylib()
	{
		QFETCH(QString, linkType);
		QFETCH(VuoCompiler::Optimization, optimization);

		QDir nodeClassDir(QDir::current().filePath("node-TestCompilingAndLinking"));
		QString nodeClassName = "vuo.test.dependsOnUserDylib";
		QString nodeClassFile = nodeClassName + ".c";
		string nodeClassSrcPath = nodeClassDir.filePath(nodeClassFile).toStdString();
		string nodeClassDstPath = VuoFileUtilities::getUserModulesPath() + "/" + nodeClassFile.toStdString();
		QDir dylibDir(BINARY_DIR "/test/TestCompilingAndLinking");
		QString dylibFile = "libUserDylib.dylib";
		string dylibSrcPath = dylibDir.filePath(dylibFile).toStdString();
		string dylibDstPath = VuoFileUtilities::getUserModulesPath() + "/" + dylibFile.toStdString();
		VuoFileUtilities::copyFile(nodeClassSrcPath, nodeClassDstPath);
		VuoFileUtilities::copyFile(dylibSrcPath, dylibDstPath);

		compiler = initCompiler("/TestCompilingAndLinking/testNodeClassThatDependsOnDylib");

		VuoCompilerNodeClass *nodeClass = compiler->getNodeClass(nodeClassName.toStdString());
		QVERIFY(nodeClass);
		string compositionString = wrapNodeInComposition(nodeClass, compiler);

		VuoRunner *runner = nullptr;
		string workingDirectory = "";

		string processName = "testNodeClassThatDependsOnDylib";
		string compiledCompositionPath = VuoFileUtilities::makeTmpFile(processName, "bc");
		VuoCompilerIssues issues;
		compiler->compileCompositionString(compositionString, compiledCompositionPath, true, &issues);
		QVERIFY2(issues.isEmpty(), issues.getLongDescription(false).c_str());

		if (linkType == "COMPOSITION_DYLIB_AND_RESOURCE_DYLIB")
		{
			string linkedCompositionPath = VuoFileUtilities::makeTmpFile(processName, "dylib");
			std::shared_ptr<VuoRunningCompositionLibraries> runningCompositionLibraries = std::make_shared<VuoRunningCompositionLibraries>();
			runningCompositionLibraries->setDeleteResourceLibraries(true);
			compiler->linkCompositionToCreateDynamicLibraries(compiledCompositionPath, linkedCompositionPath, runningCompositionLibraries.get());
			runner = VuoRunner::newSeparateProcessRunnerFromDynamicLibrary(compiler->getCompositionLoaderPath(), linkedCompositionPath, runningCompositionLibraries, workingDirectory, false, true);
		}
		else if (linkType == "COMBINED_DYLIB")
		{
			string linkedCompositionPath = VuoFileUtilities::makeTmpFile(processName, "dylib");
			compiler->linkCompositionToCreateDynamicLibrary(compiledCompositionPath, linkedCompositionPath, optimization);
			runner = VuoRunner::newCurrentProcessRunnerFromDynamicLibrary(linkedCompositionPath, workingDirectory, true);
		}
		else if (linkType == "EXECUTABLE")
		{
			string linkedCompositionPath = VuoFileUtilities::makeTmpFile(processName, "");
			compiler->linkCompositionToCreateExecutable(compiledCompositionPath, linkedCompositionPath, optimization);
			runner = VuoRunner::newSeparateProcessRunnerFromExecutable(linkedCompositionPath, workingDirectory, false, true);
		}

		runner->start();
		json_object *inValue = VuoInteger_getJson(100);
		VuoRunner::Port *inPort = runner->getPublishedInputPortWithName("in");
		runner->setPublishedInputPortValues({{ inPort, inValue }});
		runner->firePublishedInputPortEvent(inPort);
		runner->waitForFiredPublishedInputPortEvent();
		VuoRunner::Port *outPort = runner->getPublishedOutputPortWithName("out");
		json_object *outValue = runner->getPublishedOutputPortValue(outPort);
		runner->stop();

		QCOMPARE(VuoInteger_makeFromJson(outValue), 200);
		json_object_put(inValue);
		json_object_put(outValue);

		VuoFileUtilities::deleteFile(compiledCompositionPath);
		VuoFileUtilities::deleteFile(nodeClassDstPath);
		VuoFileUtilities::deleteFile(dylibDstPath);
		delete compiler;
		compiler = nullptr;
		VuoCompiler::reset();
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
		compiler = initCompiler(compositionPath);

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
		delete compiler;
		compiler = nullptr;
		VuoCompiler::reset();
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
		compiler = initCompiler(compositionPath);

		string dir, file, ext;
		VuoFileUtilities::splitPath(compositionPath, dir, file, ext);
		string compiledCompositionPath = VuoFileUtilities::makeTmpFile(file, "bc");
		string linkedCompositionPath = VuoFileUtilities::makeTmpFile(file, "dylib");

		// Simulate the most common case: the cache already exists when the process starts.
		compiler->makeModuleCachesAvailable(true, false);

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

		delete compiler;
		compiler = nullptr;
		VuoCompiler::reset();
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
		compiler = initCompiler(compositionPath);

		string dir, file, ext;
		VuoFileUtilities::splitPath(compositionPath, dir, file, ext);
		string compiledCompositionPath = VuoFileUtilities::makeTmpFile(file, "bc");
		string linkedCompositionPath = VuoFileUtilities::makeTmpFile(file, "dylib");

		// Simulate the most common case: the cache already exists when the process starts.
		compiler->makeModuleCachesAvailable(true, false);

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

		delete compiler;
		compiler = nullptr;
		VuoCompiler::reset();
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
		compiler = new VuoCompiler(compositionPath);

		// Prepare the module caches, including modules in the composition-local environment.
		VuoCompilerIssues tmpIssues;
		string tmpPath = VuoFileUtilities::makeTmpFile("testCompilingAndLinkingInSeparateProcessPerformance", "bc");
		compiler->compileComposition(compositionPath, tmpPath, true, &tmpIssues);
		VuoFileUtilities::deleteFile(tmpPath);

		QBENCHMARK {
			VuoCompilerIssues *issues = new VuoCompilerIssues();
			VuoRunner *runner = VuoCompiler::newSeparateProcessRunnerFromCompositionFile(compositionPath, issues);
			QVERIFY(runner);
			delete issues;
			delete runner;
		}

		delete compiler;
		compiler = nullptr;
		VuoCompiler::reset();
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
		compiler = initCompiler(compositionPath);

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

		delete compiler;
		compiler = nullptr;
		VuoCompiler::reset();
	}
};


QTEST_APPLESS_MAIN(TestCompilingAndLinking)
#include "TestCompilingAndLinking.moc"
