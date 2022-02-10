/**
 * @file
 * VuoCompilerSpecializedNodeClass implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCompiler.hh"
#include "VuoCompilerException.hh"
#include "VuoCompilerGenericType.hh"
#include "VuoCompilerInputDataClass.hh"
#include "VuoCompilerInputEventPort.hh"
#include "VuoCompilerInputEventPortClass.hh"
#include "VuoCompilerIssue.hh"
#include "VuoCompilerMakeListNodeClass.hh"
#include "VuoCompilerNode.hh"
#include "VuoCompilerOutputDataClass.hh"
#include "VuoCompilerOutputEventPortClass.hh"
#include "VuoCompilerPublishedInputNodeClass.hh"
#include "VuoCompilerPublishedOutputNodeClass.hh"
#include "VuoCompilerTriggerPortClass.hh"
#include "VuoGenericType.hh"
#include "VuoNode.hh"
#include "VuoNodeClass.hh"
#include "VuoNodeSet.hh"
#include "VuoPort.hh"
#include "VuoStringUtilities.hh"


/**
 * Creates a specialized node class implementation from an LLVM module, and creates its corresponding base @c VuoNodeClass.
 */
VuoCompilerSpecializedNodeClass::VuoCompilerSpecializedNodeClass(string nodeClassName, Module *module) :
	VuoCompilerNodeClass(nodeClassName, module)
{
	initialize();
}

/**
 * Creates a new compiler node class and creates a new base @c VuoNodeClass, both from @c compilerNodeClass.
 */
VuoCompilerSpecializedNodeClass::VuoCompilerSpecializedNodeClass(VuoCompilerSpecializedNodeClass *compilerNodeClass) :
	VuoCompilerNodeClass(compilerNodeClass)
{
	initialize();
}

/**
 * Creates a new implementation-less compiler node class, using the given node class for its base VuoNodeClass.
 */
VuoCompilerSpecializedNodeClass::VuoCompilerSpecializedNodeClass(VuoNodeClass *baseNodeClass) :
	VuoCompilerNodeClass(baseNodeClass)
{
	initialize();
}

/**
 * Helper function for constructors.
 */
void VuoCompilerSpecializedNodeClass::initialize(void)
{
	genericNodeClass = NULL;
	backingNodeClass = NULL;
}

/**
 * Generates a specalized node class from a generic node class.
 *
 * @param nodeClassName The name of the node class to generate. It should have the format
 *		"<generic node class name>.<type>.(...).<type>" (e.g. "vuo.dictionary.make.VuoText.VuoInteger").
 * @param compiler The compiler to use for looking up the generic node class and compiling the specialized node class.
 * @param llvmQueue Synchronizes access to LLVM's global context.
 * @return The generated node class, or null if the generic node class is not found. If @a nodeClassName is fully specialized
 *		(doesn't contain any generic type names), then the returned node class has an LLVM module associated with it.
 *		Otherwise, the returned node class does not yet have an implementation.
 */
