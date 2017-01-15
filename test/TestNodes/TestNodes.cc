/**
 * @file
 * TestNodes interface and implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "TestCompositionExecution.hh"
#include <Vuo/Vuo.h>

// Be able to use these types in QTest::addColumn()
Q_DECLARE_METATYPE(VuoCompilerNodeClass *);

/**
 * Tests each node class for common mistakes.
 */
class TestNodes : public TestCompositionExecution
{
	Q_OBJECT

private:
	VuoCompiler *compiler;
	set<string> customModuleKeys;
	set<VuoCompilerNodeClass *> builtInNodeClasses;
	set<VuoCompilerType *> builtInTypes;

	bool isCompatibleWithCurrentTarget(VuoCompilerModule *module)
	{
		VuoCompilerTargetSet currentTarget;
		currentTarget.restrictToCurrentOperatingSystemVersion();
		VuoCompilerTargetSet compatibleTargets = module->getCompatibleTargets();

		/// @todo https://b33p.net/kosada/node/6309
		VuoNodeSet *nodeSet = module->getPseudoBase()->getNodeSet();
		if (nodeSet && nodeSet->getName() == "vuo.leap")
			compatibleTargets.setMinMacVersion(VuoCompilerTargetSet::MacVersion_10_7);

		return compatibleTargets.isCompatibleWithAllOf(currentTarget);
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

		try
		{
			compiler->compileComposition(composition, compiledCompositionPath);
			compiler->linkCompositionToCreateExecutable(compiledCompositionPath, linkedCompositionPath, VuoCompiler::Optimization_SmallBinary);
		}
		catch (exception &e)
		{
			fprintf(stderr, "%s\n", e.what());
		}

		ifstream file(linkedCompositionPath.c_str());
		QVERIFY(file);
		file.close();

		remove(compiledCompositionPath.c_str());
		remove(linkedCompositionPath.c_str());
	}

private slots:

	void initTestCase()
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

		makeBuiltInNodeClasses();
		makeBuiltInTypes();
	}

	void cleanupTestCase()
	{
		delete compiler;
	}

	/**
	 * Fires an event through all (published) input ports of the node class simultaneously,
	 * followed by an event through each input port that has a port action.
	 */
	void fireEventsThroughInputPorts(VuoRunner *runner, VuoCompilerNodeClass *nodeClass)
	{
		runner->firePublishedInputPortEvent();
		foreach (VuoPortClass *portClass, nodeClass->getBase()->getInputPortClasses())
		{
			if (portClass->hasPortAction())
			{
				VuoRunner::Port *port = runner->getPublishedInputPortWithName( portClass->getName() );
				runner->firePublishedInputPortEvent(port);
			}
		}
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

		set<VuoCompilerNodeClass *> nodeClasses = builtInNodeClasses;
		for (set<VuoCompilerNodeClass *>::iterator i = nodeClasses.begin(); i != nodeClasses.end(); ++i)
		{
			VuoCompilerNodeClass *nodeClass = *i;
			string nodeClassName = nodeClass->getBase()->getClassName();

			// Test each node class in its original (possibly generic) form.
			QTest::newRow(nodeClassName.c_str()) << QString::fromStdString(nodeClassName);

			// Test some specializations of each generic node class.
			vector<string> genericTypeNames = VuoCompilerSpecializedNodeClass::getGenericTypeNamesFromPorts(nodeClass);
			if (! genericTypeNames.empty())
			{
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
		}
	}
	void testEachNode()
	{
		QFETCH(QString, nodeClassName);
		printf("%s\n", nodeClassName.toUtf8().constData()); fflush(stdout);

		VuoCompilerNodeClass *nodeClass = compiler->getNodeClass(nodeClassName.toStdString());
		string composition = wrapNodeInComposition(nodeClass, compiler);

		string compiledCompositionPath = VuoFileUtilities::makeTmpFile("testEachNode", "bc");
		string linkedCompositionPath = VuoFileUtilities::makeTmpFile("testEachNode-linked", "");
		compiler->compileCompositionString(composition, compiledCompositionPath);
		compiler->linkCompositionToCreateExecutable(compiledCompositionPath, linkedCompositionPath, VuoCompiler::Optimization_SmallBinary);
		remove(compiledCompositionPath.c_str());
		VuoRunner *runner = VuoRunner::newSeparateProcessRunnerFromExecutable(linkedCompositionPath, ".", false, true);

		TestRunnerDelegate delegate;
		runner->setDelegate(&delegate);
		runner->start();

		fireEventsThroughInputPorts(runner, nodeClass);

		foreach (VuoPortClass *portClass, nodeClass->getBase()->getInputPortClasses())
		{
			VuoCompilerInputDataClass *dataClass = static_cast<VuoCompilerInputEventPortClass *>(portClass->getCompiler())->getDataClass();
			if (dataClass)
			{
				string defaultValue = dataClass->getDefaultValue();
				if (! defaultValue.empty())
				{
					VuoRunner::Port *port = runner->getPublishedInputPortWithName( portClass->getName() );
					runner->setPublishedInputPortValue(port, NULL);

					fireEventsThroughInputPorts(runner, nodeClass);

					json_object *defaultValueObject = json_tokener_parse(defaultValue.c_str());
					runner->setPublishedInputPortValue(port, defaultValueObject);
					json_object_put(defaultValueObject);
				}
			}
		}

		runner->stop();
		delete runner;
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
		set<VuoCompilerType *> types = builtInTypes;
		for (set<VuoCompilerType *>::iterator i = types.begin(); i != types.end(); ++i)
		{
			string typeName = (*i)->getBase()->getModuleKey();
			if (VuoType::isListTypeName(typeName) || VuoType::isDictionaryTypeName(typeName) ||
					typeName == "VuoMathExpressionList")  /// @todo https://b33p.net/kosada/node/8550
				continue;

			string specializedNodeClassName = VuoCompilerSpecializedNodeClass::createSpecializedNodeClassName(genericNodeClassName,
																											  vector<string>(1, typeName));
			VuoCompilerNodeClass *specializedNodeClass = compiler->getNodeClass(specializedNodeClassName);
			QVERIFY2(specializedNodeClass, specializedNodeClassName.c_str());
			QTest::newRow(specializedNodeClassName.c_str()) << specializedNodeClass;
		}
	}
	void testEachListType()
	{
		QFETCH(VuoCompilerNodeClass *, nodeClass);
		printf("%s\n", nodeClass->getBase()->getClassName().c_str()); fflush(stdout);

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

};

QTEST_APPLESS_MAIN(TestNodes)
#include "TestNodes.moc"
