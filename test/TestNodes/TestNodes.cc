/**
 * @file
 * TestNodes interface and implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "TestCompositionExecution.hh"
#include <Vuo/Vuo.h>
#include "VuoEditor.hh"
#include "VuoRendererCommon.hh"

// Be able to use these types in QTest::addColumn()
Q_DECLARE_METATYPE(VuoNodeSet *);
Q_DECLARE_METATYPE(string);

/**
 * Tests each node class for common mistakes.
 */
class TestNodes : public TestCompositionExecution
{
	Q_OBJECT

private:
	QString singleTestDatum;
	VuoCompiler *compiler;
	set<string> customModuleKeys;
	set<VuoCompilerNodeClass *> builtInNodeClasses;
	set<VuoCompilerType *> builtInTypes;

	bool isCompatibleWithCurrentTarget(VuoCompilerModule *module)
	{
		VuoCompilerCompatibility currentTarget = VuoCompilerCompatibility::compatibilityWithTargetTriple(VuoCompiler::getProcessTarget());
		return module->getCompatibleTargets().isCompatibleWith(currentTarget);
	}

	void makeBuiltInNodeClasses(void)
	{
		map<string, VuoCompilerNodeClass *> nodeClasses = compiler->getNodeClasses();
		for (map<string, VuoCompilerNodeClass *>::iterator i = nodeClasses.begin(); i != nodeClasses.end(); ++i)
		{
			string nodeClassName = i->first;
			VuoCompilerNodeClass *nodeClass = i->second;

			if (customModuleKeys.find(nodeClassName) == customModuleKeys.end() && isCompatibleWithCurrentTarget(nodeClass))
				builtInNodeClasses.insert(nodeClass);
		}
	}

	void makeBuiltInTypes(void)
	{
		map<string, VuoCompilerType *> types = compiler->getTypes();
		for (map<string, VuoCompilerType *>::iterator i = types.begin(); i != types.end(); ++i)
		{
			string typeName = i->first;
			VuoCompilerType *type = i->second;

			if (customModuleKeys.find(typeName) == customModuleKeys.end() && isCompatibleWithCurrentTarget(type))
				builtInTypes.insert(type);
		}
	}

	void compileAndLinkComposition(VuoCompilerComposition *composition)
	{
		string compiledCompositionPath = VuoFileUtilities::makeTmpFile("testEachNode", "bc");
		string linkedCompositionPath = VuoFileUtilities::makeTmpFile("testEachNode-linked", "");

		remove(compiledCompositionPath.c_str());
		remove(linkedCompositionPath.c_str());

		VuoCompilerIssues issues;
		try
		{
			compiler->compileComposition(composition, compiledCompositionPath, true, &issues);
			compiler->linkCompositionToCreateExecutable(compiledCompositionPath, linkedCompositionPath, VuoCompiler::Optimization_SmallBinary);
		}
		catch (VuoException &e)
		{
			VUserLog("%s", e.what());
		}

		ifstream file(linkedCompositionPath.c_str());
		QVERIFY(file.is_open());
		file.close();

		remove(compiledCompositionPath.c_str());
		remove(linkedCompositionPath.c_str());
	}

public:
	TestNodes(int argc, char **argv)
	{
		compiler = initCompiler();

		set<string> customModuleDirs;
		customModuleDirs.insert( VuoFileUtilities::getUserModulesPath() );
		customModuleDirs.insert( VuoFileUtilities::getSystemModulesPath() );
		for (set<string>::iterator i = customModuleDirs.begin(); i != customModuleDirs.end(); ++i)
		{
			string dir = *i;
			set<VuoFileUtilities::File *> files = VuoFileUtilities::findAllFilesInDirectory(dir);
			for (set<VuoFileUtilities::File *>::iterator j = files.begin(); j != files.end(); ++j)
			{
				string dir, file, ext;
				VuoFileUtilities::splitPath((*j)->getRelativePath(), dir, file, ext);
				customModuleKeys.insert(file);
			}
		}

		if (argc > 1)
		{
			// If a single test is specified on the command line,
			// improve performance by loading only data for that test.
			singleTestDatum = QString::fromUtf8(argv[1]).section(':', 1);
			if (singleTestDatum.isEmpty())
			{
				makeBuiltInNodeClasses();
				makeBuiltInTypes();
			}
		}
	}