VuoNodeClass * VuoCompilerSpecializedNodeClass::newNodeClass(const string &nodeClassName, VuoCompiler *compiler, dispatch_queue_t llvmQueue)
{
	if (nodeClassName.find(".") == string::npos)
		return NULL;

	VuoNodeClass *makeListNodeClass = VuoCompilerMakeListNodeClass::newNodeClass(nodeClassName, compiler, llvmQueue);
	if (makeListNodeClass)
		return makeListNodeClass;

	VuoNodeClass *publishedInputNodeClass = VuoCompilerPublishedInputNodeClass::newNodeClass(nodeClassName, compiler, llvmQueue);
	if (publishedInputNodeClass)
		return publishedInputNodeClass;

	VuoNodeClass *publishedOutputNodeClass = VuoCompilerPublishedOutputNodeClass::newNodeClass(nodeClassName, compiler, llvmQueue);
	if (publishedOutputNodeClass)
		return publishedOutputNodeClass;


	// Find the generic node class that the given node class should specialize

	VuoCompilerNodeClass *potentialGenericNodeClass = NULL;
	vector<string> parts = VuoStringUtilities::split(nodeClassName, '.');
	for (int j = parts.size() - 1; j >= 1; --j)
	{
		string newLastPart = parts[j];
		if (! compiler->getType(newLastPart))
			break;

		vector<string> firstParts(parts.begin(), parts.begin() + j);
		string potentialGenericNodeClassName = VuoStringUtilities::join(firstParts, '.');

		potentialGenericNodeClass = compiler->getNodeClass(potentialGenericNodeClassName);
		if (potentialGenericNodeClass)
			break;
	}

	VuoCompilerNodeClass *genericNodeClass = NULL;
	vector<string> genericTypeNames;
	if (potentialGenericNodeClass)
	{
		vector<string> potentialGenericTypeNames = getGenericTypeNamesFromPorts(potentialGenericNodeClass);
		string expectedGenericNodeClassName = extractGenericNodeClassName(nodeClassName, potentialGenericTypeNames.size());
		if (expectedGenericNodeClassName == potentialGenericNodeClass->getBase()->getClassName())
		{
			genericNodeClass = potentialGenericNodeClass;
			genericTypeNames = potentialGenericTypeNames;
		}
	}

	if (! genericNodeClass)
		return NULL;


	// Map the generic types (found in ports) to the specialized types (found in the node class name)

	vector<string> nodeClassNameParts = VuoStringUtilities::split(nodeClassName, '.');
	vector<string> specializedTypeNames(nodeClassNameParts.begin() + nodeClassNameParts.size() - genericTypeNames.size(), nodeClassNameParts.end());

	map<string, string> specializedForGenericTypeName;
	bool _isFullySpecialized = true;
	for (size_t i = 0; i < genericTypeNames.size(); ++i)
	{
		if (VuoGenericType::isGenericTypeName( specializedTypeNames[i] ))
			_isFullySpecialized = false;
		else if (! compiler->getType( specializedTypeNames[i] ))
			return NULL;

		specializedForGenericTypeName[ genericTypeNames[i] ] = specializedTypeNames[i];
	}


	__block VuoCompilerSpecializedNodeClass *nodeClass;

	if (_isFullySpecialized)
	{
		// Compile the specialized node class (with any necessary headers for types)

		string genericNodeClassName = genericNodeClass->getBase()->getClassName();
		string genericImplementation = genericNodeClass->getBase()->getNodeSet()
			? genericNodeClass->getBase()->getNodeSet()->getNodeClassContents( genericNodeClassName )
			: genericNodeClass->getSourceCode();
		if (genericImplementation.empty())
		{
			VuoCompilerIssue issue(VuoCompilerIssue::Error, "specializing node in composition", "",
								   "Missing source code",
								   "%module uses generic types, but its source code isn't included in its node set. "
								   "If you are the author of this node, see the API documentation section \"Generic port types\" "
								   "under \"Developing a node class\" (https://api.vuo.org/latest/group___developing_node_classes.html). "
								   "Otherwise, contact the author of this node.");
			issue.setModule(genericNodeClass->getBase());
			throw VuoCompilerException(issue);
		}
		genericImplementation.insert(VuoFileUtilities::getFirstInsertionIndex(genericImplementation), "#define VuoSpecializedNode 1\n");

		string tmpDir = VuoFileUtilities::makeTmpDir(nodeClassName);

		replaceGenericTypesWithSpecialized(genericImplementation, specializedForGenericTypeName);

		string tmpNodeClassImplementationFile = tmpDir + "/" + nodeClassName + ".c";
		string tmpNodeClassCompiledFile = tmpDir + "/" + nodeClassName + ".vuonode";
		VuoFileUtilities::preserveOriginalFileName(genericImplementation, genericNodeClassName + ".c");
		VuoFileUtilities::writeStringToFile(genericImplementation, tmpNodeClassImplementationFile);

		set<VuoNodeSet *> nodeSetDependencies;
		VuoNodeSet *genericNodeSet = genericNodeClass->getBase()->getNodeSet();
		if (genericNodeSet)
		{
			nodeSetDependencies.insert(genericNodeSet);
		}
		for (vector<string>::iterator i = specializedTypeNames.begin(); i != specializedTypeNames.end(); ++i)
		{
			string specializedTypeName = *i;
			if (VuoGenericType::isGenericTypeName(specializedTypeName))
				continue;

			string innermostTypeName = VuoType::extractInnermostTypeName(specializedTypeName);
			VuoNodeSet *typeNodeSet = compiler->getType(innermostTypeName)->getBase()->getNodeSet();
			if (typeNodeSet)
				nodeSetDependencies.insert(typeNodeSet);
		}
		set<string> tmpHeaders;
		for (set<VuoNodeSet *>::iterator i = nodeSetDependencies.begin(); i != nodeSetDependencies.end(); ++i)
		{
			VuoNodeSet *nodeSet = *i;
			vector<string> headerFileNames = nodeSet->getHeaderFileNames();
			for (vector<string>::iterator j = headerFileNames.begin(); j != headerFileNames.end(); ++j)
			{
				string headerFileName = *j;
				string header = nodeSet->getHeaderContents( headerFileName );
				string tmpHeaderFile = tmpDir + "/" + headerFileName;
				VuoFileUtilities::writeStringToFile(header, tmpHeaderFile);
				tmpHeaders.insert(tmpHeaderFile);
			}
		}
		vector<string> includeDirs;
		includeDirs.push_back(tmpDir);

		compiler->compileModule(tmpNodeClassImplementationFile, tmpNodeClassCompiledFile, includeDirs);
		Module *module = compiler->readModuleFromBitcode(tmpNodeClassCompiledFile, compiler->getArch());

		for (set<string>::iterator i = tmpHeaders.begin(); i != tmpHeaders.end(); ++i)
			remove((*i).c_str());
		remove(tmpNodeClassImplementationFile.c_str());
		remove(tmpNodeClassCompiledFile.c_str());
		remove(tmpDir.c_str());


		// Construct the VuoCompilerSpecializedNodeClass

		dispatch_sync(llvmQueue, ^{
						  VuoCompilerSpecializedNodeClass *dummyNodeClass = new VuoCompilerSpecializedNodeClass(nodeClassName, module);
						  nodeClass = new VuoCompilerSpecializedNodeClass(dummyNodeClass);
						  delete dummyNodeClass;
					  });
	}
	else
	{
		// Construct a node class that doesn't yet have an implementation, using the generic node class as the model.

		vector<VuoPortClass *> modelInputPortClasses = genericNodeClass->getBase()->getInputPortClasses();
		vector<VuoPortClass *> inputPortClasses;
		for (VuoPortClass *modelPortClass : modelInputPortClasses)
		{
			VuoPortClass *portClass = copyPortClassFromModel(modelPortClass, true);
			inputPortClasses.push_back(portClass);
		}

		vector<VuoPortClass *> modelOutputPortClasses = genericNodeClass->getBase()->getOutputPortClasses();
		vector<VuoPortClass *> outputPortClasses;
		for (VuoPortClass *modelPortClass : modelOutputPortClasses)
		{
			VuoPortClass *portClass = copyPortClassFromModel(modelPortClass, false);
			outputPortClasses.push_back(portClass);
		}

		vector<VuoPortClass *> modelPortClasses;
		modelPortClasses.insert(modelPortClasses.end(), modelInputPortClasses.begin(), modelInputPortClasses.end());
		modelPortClasses.insert(modelPortClasses.end(), modelOutputPortClasses.begin(), modelOutputPortClasses.end());
		vector<VuoPortClass *> portClasses;
		portClasses.insert(portClasses.end(), inputPortClasses.begin(), inputPortClasses.end());
		portClasses.insert(portClasses.end(), outputPortClasses.begin(), outputPortClasses.end());
		for (size_t i = 0; i < modelPortClasses.size(); ++i)
		{
			VuoCompilerPortClass *modelPortClass = static_cast<VuoCompilerPortClass *>( modelPortClasses[i]->getCompiler() );
			VuoCompilerPortClass *portClass = static_cast<VuoCompilerPortClass *>( portClasses[i]->getCompiler() );
			VuoType *modelPortType = modelPortClass->getDataVuoType();
			if (modelPortType)
			{
				string modelPortTypeName = modelPortType->getModuleKey();
				string innermostModelPortTypeName = VuoType::extractInnermostTypeName(modelPortTypeName);
				map<string, string>::iterator specializedTypeNameIter = specializedForGenericTypeName.find(innermostModelPortTypeName);
				if (specializedTypeNameIter != specializedForGenericTypeName.end())
				{
					string innermostSpecializedTypeName = specializedTypeNameIter->second;
					if (innermostSpecializedTypeName != innermostModelPortTypeName)
					{
						string specializedTypeName = VuoGenericType::replaceInnermostGenericTypeName(modelPortTypeName, innermostSpecializedTypeName);
						VuoType *portType = compiler->getType(specializedTypeName)->getBase();
						portClass->setDataVuoType(portType);
					}
				}
			}
		}

		VuoPortClass *refreshPortClass = inputPortClasses.front();

		VuoNodeClass *baseNodeClass = new VuoNodeClass(nodeClassName, refreshPortClass, inputPortClasses, outputPortClasses);
		nodeClass = new VuoCompilerSpecializedNodeClass(baseNodeClass);

		baseNodeClass->setDefaultTitle(genericNodeClass->getBase()->getDefaultTitle());
		baseNodeClass->setDescription(genericNodeClass->getBase()->getDescription());
		baseNodeClass->setVersion(genericNodeClass->getBase()->getVersion());
		baseNodeClass->setKeywords(genericNodeClass->getBase()->getKeywords());
		baseNodeClass->setNodeSet(genericNodeClass->getBase()->getNodeSet());
		baseNodeClass->setExampleCompositionFileNames(genericNodeClass->getBase()->getExampleCompositionFileNames());
		baseNodeClass->setDeprecated(genericNodeClass->getBase()->getDeprecated());

		for (vector<string>::iterator i = genericTypeNames.begin(); i != genericTypeNames.end(); ++i)
		{
			string genericTypeName = *i;
			string defaultSpecializedTypeName = genericNodeClass->getDefaultSpecializedTypeName(*i);
			if (! defaultSpecializedTypeName.empty())
				nodeClass->defaultSpecializedForGenericTypeName[genericTypeName] = defaultSpecializedTypeName;
		}

		// The compiler will assign a fully-specialized backing node class with updateBackingNodeClass(), but
		// the renderer may need a backing node class before then, e.g. to know if the node is stateful.
		nodeClass->backingNodeClass = genericNodeClass;
	}

	nodeClass->genericNodeClass = genericNodeClass;
	nodeClass->specializedForGenericTypeName = specializedForGenericTypeName;

	nodeClass->setBuiltIn( genericNodeClass->isBuiltIn() );

	return nodeClass->getBase();
}

