/**
 * @file
 * VuoCompilerSpecializedNodeClass implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerSpecializedNodeClass.hh"
#include "VuoCompiler.hh"
#include "VuoCompilerGenericType.hh"
#include "VuoNodeSet.hh"
#include "VuoPort.hh"
#include "VuoStringUtilities.hh"
#include <sstream>


/**
 * Creates a specialized node class implementation from an LLVM module, and creates its corresponding base @c VuoNodeClass.
 */
VuoCompilerSpecializedNodeClass::VuoCompilerSpecializedNodeClass(string nodeClassName, Module *module) :
	VuoCompilerNodeClass(nodeClassName, module)
{
	genericNodeClass = NULL;

	// Add the backing type for each generic port type as a dependency.
	vector<VuoPortClass *> portClasses;
	vector<VuoPortClass *> inputPortClasses = getBase()->getInputPortClasses();
	vector<VuoPortClass *> outputPortClasses = getBase()->getOutputPortClasses();
	portClasses.insert(portClasses.end(), inputPortClasses.begin(), inputPortClasses.end());
	portClasses.insert(portClasses.end(), outputPortClasses.begin(), outputPortClasses.end());
	for (vector<VuoPortClass *>::iterator i = portClasses.begin(); i != portClasses.end(); ++i)
	{
		VuoCompilerPortClass *portClass = static_cast<VuoCompilerPortClass *>((*i)->getCompiler());
		VuoGenericType *genericType = dynamic_cast<VuoGenericType *>(portClass->getDataVuoType());
		if (genericType)
		{
			VuoGenericType::Compatibility compatibility;
			set<string> compatibleTypeNames = genericType->getCompatibleSpecializedTypes(compatibility);
			string backingTypeName = VuoCompilerGenericType::chooseBackingTypeName(genericType->getModuleKey(), compatibleTypeNames);
			dependencies.insert(backingTypeName);
			string innermostBackingTypeName = VuoType::extractInnermostTypeName(backingTypeName);
			dependencies.insert(innermostBackingTypeName);
		}
	}
}

/**
 * Creates a new compiler node class and creates a new base @c VuoNodeClass, both from @c compilerNodeClass.
 */
VuoCompilerSpecializedNodeClass::VuoCompilerSpecializedNodeClass(VuoCompilerSpecializedNodeClass *compilerNodeClass, VuoNode *nodeToBack) :
	VuoCompilerNodeClass(compilerNodeClass)
{
	genericNodeClass = NULL;

	// Copy the generic types (with their compatible types and backing types) from nodeToBack to this node class.
	if (nodeToBack)
	{
		vector<VuoPort *> inputPorts = nodeToBack->getInputPorts();
		for (vector<VuoPort *>::iterator i = inputPorts.begin(); i != inputPorts.end(); ++i)
		{
			VuoPort *portInNode = *i;
			VuoPortClass *portInNodeClass = getInputPortClassWithName( portInNode->getClass()->getName() );
			VuoCompilerPort *compilerPortInNode = dynamic_cast<VuoCompilerPort *>(portInNode->getCompiler());
			VuoCompilerPortClass *compilerPortInNodeClass = dynamic_cast<VuoCompilerPortClass *>(portInNodeClass->getCompiler());
			VuoType *type = compilerPortInNode->getDataVuoType();
			if (type)
				compilerPortInNodeClass->setDataVuoType(type);
		}
		vector<VuoPort *> outputPorts = nodeToBack->getOutputPorts();
		for (vector<VuoPort *>::iterator i = outputPorts.begin(); i != outputPorts.end(); ++i)
		{
			VuoPort *portInNode = *i;
			VuoPortClass *portInNodeClass = getOutputPortClassWithName( portInNode->getClass()->getName() );
			VuoCompilerPort *compilerPortInNode = dynamic_cast<VuoCompilerPort *>(portInNode->getCompiler());
			VuoCompilerPortClass *compilerPortInNodeClass = dynamic_cast<VuoCompilerPortClass *>(portInNodeClass->getCompiler());
			VuoType *type = compilerPortInNode->getDataVuoType();
			if (type)
				compilerPortInNodeClass->setDataVuoType(type);
		}
	}
}

/**
 * Generates a specalized node class from a generic node class.
 *
 * @param nodeClassName The name of the node class to generate. It should have the format
 *		"<generic node class name>.<type>.(...).<type>" (e.g. "vuo.dictionary.make.VuoText.VuoInteger").
 * @param compiler The compiler to use for looking up the generic node class and compiling the specialized node class.
 * @param nodeToBack Optionally, a 'Make List' node whose generic types should be used to determine this node class's backing types.
 * @return The generated node class, or null if the generic node class is not found.
 */