	~TestNodes()
	{
		delete compiler;
	}

private slots:
	/**
	 * Fires an event through all (published) input ports of the node class simultaneously,
	 * followed by an event through each input port that has a port action.
	 */
	void fireEventsThroughInputPorts(VuoRunner *runner, VuoCompilerNodeClass *nodeClass)
	{
		vector<VuoRunner::Port *> inputPortsVec = runner->getPublishedInputPorts();
		set<VuoRunner::Port *> inputPorts(inputPortsVec.begin(), inputPortsVec.end());
		runner->firePublishedInputPortEvent(inputPorts);
		runner->waitForFiredPublishedInputPortEvent();

		foreach (VuoPortClass *portClass, nodeClass->getBase()->getInputPortClasses())
		{
			if (portClass->hasPortAction())
			{
				VuoRunner::Port *port = runner->getPublishedInputPortWithName( portClass->getName() );
				runner->firePublishedInputPortEvent(port);
				runner->waitForFiredPublishedInputPortEvent();
			}
		}
	}

	void checkLinks(QString markdownText)
	{
		// vuo-node:
		auto internalLinks = QRegularExpression(VuoEditor::vuoNodeDocumentationScheme + ":(//)?([^)]+)").globalMatch(markdownText);
		while (internalLinks.hasNext()) {
			QString nodeClassName = internalLinks.next().captured(2);
			VuoCompilerNodeClass *nodeClass = compiler->getNodeClass(nodeClassName.toStdString());
			if (!nodeClass)
				QFAIL(("Links to the '" + nodeClassName + "' node, which doesn't exist.").toUtf8().data());
			if (nodeClass->getBase()->getDeprecated())
				QFAIL(("Links to the '" + nodeClassName + "' node, which is deprecated.").toUtf8().data());
		}

		// vuo-nodeset:
		internalLinks = QRegularExpression(VuoEditor::vuoNodeSetDocumentationScheme + ":(//)?([^)]+)").globalMatch(markdownText);
		while (internalLinks.hasNext()) {
			QString nodeSetName = internalLinks.next().captured(2);
			VuoNodeSet *nodeSet = compiler->getNodeSetForName(nodeSetName.toStdString());
			if (!nodeSet)
				QFAIL(("Links to the '" + nodeSetName + "' node set, which doesn't exist.").toUtf8().data());
		}
	}

	void testEachNodeSet_data()
	{
		QTest::addColumn<VuoNodeSet *>("nodeSet");

		for (auto i : compiler->getNodeSets())
			QTest::newRow(i.first.c_str()) << i.second;
	}
	void testEachNodeSet()
	{
		QFETCH(VuoNodeSet *, nodeSet);
		VUserLog("%s", nodeSet->getName().c_str());

		VuoText trimmedDescription = VuoText_trim(nodeSet->getDescription().c_str());
		VuoLocal(trimmedDescription);
		if (VuoText_isEmpty(trimmedDescription))
			QFAIL("Missing node set description.");

		checkLinks(trimmedDescription);
	}