/**
 * Creates a new port class (base + compiler detail) with properties copied from @a modelPortClass (base + compiler detail).
 */
VuoPortClass * VuoCompilerSpecializedNodeClass::copyPortClassFromModel(VuoPortClass *modelPortClass, bool isInput)
{
	string name = modelPortClass->getName();
	VuoPortClass::PortType portType = modelPortClass->getPortType();

	VuoCompilerPortClass *modelCompilerPortClass = static_cast<VuoCompilerPortClass *>(modelPortClass->getCompiler());
	VuoType *dataType = modelCompilerPortClass->getDataVuoType();

	VuoCompilerPortClass *portClass = nullptr;
	if (isInput)
	{
		VuoCompilerInputEventPortClass *inputPortClass = new VuoCompilerInputEventPortClass(name);
		portClass = inputPortClass;

		if (dataType)
		{
			VuoCompilerInputDataClass *inputDataClass = new VuoCompilerInputDataClass("");
			inputPortClass->setDataClass(inputDataClass);
		}
	}
	else
	{
		if (portType == VuoPortClass::eventOnlyPort || portType == VuoPortClass::dataAndEventPort)
		{
			VuoCompilerOutputEventPortClass *outputPortClass = new VuoCompilerOutputEventPortClass(name);
			portClass = outputPortClass;

			if (dataType)
			{
				VuoCompilerOutputDataClass *outputDataClass = new VuoCompilerOutputDataClass("");
				outputPortClass->setDataClass(outputDataClass);
			}
		}
		else if (portType == VuoPortClass::triggerPort)
		{
			VuoCompilerTriggerPortClass *triggerPortClass = new VuoCompilerTriggerPortClass(name);
			portClass = triggerPortClass;
		}
	}

	if ((portType == VuoPortClass::eventOnlyPort || portType == VuoPortClass::dataAndEventPort) && dataType)
	{
		VuoCompilerEventPortClass *modelEventPortClass = static_cast<VuoCompilerEventPortClass *>(modelCompilerPortClass);
		VuoCompilerEventPortClass *eventPortClass = static_cast<VuoCompilerEventPortClass *>(portClass);
		json_object *dataDetails = modelEventPortClass->getDataClass()->getDetails();
		eventPortClass->getDataClass()->setDetails(dataDetails);
		json_object_get(dataDetails);
	}

	if (dataType)
		portClass->setDataVuoType(dataType);

	json_object *details = modelCompilerPortClass->getDetails();
	portClass->setDetails(details);
	json_object_get(details);

	portClass->getBase()->setEventBlocking(modelPortClass->getEventBlocking());
	portClass->getBase()->setPortAction(modelPortClass->hasPortAction());
	portClass->getBase()->setDefaultEventThrottling(modelPortClass->getDefaultEventThrottling());

	return portClass->getBase();
}