VuoNodeClass * VuoCompilerSpecializedNodeClass::newNodeClass(string nodeClassName, VuoCompiler *compiler, VuoNode *nodeToBack)
{
	// Find the generic node class that the given node class should specialize

	VuoCompilerNodeClass *genericNodeClass = NULL;
	vector<string> genericTypeNames;

	map<string, VuoCompilerNodeClass *> nodeClasses = compiler->getNodeClasses();
	for (map<string, VuoCompilerNodeClass *>::const_iterator i = nodeClasses.begin(); i != nodeClasses.end(); ++i)
	{
		VuoCompilerNodeClass *potentialGenericNodeClass = i->second;

		if (! VuoStringUtilities::beginsWith(nodeClassName, potentialGenericNodeClass->getBase()->getClassName()))
			continue;

		vector<string> potentialGenericTypeNames = getGenericTypeNamesFromPorts(potentialGenericNodeClass);
		string expectedGenericNodeClassName = extractGenericNodeClassName(nodeClassName, potentialGenericTypeNames.size());

		if (expectedGenericNodeClassName == potentialGenericNodeClass->getBase()->getClassName())
		{
			genericNodeClass = potentialGenericNodeClass;
			genericTypeNames = potentialGenericTypeNames;
			break;
		}
	}

	if (! genericNodeClass)
		return NULL;


	// Map the generic types (found in ports) to the specialized types (found in the node class name)

	vector<string> nodeClassNameParts = VuoStringUtilities::split(nodeClassName, '.');
	vector<string> specializedTypeNames(nodeClassNameParts.begin() + nodeClassNameParts.size() - genericTypeNames.size(), nodeClassNameParts.end());

	map<string, string> specializedForGenericTypeName;
	for (size_t i = 0; i < genericTypeNames.size(); ++i)
	{
		if (! VuoGenericType::isGenericTypeName( specializedTypeNames[i] ) && ! compiler->getType( specializedTypeNames[i] ))
			return NULL;

		specializedForGenericTypeName[ genericTypeNames[i] ] = specializedTypeNames[i];
	}


	// Compile the specialized node class (with any necessary headers for types)

	string genericNodeClassName = genericNodeClass->getBase()->getClassName();
	string genericImplementation = genericNodeClass->getBase()->getNodeSet()->getNodeClassContents( genericNodeClassName );
	genericImplementation.insert(VuoFileUtilities::getFirstInsertionIndex(genericImplementation), "#define VuoSpecializedNode 1\n");

	string tmpDir = VuoFileUtilities::makeTmpDir(nodeClassName);

	replaceGenericTypesWithSpecialized(genericImplementation, specializedForGenericTypeName, nodeToBack);

	string tmpNodeClassImplementationFile = tmpDir + "/" + nodeClassName + ".c";
	string tmpNodeClassCompiledFile = tmpDir + "/" + nodeClassName + ".vuonode";
	VuoCompiler::preserveOriginalFileName(genericImplementation, genericNodeClassName + ".c");
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
	Module *module = compiler->readModuleFromBitcode(tmpNodeClassCompiledFile);

	for (set<string>::iterator i = tmpHeaders.begin(); i != tmpHeaders.end(); ++i)
		remove((*i).c_str());
	remove(tmpNodeClassImplementationFile.c_str());
	remove(tmpNodeClassCompiledFile.c_str());
	remove(tmpDir.c_str());


	// Construct the VuoCompilerSpecializedNodeClass

	VuoCompilerSpecializedNodeClass *dummyNodeClass = new VuoCompilerSpecializedNodeClass(nodeClassName, module);
	VuoCompilerSpecializedNodeClass *nodeClass = new VuoCompilerSpecializedNodeClass(dummyNodeClass, nodeToBack);
	delete dummyNodeClass;

	nodeClass->genericNodeClass = genericNodeClass;
	nodeClass->specializedForGenericTypeName = specializedForGenericTypeName;

	return nodeClass->getBase();
}

/**
 * Replaces all occurrences of the given generic type names in the node class source code with their corresponding
 * specialized type names.
 */