	/**
	 * Tests that each node class lists all necessary dependencies. If not, a composition that contains just that
	 * node class will fail to build.
	 *
	 * Tests that each generic node class successfully compiles when its generic port types are specialized.
	 */
	void testEachNode_data()
	{
		QTest::addColumn<QString>("nodeClassName");

		auto addTestsForNodeClass = ^(VuoCompilerNodeClass *nodeClass){
			string nodeClassName = nodeClass->getBase()->getClassName();

			vector<string> genericTypeNames = VuoCompilerSpecializedNodeClass::getGenericTypeNamesFromPorts(nodeClass);
			if (genericTypeNames.empty())
				// Test each node class in its original form.
				QTest::newRow(nodeClassName.c_str()) << QString::fromStdString(nodeClassName);
			else
			{
				// Test some specializations of each generic node class.
				map<string, vector<string> > specializedTypeNamesForGeneric;
				size_t maxSpecializedTypesCount = 0;

				foreach (string genericTypeName, genericTypeNames)
				{
					VuoGenericType *genericType = NULL;
					vector<VuoPortClass *> inputPortClasses = nodeClass->getBase()->getInputPortClasses();
					vector<VuoPortClass *> outputPortClasses = nodeClass->getBase()->getOutputPortClasses();
					vector<VuoPortClass *> portClasses;
					portClasses.insert(portClasses.end(), inputPortClasses.begin(), inputPortClasses.end());
					portClasses.insert(portClasses.end(), outputPortClasses.begin(), outputPortClasses.end());
					for (vector<VuoPortClass *>::iterator i = portClasses.begin(); i != portClasses.end(); ++i)
					{
						VuoCompilerPortClass *portClass = static_cast<VuoCompilerPortClass *>((*i)->getCompiler());
						VuoGenericType *possibleGenericType = dynamic_cast<VuoGenericType *>(portClass->getDataVuoType());
						if (possibleGenericType &&
								VuoType::extractInnermostTypeName(possibleGenericType->getModuleKey()) == genericTypeName)
						{
							genericType = possibleGenericType;
							break;
						}
					}

					VuoGenericType::Compatibility compatibility;
					vector<string> compatibleTypeNames = genericType->getCompatibleSpecializedTypes(compatibility);
					if (compatibility == VuoGenericType::whitelistedTypes)
					{
						// There are a small number of compatible specialized types, so test all of them.
						for (vector<string>::iterator i = compatibleTypeNames.begin(); i != compatibleTypeNames.end(); ++i)
						{
							string compatibleTypeName = VuoType::extractInnermostTypeName(*i);
							specializedTypeNamesForGeneric[genericTypeName].push_back(compatibleTypeName);
						}
					}
					else
					{
						// All types (or list types) are compatible, so just test one that's different from the
						// default backing type.
						specializedTypeNamesForGeneric[genericTypeName].push_back("VuoPoint2d");
					}

					size_t specializedTypesCount = specializedTypeNamesForGeneric[genericTypeName].size();
					maxSpecializedTypesCount = MAX(maxSpecializedTypesCount, specializedTypesCount);
				}

				for (size_t i = 0; i < maxSpecializedTypesCount; ++i)
				{
					vector<string> specializedTypeNames;
					for (size_t j = 0; j < genericTypeNames.size(); ++j)
					{
						string genericTypeName = genericTypeNames[j];
						size_t specializedTypesCount = specializedTypeNamesForGeneric[genericTypeName].size();
						string specializedTypeName = specializedTypeNamesForGeneric[genericTypeName][(i + j) % specializedTypesCount];
						specializedTypeNames.push_back(specializedTypeName);
					}

					string specializedNodeClassName = VuoCompilerSpecializedNodeClass::createSpecializedNodeClassName(nodeClassName,
																													  specializedTypeNames);
					QTest::newRow(specializedNodeClassName.c_str()) << QString::fromStdString(specializedNodeClassName);
				}
			}
		};

		if (singleTestDatum.isEmpty())
			for (auto nodeClass : builtInNodeClasses)
				addTestsForNodeClass(nodeClass);
		else
			addTestsForNodeClass(compiler->getNodeClass(singleTestDatum.toStdString()));
	}
	void testEachNode()
	{
		QFETCH(QString, nodeClassName);
		VUserLog("%s", nodeClassName.toUtf8().constData());

		VuoCompilerNodeClass *nodeClass = compiler->getNodeClass(nodeClassName.toStdString());

		VuoText trimmedDescription = VuoText_trim(nodeClass->getBase()->getDescription().c_str());
		VuoLocal(trimmedDescription);
		if (!nodeClass->getBase()->getDeprecated()
		  && !dynamic_cast<VuoCompilerSpecializedNodeClass *>(nodeClass)
		  && VuoText_isEmpty(trimmedDescription))
			QFAIL("Missing node description.");

		if (!nodeClass->getBase()->getDeprecated())
			checkLinks(trimmedDescription);

		// Ensure all non-deprecated nodes either have an input port or a trigger port.
		if (!nodeClass->getBase()->getDeprecated()
		  && nodeClass->getBase()->getInputPortClasses().size() <= 1)
		{
			bool hasTriggerOutput = false;
			for (auto i : nodeClass->getBase()->getOutputPortClasses())
				if (i->getPortType() == VuoPortClass::triggerPort)
				{
					hasTriggerOutput = true;
					break;
				}
			if (!hasTriggerOutput)
				QFAIL("This node doesn't have any input ports (aside from the now-hidden Refresh port).");
		}

		if (nodeClassName == "vuo.app.stopComposition")
			QSKIP("Not testing events on this node since it stops the composition.");

		if (nodeClassName == "vuo.ui.open"
		 || nodeClassName == "vuo.ui.save")
			QSKIP("Not testing events on this node since it blocks on user input.");

		// https://b33p.net/kosada/node/12090
		if (nodeClassName == "vuo.video.receive")
			QSKIP("Not testing events on this node since it sporadically fails.");

		// https://b33p.net/kosada/vuo/vuo/-/issues/18815
		if (nodeClassName == "vuo.audio.split.frequency")
			QSKIP("Skipping due to a not-yet-diagnosed memory error.");

		// https://b33p.net/kosada/vuo/vuo/-/issues/18816
		if (nodeClassName == "vuo.image.make.web")
			QSKIP("Skipping due to a not-yet-diagnosed memory error.");

		string composition = wrapNodeInComposition(nodeClass, compiler);

		string compiledCompositionPath = VuoFileUtilities::makeTmpFile("testEachNode", "bc");
		string linkedCompositionPath = VuoFileUtilities::makeTmpFile("testEachNode-linked", "");
		VuoCompilerIssues issues;
		compiler->compileCompositionString(composition, compiledCompositionPath, true, &issues);
		compiler->linkCompositionToCreateExecutable(compiledCompositionPath, linkedCompositionPath, VuoCompiler::Optimization_SmallBinary);
		remove(compiledCompositionPath.c_str());
		VuoRunner *runner = VuoRunner::newSeparateProcessRunnerFromExecutable(linkedCompositionPath, ".", false, true);

		TestRunnerDelegate delegate;
		runner->setDelegate(&delegate);
		runner->setRuntimeChecking(true);
		runner->start();

		vector<json_object *> portValuesToTest{
			nullptr,
			json_object_new_int(0),
			json_object_new_int(1),
			json_object_new_int(-1),
			json_object_new_double(0.5),
			json_object_new_double(NAN),
			json_object_new_array(),
			json_object_new_object(),
			json_object_new_string(""),
		};

		try
		{
			// Test executing the node with its default port values.
			VUserLog("    defaults");
			fireEventsThroughInputPorts(runner, nodeClass);

			for (auto portClass : nodeClass->getBase()->getInputPortClasses())
			{
				VuoCompilerInputDataClass *dataClass = static_cast<VuoCompilerInputEventPortClass *>(portClass->getCompiler())->getDataClass();
				if (dataClass)
				{
					string defaultValue = dataClass->getDefaultValue();

					// Test executing the node with a few specific values in each port.
					map<VuoRunner::Port *, json_object *> m;
					VuoRunner::Port *port = runner->getPublishedInputPortWithName(portClass->getName());
					for (auto portValue : portValuesToTest)
					{
						VUserLog("    %s -> %s", port->getName().c_str(), json_object_to_json_string(portValue));
						m[port] = portValue;
						runner->setPublishedInputPortValues(m);
						fireEventsThroughInputPorts(runner, nodeClass);
					}

					// Restore the port's default value before testing the next port.
					json_object *defaultValueObject = json_tokener_parse(defaultValue.c_str());
					m[port] = defaultValueObject;
					runner->setPublishedInputPortValues(m);
					json_object_put(defaultValueObject);
				}
			}

			runner->stop();
			delete runner;
		}
		catch (VuoException &e)
		{
			VUserLog("error: %s", e.what());
			return;
		}
	}