/**
 * Returns the node class to use for instantiating a node within a composition.
 *
 *    - If @a origNodeClass doesn't have any generic types, then @a origNodeClass itself is returned.
 *    - If @a origNodeClass is a partially-specialized generic node class (e.g. `vuo.osc.message.get.2.VuoReal.VuoGenericType2`),
 *			then a new VuoCompilerSpecializedNodeClass is returned, to be unique to the created node.
 *    - If @a origNodeClass is a generic node class that the compiler loaded from the modules folders
 *			(e.g. `vuo.osc.message.get.2`), then a new or existing VuoCompilerSpecializedNodeClass is returned,
 *			depending on which of the two above cases applies to the node class after it has been specialized
 *			with its default types, if any.
 */
VuoCompilerNodeClass * VuoCompilerSpecializedNodeClass::getNodeClassForNode(VuoCompilerNodeClass *origNodeClass, VuoCompiler *compiler)
{
	vector<string> genericTypes = getGenericTypeNamesFromPorts(origNodeClass);
	if (! genericTypes.empty())
	{
		VuoCompilerSpecializedNodeClass *specializedNodeClass = dynamic_cast<VuoCompilerSpecializedNodeClass *>(origNodeClass);
		if (specializedNodeClass)
		{
			return newNodeClass(origNodeClass->getBase()->getClassName(), compiler, NULL)->getCompiler();
		}
		else
		{
			string rawGenericNodeClassName = origNodeClass->getBase()->getClassName();
			string rawGenericNodeClassNameWithSuffixes = createSpecializedNodeClassName(rawGenericNodeClassName, genericTypes);
			VuoCompilerNodeClass *rawGenericNodeClassUncasted = newNodeClass(rawGenericNodeClassNameWithSuffixes, compiler, NULL)->getCompiler();
			VuoCompilerSpecializedNodeClass *rawGenericNodeClass = static_cast<VuoCompilerSpecializedNodeClass *>(rawGenericNodeClassUncasted);
			string genericNodeClassName = rawGenericNodeClass->createDefaultSpecializedNodeClassName();
			return compiler->getNodeClass(genericNodeClassName);
		}
	}

	return origNodeClass;
}

/**
 * Returns true if @a potentialSpecializedNodeClassName is one of the possible specializations of @a potentialGenericNodeClass.
 */
bool VuoCompilerSpecializedNodeClass::isSpecializationOfNodeClass(const string &potentialSpecializedNodeClassName,
																  VuoCompilerNodeClass *potentialGenericNodeClass)
{
	vector<string> genericTypesFromGeneric = getGenericTypeNamesFromPorts(potentialGenericNodeClass);
	if (genericTypesFromGeneric.empty())
		return false;

	string genericNodeClassNameFromSpecialized = extractGenericNodeClassName(potentialSpecializedNodeClassName, genericTypesFromGeneric.size());
	return genericNodeClassNameFromSpecialized == potentialGenericNodeClass->getBase()->getClassName();
}

/**
 * Updates the implementation for this VuoCompilerSpecializedNodeClass to be consistent with the backing types
 * of @a nodeToBack, and updates the VuoCompilerNode of @a nodeToBack to be consistent with that implementation.
 */
void VuoCompilerSpecializedNodeClass::updateBackingNodeClass(VuoNode *nodeToBack, VuoCompiler *compiler)
{
	string backingNodeClassName = createFullySpecializedNodeClassName(nodeToBack);
	if (backingNodeClassName == getBase()->getClassName() ||
			(backingNodeClass && backingNodeClassName == backingNodeClass->getBase()->getClassName()))
		return;

	VuoCompilerNode *replacementCompilerNode = createReplacementBackingNode(nodeToBack, backingNodeClassName, compiler);
	replacementCompilerNode->setGraphvizIdentifier( nodeToBack->getCompiler()->getGraphvizIdentifier() );

	backingNodeClass = replacementCompilerNode->getBase()->getNodeClass()->getCompiler();

	// Transfer each VuoCompilerPort from the replacement to nodeToBack.
	vector<VuoPort *> inputPortsOnReplacementNode = replacementCompilerNode->getBase()->getInputPorts();
	vector<VuoPort *> inputPortsOnNodeToBack = nodeToBack->getInputPorts();
	vector<VuoPort *> outputPortsOnReplacementNode = replacementCompilerNode->getBase()->getOutputPorts();
	vector<VuoPort *> outputPortsOnNodeToBack = nodeToBack->getOutputPorts();
	vector<VuoPort *> portsOnReplacementNode;
	vector<VuoPort *> portsOnNodeToBack;
	portsOnReplacementNode.insert(portsOnReplacementNode.end(), inputPortsOnReplacementNode.begin(), inputPortsOnReplacementNode.end());
	portsOnReplacementNode.insert(portsOnReplacementNode.end(), outputPortsOnReplacementNode.begin(), outputPortsOnReplacementNode.end());
	portsOnNodeToBack.insert(portsOnNodeToBack.end(), inputPortsOnNodeToBack.begin(), inputPortsOnNodeToBack.end());
	portsOnNodeToBack.insert(portsOnNodeToBack.end(), outputPortsOnNodeToBack.begin(), outputPortsOnNodeToBack.end());
	for (int i = 0; i < inputPortsOnReplacementNode.size(); ++i)
	{
		VuoCompilerInputEventPort *port = dynamic_cast<VuoCompilerInputEventPort *>(inputPortsOnNodeToBack[i]->getCompiler());
		VuoCompilerInputEventPort *replacementPort = dynamic_cast<VuoCompilerInputEventPort *>(inputPortsOnReplacementNode[i]->getCompiler());
		if (port->getData())
			replacementPort->getData()->setInitialValue( port->getData()->getInitialValue() );
	}
	for (int i = 0; i < portsOnReplacementNode.size(); ++i)
	{
		VuoPort *baseToKeep = portsOnNodeToBack[i];
		VuoCompilerNodeArgument *compilerPortToKeep = portsOnReplacementNode[i]->getCompiler();
		VuoCompilerNodeArgumentClass *compilerPortClassToKeep = portsOnReplacementNode[i]->getClass()->getCompiler();
		baseToKeep->setCompiler(compilerPortToKeep);
		baseToKeep->getClass()->setCompiler(compilerPortClassToKeep);
		compilerPortToKeep->setBase(baseToKeep);
		compilerPortClassToKeep->setBase(baseToKeep->getClass());
	}

	// Transfer the VuoCompilerNode from the replacement to nodeToBack.
	VuoNode *baseToDiscard = replacementCompilerNode->getBase();
	VuoCompilerNode *compilerToDiscard = nodeToBack->getCompiler();
	nodeToBack->setCompiler(replacementCompilerNode);
	replacementCompilerNode->setBase(nodeToBack);
	delete baseToDiscard;
	delete compilerToDiscard;
}