void VuoCompilerSpecializedNodeClass::replaceGenericTypesWithSpecialized(string &nodeClassSource, map<string, string> specializedForGenericTypeName, VuoNode *nodeToBack)
{
	map<string, string> replacementForGeneric = specializedForGenericTypeName;
	if (nodeToBack)
	{
		map<string, string> backingTypeForGeneric = getBackingTypeNamesFromPorts(nodeToBack);
		for (map<string, string>::iterator i = backingTypeForGeneric.begin(); i != backingTypeForGeneric.end(); ++i)
		{
			string genericTypeName = i->first;
			string backingTypeName = i->second;
			if (specializedForGenericTypeName[genericTypeName] == genericTypeName)
				replacementForGeneric[genericTypeName] = backingTypeName;
		}
	}

	set<string> replacementTypeNames;
	for (map<string, string>::iterator i = replacementForGeneric.begin(); i != replacementForGeneric.end(); ++i)
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
void VuoCompilerSpecializedNodeClass::replaceGenericTypesWithBacking(string &nodeClassSource, VuoNode *nodeToBack)
{
	size_t insertionPos = nodeClassSource.find("VuoModuleMetadata");  // assumed to be after '#include "node.h"' and before any generic type identifiers
	if (insertionPos == string::npos)
		return;

	map<string, string> backingTypeForGeneric;
	map<string, set<string> > compatibleTypesForGeneric;
	if (nodeToBack)
	{
		// Copy the backing types from nodeToBack.
		backingTypeForGeneric = getBackingTypeNamesFromPorts(nodeToBack);
		for (map<string, string>::iterator i = backingTypeForGeneric.begin(); i != backingTypeForGeneric.end(); ++i)
			backingTypeForGeneric[ VuoType::extractInnermostTypeName(i->first) ] = VuoType::extractInnermostTypeName(i->second);
	}
	else
	{
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
	}

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
			string innermostBackingTypeName = (nodeToBack ?
												   backingTypeForGeneric[innermostTypeName] :
												   VuoCompilerGenericType::chooseBackingTypeName(genericTypeName,
																								 compatibleTypesForGeneric[innermostTypeName]));
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
		string innermostBackingTypeName = (nodeToBack ?
											   backingTypeForGeneric[innermostTypeName] :
											   VuoCompilerGenericType::chooseBackingTypeName(innermostTypeName,
																							 compatibleTypesForGeneric[innermostTypeName]));
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
 * Returns the backing type for each generic type used by the given node class's ports.
 */
map<string, string> VuoCompilerSpecializedNodeClass::getBackingTypeNamesFromPorts(VuoNodeClass *nodeClass)
{
	map<string, string> backingTypeForPort;

	vector<VuoPortClass *> inputPortClasses = nodeClass->getInputPortClasses();
	vector<VuoPortClass *> outputPortClasses = nodeClass->getOutputPortClasses();
	vector<VuoPortClass *> portClasses;
	portClasses.insert(portClasses.end(), inputPortClasses.begin(), inputPortClasses.end());
	portClasses.insert(portClasses.end(), outputPortClasses.begin(), outputPortClasses.end());
	for (vector<VuoPortClass *>::iterator i = portClasses.begin(); i != portClasses.end(); ++i)
	{
		VuoCompilerPortClass *portClass = static_cast<VuoCompilerPortClass *>((*i)->getCompiler());
		VuoType *type = portClass->getDataVuoType();
		if (! type)
			continue;

		VuoCompilerGenericType *genericType = dynamic_cast<VuoCompilerGenericType *>(type->getCompiler());
		if (! genericType)
			continue;

		string genericTypeName = genericType->getBase()->getModuleKey();
		backingTypeForPort[genericTypeName] = genericType->getBackingTypeName();
	}

	return backingTypeForPort;
}

/**
 * Returns the backing type for each generic type used by the given node's ports.
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

		VuoCompilerGenericType *genericType = dynamic_cast<VuoCompilerGenericType *>(type->getCompiler());
		if (! genericType)
			continue;

		string genericTypeName = genericType->getBase()->getModuleKey();
		backingTypeForPort[genericTypeName] = genericType->getBackingTypeName();
	}

	return backingTypeForPort;
}

/**
 * Returns this port's type in the generic node that this specialized node class was derived from.
 */
VuoType * VuoCompilerSpecializedNodeClass::getOriginalPortType(VuoPortClass *portClass)
{
	vector<VuoPortClass *> specializedPortClasses;
	{
		vector<VuoPortClass *> inputPortClasses = getBase()->getInputPortClasses();
		vector<VuoPortClass *> outputPortClasses = getBase()->getOutputPortClasses();
		specializedPortClasses.insert(specializedPortClasses.end(), inputPortClasses.begin(), inputPortClasses.end());
		specializedPortClasses.insert(specializedPortClasses.end(), outputPortClasses.begin(), outputPortClasses.end());
	}

	vector<VuoPortClass *> genericPortClasses;
	{
		vector<VuoPortClass *> inputPortClasses = genericNodeClass->getBase()->getInputPortClasses();
		vector<VuoPortClass *> outputPortClasses = genericNodeClass->getBase()->getOutputPortClasses();
		genericPortClasses.insert(genericPortClasses.end(), inputPortClasses.begin(), inputPortClasses.end());
		genericPortClasses.insert(genericPortClasses.end(), outputPortClasses.begin(), outputPortClasses.end());
	}

	for (int i = 0; i < specializedPortClasses.size(); ++i)
	{
		if (specializedPortClasses[i] == portClass)
		{
			VuoCompilerPortClass *compilerPortClass = static_cast<VuoCompilerPortClass *>(genericPortClasses[i]->getCompiler());
			return compilerPortClass->getDataVuoType();
		}
	}

	return NULL;
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
VuoNodeSet *VuoCompilerSpecializedNodeClass::getOriginalGenericNodeSet(void)
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
 * Extracts the generic node class name from the specialized node class name by removing type names from the end.
 */
string VuoCompilerSpecializedNodeClass::extractGenericNodeClassName(string specializedNodeClassName, size_t genericTypeCount)
{
	vector<string> specializedNameParts = VuoStringUtilities::split(specializedNodeClassName, '.');
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