	/**
	 * Tests all specializations of one generic node class that is compatible with any port type.
	 * This makes sure that every port type can be used with VuoList, which remains easy to forget until...
	 * @todo https://b33p.net/kosada/node/7032
	 */
	void testEachListType_data()
	{
		QTest::addColumn< VuoCompilerNodeClass * >("nodeClass");

		string genericNodeClassName = "vuo.list.get";
		auto addTestForType = ^(VuoCompilerType *type){
			string typeName = type->getBase()->getModuleKey();
			if (VuoType::isListTypeName(typeName) || VuoType::isDictionaryTypeName(typeName) ||
					typeName == "VuoMathExpressionList")  /// @todo https://b33p.net/kosada/node/8550
				return;

			string specializedNodeClassName = VuoCompilerSpecializedNodeClass::createSpecializedNodeClassName(genericNodeClassName,
																											  vector<string>(1, typeName));
			VuoCompilerNodeClass *specializedNodeClass = compiler->getNodeClass(specializedNodeClassName);
			QVERIFY2(specializedNodeClass, specializedNodeClassName.c_str());
			QTest::newRow(typeName.c_str()) << specializedNodeClass;
		};

		if (singleTestDatum.isEmpty())
			for (auto type : builtInTypes)
				addTestForType(type);
		else
			addTestForType(compiler->getType(singleTestDatum.toStdString()));
	}
	void testEachListType()
	{
		QFETCH(VuoCompilerNodeClass *, nodeClass);
		VUserLog("%s", nodeClass->getBase()->getClassName().c_str());

		VuoNode *node = compiler->createNode(nodeClass);

		VuoComposition baseComposition;
		VuoCompilerComposition composition(&baseComposition, NULL);
		baseComposition.addNode(node);

		compileAndLinkComposition(&composition);
	}