/**
 * Returns a fully specialized node of class @a backingNodeClassName that will replace the node class of @a nodeToBack.
 */
VuoCompilerNode * VuoCompilerSpecializedNodeClass::createReplacementBackingNode(VuoNode *nodeToBack, string backingNodeClassName, VuoCompiler *compiler)
{
	VuoCompilerNodeClass *nodeClass = compiler->getNodeClass(backingNodeClassName);
	if (!nodeClass)
		throw VuoCompilerException(VuoCompilerIssue(VuoCompilerIssue::Error, "specializing node in composition", "",
													"Couldn't get backing node class.",
													"'" + backingNodeClassName + "' uses generic types, but it couldn't be compiled. "
													"Check the macOS Console for details."));
	return nodeClass->newNode(nodeToBack)->getCompiler();
}

/**
 * Replaces all occurrences of the given generic type names in the node class source code with their corresponding
 * specialized type names.
 */
void VuoCompilerSpecializedNodeClass::replaceGenericTypesWithSpecialized(string &nodeClassSource, map<string, string> specializedForGenericTypeName)
{
	set<string> replacementTypeNames;
	for (map<string, string>::iterator i = specializedForGenericTypeName.begin(); i != specializedForGenericTypeName.end(); ++i)
	{
		string genericTypeName = i->first;
		string replacementTypeName = i->second;

		if (replacementTypeName != genericTypeName)
		{
			// Replace each occurrence of the generic type name with the specialized or backing type name.
			VuoStringUtilities::replaceAll(nodeClassSource, genericTypeName, replacementTypeName);

			string innermostTypeName = VuoType::extractInnermostTypeName(replacementTypeName);
			replacementTypeNames.insert(innermostTypeName);
		}
	}

	string includesToAdd;
	for (set<string>::iterator i = replacementTypeNames.begin(); i != replacementTypeNames.end(); ++i)
	{
		string replacementTypeName = *i;
		includesToAdd += "#include \"" + replacementTypeName + ".h\"\n";

		if (nodeClassSource.find(VuoType::listTypeNamePrefix + replacementTypeName) != string::npos)
			includesToAdd += "#include \"" + VuoType::listTypeNamePrefix + replacementTypeName + ".h\"\n";
	}

	size_t insertionPos = nodeClassSource.find("VuoModuleMetadata");  // assumed to be after '#include "node.h"' and before any generic type names
	if (insertionPos == string::npos)
		return;
	nodeClassSource.insert(insertionPos, includesToAdd);
}

/**
 * Replaces all occurrences of generic type names in the node class source code with a default actual type name.
 */
void VuoCompilerSpecializedNodeClass::replaceGenericTypesWithBacking(string &nodeClassSource)
{
	size_t insertionPos = nodeClassSource.find("VuoModuleMetadata");  // assumed to be after '#include "node.h"' and before any generic type identifiers
	if (insertionPos == string::npos)
		return;

	map<string, vector<string> > compatibleTypesForGeneric;

	// Figure out the backing types by parsing the "genericTypes" metadata from nodeClassSource.
	size_t metadataStartPos = nodeClassSource.find("(", insertionPos) + 1;
	if (metadataStartPos == string::npos)
		return;
	size_t metadataEndPos = nodeClassSource.find(");\n", metadataStartPos);
	if (metadataEndPos == string::npos)
		return;
	string moduleMetadataString = nodeClassSource.substr(metadataStartPos, metadataEndPos - metadataStartPos);
	json_object *moduleMetadata = json_tokener_parse(moduleMetadataString.c_str());
	if (! moduleMetadata)
		return;
	map<string, string> defaultTypeForGeneric;
	VuoCompilerNodeClass::parseGenericTypes(moduleMetadata, defaultTypeForGeneric, compatibleTypesForGeneric);
	json_object_put(moduleMetadata);

	set<string> genericTypeIdentifiers;
	size_t startPos = 0;
	string genericTypeName;
	while ((startPos = VuoGenericType::findGenericTypeName(nodeClassSource, startPos, genericTypeName)) != string::npos)
	{
		size_t endPos = startPos + genericTypeName.length();
		if ((startPos == 0 || ! VuoStringUtilities::isValidCharInIdentifier( nodeClassSource[startPos-1] )) &&
				(endPos == nodeClassSource.length() - 1 || ! VuoStringUtilities::isValidCharInIdentifier( nodeClassSource[endPos] )))
		{
			// Found a generic type identifier, so prepare to add a typedef.
			genericTypeIdentifiers.insert(genericTypeName);
			startPos += genericTypeName.length();
		}
		else
		{
			// Found a generic type name within another identifier, so replace it with its backing type name.
			string innermostTypeName = VuoType::extractInnermostTypeName(genericTypeName);
			string innermostBackingTypeName = VuoCompilerGenericType::chooseBackingTypeName(genericTypeName,
																							compatibleTypesForGeneric[innermostTypeName]);
			string backingTypeName = VuoGenericType::replaceInnermostGenericTypeName(genericTypeName, innermostBackingTypeName);
			nodeClassSource.replace(startPos, genericTypeName.length(), backingTypeName);
			startPos += backingTypeName.length();
		}
	}

	// Add a typedef to replace each generic type with its backing type.
	string typedefsToAdd;
	map<string, bool> typeNamesSeen;
	for (set<string>::iterator i = genericTypeIdentifiers.begin(); i != genericTypeIdentifiers.end(); ++i)
	{
		string genericTypeName = *i;

		string innermostTypeName = VuoType::extractInnermostTypeName(genericTypeName);
		string innermostBackingTypeName = VuoCompilerGenericType::chooseBackingTypeName(innermostTypeName,
																						compatibleTypesForGeneric[innermostTypeName]);
		if (! typeNamesSeen[innermostTypeName])
		{
			typeNamesSeen[innermostTypeName] = true;

			string replacementTypeName = innermostBackingTypeName;
			typedefsToAdd += "typedef " + replacementTypeName + " " + innermostTypeName + ";\n";
		}

		if (! typeNamesSeen[genericTypeName])
		{
			typeNamesSeen[genericTypeName] = true;

			string replacementCollectionTypeName = VuoGenericType::replaceInnermostGenericTypeName(genericTypeName, innermostBackingTypeName);
			typedefsToAdd += "typedef " + replacementCollectionTypeName + " " + genericTypeName + ";\n";
		}
	}
	nodeClassSource.insert(insertionPos, typedefsToAdd);
}