	/**
	 * Tests that there are no duplicate symbols between different node classes.
	 */
	void testCompositionContainingAllNodes()
	{
		VuoComposition baseComposition;
		VuoCompilerComposition composition(&baseComposition, NULL);

		set<VuoCompilerNodeClass *> nodeClasses = builtInNodeClasses;
		for (set<VuoCompilerNodeClass *>::iterator i = nodeClasses.begin(); i != nodeClasses.end(); ++i)
		{
			VuoCompilerNodeClass *nodeClass = *i;

			// Add an instance of each node class in its original (possibly generic) form.
			VuoNode *node = compiler->createNode(nodeClass);
			composition.setUniqueGraphvizIdentifierForNode(node);
			baseComposition.addNode(node);

			// If generic, add an instance of the node class specialized with its default backing types.
			// This checks for duplicate symbols between the generic and the specialized node class.
			VuoCompilerSpecializedNodeClass *s = dynamic_cast<VuoCompilerSpecializedNodeClass *>(node->getNodeClass()->getCompiler());
			if (s)
			{
				string specializedNodeClassName = s->createFullySpecializedNodeClassName(node);
				VuoCompilerNodeClass *specializedNodeClass = compiler->getNodeClass(specializedNodeClassName);
				QVERIFY2(specializedNodeClass, specializedNodeClassName.c_str());

				VuoNode *specializedNode = compiler->createNode(specializedNodeClass);
				baseComposition.addNode(specializedNode);
				composition.setUniqueGraphvizIdentifierForNode(specializedNode);
			}
		}

		compileAndLinkComposition(&composition);
	}

	void testLintCompositions_data()
	{
		QTest::addColumn<QString>("compositionFile");

		// Example compositions.
		map<string, VuoNodeSet *> nodeSets = compiler->getNodeSets();
		for (map<string, VuoNodeSet *>::iterator i = nodeSets.begin(); i != nodeSets.end(); ++i)
		{
			vector<string> examples = i->second->getExampleCompositionFileNames();
			for (vector<string>::iterator j = examples.begin(); j != examples.end(); ++j)
				QTest::newRow((i->second->getName() + ":" + *j).c_str()) << ("../../node/" + i->second->getName() + "/examples/" + *j).c_str();
		}

		// Documentation compositions.
		for (auto file : VuoFileUtilities::findFilesInDirectory("../../documentation/composition", { "vuo" }))
			QTest::newRow(("documentation:" + file->basename()).c_str()) << QString::fromStdString(file->path());
	}
	void testLintCompositions()
	{
		QFETCH(QString, compositionFile);

		VuoCompilerGraphvizParser *graphParser = VuoCompilerGraphvizParser::newParserFromCompositionFile(compositionFile.toStdString(), compiler);
		foreach (VuoNode *node, graphParser->getNodes())
			QVERIFY2(!node->getNodeClass()->getDeprecated(), ("Found deprecated node '" + node->getNodeClass()->getClassName() + "' in composition.").c_str());

		foreach (VuoCable *cable, graphParser->getCables())
			QVERIFY2(cable->getToPort() != cable->getToNode()->getRefreshPort(), "Found a cable to a node's refresh port.");

		checkLinks(QString::fromStdString(graphParser->getMetadata()->getDescription()));
	}

	void testNodeExampleReferences_data()
	{
		QTest::addColumn<QString>("nodeClassName");

		if (!singleTestDatum.isEmpty())
		{
			QTest::newRow(singleTestDatum.toUtf8().constData()) << singleTestDatum;
			return;
		}

		set<VuoCompilerNodeClass *> nodeClasses = builtInNodeClasses;
		for (set<VuoCompilerNodeClass *>::iterator i = nodeClasses.begin(); i != nodeClasses.end(); ++i)
		{
			VuoCompilerNodeClass *nodeClass = *i;
			string nodeClassName = nodeClass->getBase()->getClassName();
			QTest::newRow(nodeClassName.c_str()) << QString::fromStdString(nodeClassName);
		}
	}
	void testNodeExampleReferences()
	{
		QFETCH(QString, nodeClassName);

		VuoCompilerNodeClass *nodeClass = compiler->getNodeClass(nodeClassName.toStdString());
		for(string examplePath : nodeClass->getBase()->getExampleCompositionFileNames())
		{
			/// As in @ref VuoNodePopover::generateNodePopoverText().
			string compositionAsString;
			if (!QString::fromStdString(examplePath).startsWith("vuo-example"))
			{
				VuoNodeSet *nodeSet = nodeClass->getBase()->getNodeSet();
				if (nodeSet)
					compositionAsString = nodeSet->getExampleCompositionContents(examplePath);
			}

			else
			{
				QUrl exampleUrl(QString::fromStdString(examplePath));
				string exampleNodeSetName = exampleUrl.host().toStdString();
				string exampleFileName = VuoStringUtilities::substrAfter(exampleUrl.path().toStdString(), "/");

				VuoNodeSet *exampleNodeSet = (compiler? compiler->getNodeSetForName(exampleNodeSetName) : NULL);
				if (exampleNodeSet)
					compositionAsString = exampleNodeSet->getExampleCompositionContents(exampleFileName);
			}

			QVERIFY2(compositionAsString.length() > 0,
					 QString("Node %1 references example composition %2, but I can't find it.")
					 .arg(nodeClassName)
					 .arg(QString::fromStdString(examplePath))
					 .toUtf8().constData());
		}
	}
};

int main(int argc, char *argv[])
{
	qInstallMessageHandler(VuoRendererCommon::messageHandler);
	TestNodes tc(argc, argv);
	QTEST_SET_MAIN_SOURCE_PATH
	return QTest::qExec(&tc, argc, argv);
}

#include "TestNodes.moc"