/**
 * Returns true if this node class has no unspecialized generic types.
 */
bool VuoCompilerSpecializedNodeClass::isFullySpecialized(void)
{
	return getGenericTypeNamesFromPorts(this).empty();
}

/**
 * Returns true if the node class has at least one unspecialized generic type.
 */
bool VuoCompilerSpecializedNodeClass::hasGenericPortTypes(VuoCompilerNodeClass *nodeClass)
{
	return ! getGenericTypeNamesFromPorts(nodeClass).empty();
}

/**
 * Returns the generic type names used by this node class's ports, in numerical order of their suffixes.
 */
vector<string> VuoCompilerSpecializedNodeClass::getGenericTypeNamesFromPorts(VuoCompilerNodeClass *nodeClass)
{
	set<string> genericTypeNames;
	vector<VuoPortClass *> inputPortClasses = nodeClass->getBase()->getInputPortClasses();
	vector<VuoPortClass *> outputPortClasses = nodeClass->getBase()->getOutputPortClasses();
	vector<VuoPortClass *> portClasses;
	portClasses.insert(portClasses.end(), inputPortClasses.begin(), inputPortClasses.end());
	portClasses.insert(portClasses.end(), outputPortClasses.begin(), outputPortClasses.end());
	for (vector<VuoPortClass *>::iterator i = portClasses.begin(); i != portClasses.end(); ++i)
	{
		VuoCompilerPortClass *portClass = static_cast<VuoCompilerPortClass *>((*i)->getCompiler());
		VuoGenericType *genericType = dynamic_cast<VuoGenericType *>(portClass->getDataVuoType());
		if (genericType)
		{
			string innermostTypeName = VuoType::extractInnermostTypeName(genericType->getModuleKey());
			genericTypeNames.insert(innermostTypeName);
		}
	}

	vector<string> sortedGenericTypeNames(genericTypeNames.begin(), genericTypeNames.end());
	VuoGenericType::sortGenericTypeNames(sortedGenericTypeNames);
	return sortedGenericTypeNames;
}

/**
 * Returns the backing type for each generic type used by the given node's ports.
 *
 * The returned generic type names are those on the node class, not those that have been assigned
 * to the node within its composition.
 */
map<string, string> VuoCompilerSpecializedNodeClass::getBackingTypeNamesFromPorts(VuoNode *node)
{
	map<string, string> backingTypeForPort;

	vector<VuoPort *> inputPorts = node->getInputPorts();
	vector<VuoPort *> outputPorts = node->getOutputPorts();
	vector<VuoPort *> ports;
	ports.insert(ports.end(), inputPorts.begin(), inputPorts.end());
	ports.insert(ports.end(), outputPorts.begin(), outputPorts.end());
	for (vector<VuoPort *>::iterator i = ports.begin(); i != ports.end(); ++i)
	{
		VuoCompilerPort *port = static_cast<VuoCompilerPort *>((*i)->getCompiler());
		VuoType *type = port->getDataVuoType();
		if (! type || ! type->hasCompiler())
			continue;

		VuoCompilerGenericType *genericTypeForPort = dynamic_cast<VuoCompilerGenericType *>(type->getCompiler());
		if (! genericTypeForPort)
			continue;
		string backingTypeName = genericTypeForPort->getBackingTypeName();

		VuoCompilerPortClass *portClass = static_cast<VuoCompilerPortClass *>((*i)->getClass()->getCompiler());
		string genericTypeNameForPortClass = portClass->getDataVuoType()->getModuleKey();

		string innermostGenericTypeName = VuoType::extractInnermostTypeName( genericTypeNameForPortClass );
		string innermostBackingTypeName = VuoType::extractInnermostTypeName( backingTypeName );
		backingTypeForPort[innermostGenericTypeName] = innermostBackingTypeName;
	}

	return backingTypeForPort;
}

/**
 * Returns this port's type in the generic node that this specialized node class was derived from.
 */
VuoType * VuoCompilerSpecializedNodeClass::getOriginalPortType(VuoPortClass *portClass)
{
	int i = 0;
	for (auto specializedPortClass : getBase()->getInputPortClasses())
	{
		if (specializedPortClass == portClass)
			return static_cast<VuoCompilerPortClass *>(genericNodeClass->getBase()->getInputPortClasses()[i]->getCompiler())->getDataVuoType();
		++i;
	}

	i = 0;
	for (auto specializedPortClass : getBase()->getOutputPortClasses())
	{
		if (specializedPortClass == portClass)
			return static_cast<VuoCompilerPortClass *>(genericNodeClass->getBase()->getOutputPortClasses()[i]->getCompiler())->getDataVuoType();
		++i;
	}

	return nullptr;
}

/**
 * Returns the original node's class name (without any type suffixes).
 */
string VuoCompilerSpecializedNodeClass::getOriginalGenericNodeClassName(void)
{
	return genericNodeClass->getBase()->getModuleKey();
}

/**
 * Returns the original node's description.
 */
string VuoCompilerSpecializedNodeClass::getOriginalGenericNodeClassDescription(void)
{
	return genericNodeClass->getBase()->getDescription();
}

/**
 * Returns the original node's node set.
 */
VuoNodeSet * VuoCompilerSpecializedNodeClass::getOriginalGenericNodeSet(void)
{
	return genericNodeClass->getBase()->getNodeSet();
}

/**
 * Returns the name for the node class that would result if the given port were changed back to its original type.
 */
string VuoCompilerSpecializedNodeClass::createUnspecializedNodeClassName(set<VuoPortClass *> portClassesToUnspecialize)
{
	set<string> genericTypeNames;
	for (set<VuoPortClass *>::iterator i = portClassesToUnspecialize.begin(); i != portClassesToUnspecialize.end(); ++i)
	{
		VuoPortClass *portClassToUnspecialize = *i;
		VuoGenericType *genericType = dynamic_cast<VuoGenericType *>( getOriginalPortType(portClassToUnspecialize) );
		if (genericType)
		{
			string innermostGenericTypeName = VuoType::extractInnermostTypeName(genericType->getModuleKey());
			genericTypeNames.insert(innermostGenericTypeName);
		}
	}

	vector<string> specializedTypeNames;
	vector<string> origGenericTypeNames = getGenericTypeNamesFromPorts(genericNodeClass);
	for (vector<string>::iterator i = origGenericTypeNames.begin(); i != origGenericTypeNames.end(); ++i)
	{
		string g = *i;
		string s = (genericTypeNames.find(g) != genericTypeNames.end() ? g : specializedForGenericTypeName[g]);
		specializedTypeNames.push_back(s);
	}

	string genericNodeClassName = extractGenericNodeClassName(getBase()->getClassName(), origGenericTypeNames.size());
	return createSpecializedNodeClassName(genericNodeClassName, specializedTypeNames);
}

/**
 * Returns the name for the node class that would result if the given (innermost) specialized type were
 * substituted for the given (innermost) generic type in this node class.
 */
string VuoCompilerSpecializedNodeClass::createSpecializedNodeClassNameWithReplacement(string genericTypeName, string specializedTypeName)
{
	vector<string> specializedTypeNames;
	vector<string> origGenericTypeNames = getGenericTypeNamesFromPorts(genericNodeClass);
	for (vector<string>::iterator i = origGenericTypeNames.begin(); i != origGenericTypeNames.end(); ++i)
	{
		string g = *i;
		string s = (g == genericTypeName ? specializedTypeName : specializedForGenericTypeName[g]);
		specializedTypeNames.push_back(s);
	}

	string genericNodeClassName = extractGenericNodeClassName(getBase()->getClassName(), origGenericTypeNames.size());
	return createSpecializedNodeClassName(genericNodeClassName, specializedTypeNames);
}

/**
 * Returns the name for the node class that would result if all of this node class's generic types that
 * have default specialized types (as defined in the node class metadata) were replaced with those types.
 */
string VuoCompilerSpecializedNodeClass::createDefaultSpecializedNodeClassName(void)
{
	vector<string> sortedGenericTypeNames = getGenericTypeNamesFromPorts(this);
	vector<string> sortedSpecializedTypeNames;
	for (vector<string>::iterator i = sortedGenericTypeNames.begin(); i != sortedGenericTypeNames.end(); ++i)
	{
		string genericTypeName = *i;
		map<string, string>::iterator defaultTypeNameIter = defaultSpecializedForGenericTypeName.find(genericTypeName);
		string specializedTypeName = (defaultTypeNameIter != defaultSpecializedForGenericTypeName.end() ?
																 defaultTypeNameIter->second : genericTypeName);
		sortedSpecializedTypeNames.push_back(specializedTypeName);
	}

	string genericNodeClassName = extractGenericNodeClassName(getBase()->getClassName(), sortedGenericTypeNames.size());
	return createSpecializedNodeClassName(genericNodeClassName, sortedSpecializedTypeNames);
}

/**
 * Returns the name for the node class that would result if all of this node class's generic types
 * were specialized with the backing types from @a nodeToBack.
 */
string VuoCompilerSpecializedNodeClass::createFullySpecializedNodeClassName(VuoNode *nodeToBack)
{
	map<string, string> backingForGeneric = getBackingTypeNamesFromPorts(nodeToBack);

	vector<string> nodeClassNameParts = VuoStringUtilities::split(getBase()->getClassName(), '.');
	for (int i = 1; i < nodeClassNameParts.size(); ++i)
	{
		map<string, string>::iterator backingIter = backingForGeneric.find( VuoType::extractInnermostTypeName(nodeClassNameParts[i]) );
		if (backingIter != backingForGeneric.end())
			nodeClassNameParts[i] = VuoGenericType::replaceInnermostGenericTypeName(nodeClassNameParts[i], backingIter->second);
	}

	return VuoStringUtilities::join(nodeClassNameParts, '.');
}

/**
 * Extracts the generic node class name from the specialized node class name by removing type names from the end.
 */
string VuoCompilerSpecializedNodeClass::extractGenericNodeClassName(string specializedNodeClassName, size_t genericTypeCount)
{
	vector<string> specializedNameParts = VuoStringUtilities::split(specializedNodeClassName, '.');
	if (specializedNameParts.size() < 2 + genericTypeCount)
		return "";

	vector<string> genericNameParts(specializedNameParts.begin(), specializedNameParts.begin() + specializedNameParts.size() - genericTypeCount);
	return VuoStringUtilities::join(genericNameParts, '.');
}

/**
 * Creates a specialized node class name by appending type names to the generic node class name.
 */
string VuoCompilerSpecializedNodeClass::createSpecializedNodeClassName(string genericNodeClassName, vector<string> specializedTypeNames)
{
	return genericNodeClassName + '.' + VuoStringUtilities::join(specializedTypeNames, '.');
}

/// @{
/**
 * If the backing node class exists, this function is performed on the backing node class instead of this node class.
 */
string VuoCompilerSpecializedNodeClass::getClassIdentifier(void)
{ return backingNodeClass ? backingNodeClass->getClassIdentifier() : VuoCompilerNodeClass::getClassIdentifier(); }

Function * VuoCompilerSpecializedNodeClass::getEventFunction(void)
{ return backingNodeClass ? backingNodeClass->getEventFunction() : VuoCompilerNodeClass::getEventFunction(); }

Function * VuoCompilerSpecializedNodeClass::getInitFunction(void)
{ return backingNodeClass ? backingNodeClass->getInitFunction() : VuoCompilerNodeClass::getInitFunction(); }

Function * VuoCompilerSpecializedNodeClass::getFiniFunction(void)
{ return backingNodeClass ? backingNodeClass->getFiniFunction() : VuoCompilerNodeClass::getFiniFunction(); }

Function * VuoCompilerSpecializedNodeClass::getCallbackStartFunction(void)
{ return backingNodeClass ? backingNodeClass->getCallbackStartFunction() : VuoCompilerNodeClass::getCallbackStartFunction(); }

Function * VuoCompilerSpecializedNodeClass::getCallbackUpdateFunction(void)
{ return backingNodeClass ? backingNodeClass->getCallbackUpdateFunction() : VuoCompilerNodeClass::getCallbackUpdateFunction(); }

Function * VuoCompilerSpecializedNodeClass::getCallbackStopFunction(void)
{ return backingNodeClass ? backingNodeClass->getCallbackStopFunction() : VuoCompilerNodeClass::getCallbackStopFunction(); }

Function * VuoCompilerSpecializedNodeClass::getCompositionAddNodeMetadataFunction(void)
{ return backingNodeClass ? backingNodeClass->getCompositionAddNodeMetadataFunction() : VuoCompilerNodeClass::getCompositionAddNodeMetadataFunction(); }

Function * VuoCompilerSpecializedNodeClass::getCompositionPerformDataOnlyTransmissionsFunction(void)
{ return backingNodeClass ? backingNodeClass->getCompositionPerformDataOnlyTransmissionsFunction() : VuoCompilerNodeClass::getCompositionPerformDataOnlyTransmissionsFunction(); }

Function * VuoCompilerSpecializedNodeClass::getCompositionSetPublishedInputPortValueFunction(void)
{ return backingNodeClass ? backingNodeClass->getCompositionSetPublishedInputPortValueFunction() : VuoCompilerNodeClass::getCompositionSetPublishedInputPortValueFunction(); }

Function * VuoCompilerSpecializedNodeClass::getTriggerWorkerFunction(string portIdentifier)
{ return backingNodeClass ? backingNodeClass->getTriggerWorkerFunction(portIdentifier) : VuoCompilerNodeClass::getTriggerWorkerFunction(portIdentifier); }

vector<VuoCompilerTriggerDescription *> VuoCompilerSpecializedNodeClass::getTriggerDescriptions(void)
{ return backingNodeClass ? backingNodeClass->getTriggerDescriptions() : VuoCompilerNodeClass::getTriggerDescriptions(); }

VuoCompilerInstanceDataClass * VuoCompilerSpecializedNodeClass::getInstanceDataClass(void)
{ return backingNodeClass ? backingNodeClass->getInstanceDataClass() : VuoCompilerNodeClass::getInstanceDataClass(); }

string VuoCompilerSpecializedNodeClass::getDoxygenDocumentation(void)
{ return backingNodeClass ? backingNodeClass->getDoxygenDocumentation() : VuoCompilerNodeClass::getDoxygenDocumentation(); }

string VuoCompilerSpecializedNodeClass::getDefaultSpecializedTypeName(string genericTypeName)
{ return backingNodeClass ? backingNodeClass->getDefaultSpecializedTypeName(genericTypeName) : VuoCompilerNodeClass::getDefaultSpecializedTypeName(genericTypeName); }

vector<string> VuoCompilerSpecializedNodeClass::getAutomaticKeywords(void)
{ return backingNodeClass ? backingNodeClass->getAutomaticKeywords() : VuoCompilerNodeClass::getAutomaticKeywords(); }

string VuoCompilerSpecializedNodeClass::getDependencyName(void)
{ return backingNodeClass ? backingNodeClass->getDependencyName() : VuoCompilerNodeClass::getDependencyName(); }

bool VuoCompilerSpecializedNodeClass::isStateful(void)
{ return backingNodeClass ? backingNodeClass->isStateful() : VuoCompilerNodeClass::isStateful(); }
/// @}

/**
 * Returns the dependencies of the backing node class, if it exists, otherwise this node class.
 * The list of dependencies includes the generic node class that this node class specializes.
 */
set<string> VuoCompilerSpecializedNodeClass::getDependencies(void)
{
	set<string> deps = (backingNodeClass ? backingNodeClass->getDependencies() : VuoCompilerNodeClass::getDependencies());
	if (genericNodeClass)
		deps.insert( genericNodeClass->getBase()->getClassName() );
	return deps;
}
