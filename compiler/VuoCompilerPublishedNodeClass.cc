/**
 * @file
 * VuoCompilerPublishedNodeClass implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include <sstream>

#include "VuoCompiler.hh"
#include "VuoCompilerPortClass.hh"
#include "VuoCompilerPublishedPort.hh"
#include "VuoCompilerPublishedNodeClass.hh"
#include "VuoCompilerPublishedInputNodeClass.hh"
#include "VuoCompilerPublishedOutputNodeClass.hh"
#include "VuoCompilerType.hh"
#include "VuoGenericType.hh"
#include "VuoMakeDependencies.hh"
#include "VuoNode.hh"
#include "VuoNodeClass.hh"
#include "VuoPublishedPort.hh"
#include "VuoStringUtilities.hh"
#include "VuoType.hh"

/**
 * Creates a node class implementation from an LLVM module, and creates its corresponding base @c VuoNodeClass.
 */
VuoCompilerPublishedNodeClass::VuoCompilerPublishedNodeClass(string nodeClassName, Module *module) :
	VuoCompilerSpecializedNodeClass(nodeClassName, module)
{
}

/**
 * Creates a new compiler node class and a new base @c VuoNodeClass, both from @c compilerNodeClass.
 */
VuoCompilerPublishedNodeClass::VuoCompilerPublishedNodeClass(VuoCompilerPublishedNodeClass *compilerNodeClass) :
	VuoCompilerSpecializedNodeClass(compilerNodeClass)
{
}

/**
 * Creates a new implementation-less compiler node class, using the given node class for its base VuoNodeClass.
 */
VuoCompilerPublishedNodeClass::VuoCompilerPublishedNodeClass(VuoNodeClass *baseNodeClass) :
	VuoCompilerSpecializedNodeClass(baseNodeClass)
{
}

/**
 * Returns a new node class with port names and types as specified by @a nodeClassName, or null if @a nodeClassName is not a
 * valid published input/output node class name.
 */
VuoNodeClass * VuoCompilerPublishedNodeClass::newNodeClass(const string &nodeClassName, VuoCompiler *compiler, dispatch_queue_t llvmQueue, VuoCompilerPublishedNodeClass *singleton)
{
	vector<string> portNames;
	vector<string> typeNames;
	bool parsedOk = singleton->parseNodeClassName(nodeClassName, portNames, typeNames);
	if (! parsedOk)
		return NULL;

	vector<VuoType *> types;
	for (string typeName : typeNames)
	{
		VuoCompilerType *type = (typeName == "event" ? nullptr : compiler->getType(typeName));
		types.push_back(type ? type->getBase() : nullptr);
	}

	return newNodeClass(portNames, types, llvmQueue, singleton);
}

/**
 * Returns a new node class with port names and types corresponding to @a publishedPorts.
 *
 * If there are any name collisions between @a publishedPorts and the ports that appear on every published input/output node
 * (such as the refresh port), the port in the node class corresponding to the published port is assigned a unique name.
 */
VuoNodeClass * VuoCompilerPublishedNodeClass::newNodeClass(const vector<VuoPublishedPort *> &publishedPorts, dispatch_queue_t llvmQueue, VuoCompilerPublishedNodeClass *singleton)
{
	vector<string> portNames = singleton->formUniquePortNames(publishedPorts);

	vector<VuoType *> types;
	for (VuoPublishedPort *publishedPort : publishedPorts)
	{
		VuoType *type = static_cast<VuoCompilerPort *>(publishedPort->getCompiler())->getDataVuoType();
		types.push_back(type);
	}

	return newNodeClass(portNames, types, llvmQueue, singleton);
}

/**
 * Returns a new node class with the given port names and types.
 */
VuoNodeClass * VuoCompilerPublishedNodeClass::newNodeClass(const vector<string> &portNames, const vector<VuoType *> &types, dispatch_queue_t llvmQueue, VuoCompilerPublishedNodeClass *singleton)
{
	string nodeClassName = singleton->buildNodeClassName(portNames, types);

	bool isFullySpecialized = true;
	for (VuoType *type : types)
	{
		if (dynamic_cast<VuoGenericType *>(type))
		{
			isFullySpecialized = false;
			break;
		}
	}

	__block VuoNodeClass *nodeClass;
	if (isFullySpecialized && llvmQueue)
	{
		dispatch_sync(llvmQueue, ^{
						  nodeClass = singleton->newNodeClassWithImplementation(nodeClassName, portNames, types);
					  });

		vector<string> typeDependencies;
		for (VuoType *type : types)
		{
			if (type && type->hasCompiler())
			{
				string typeDependency = type->getCompiler()->getDependencyPath();
				if (! typeDependency.empty())
					typeDependencies.push_back(typeDependency);
			}
		}

		std::sort(typeDependencies.begin(), typeDependencies.end());
		auto endOfUnique = std::unique(typeDependencies.begin(), typeDependencies.end());
		typeDependencies.erase(endOfUnique, typeDependencies.end());

		VuoCompilerPublishedNodeClass *publishedNodeClass = static_cast<VuoCompilerPublishedNodeClass *>(nodeClass->getCompiler());
		publishedNodeClass->makeDependencies = VuoMakeDependencies::createFromComponents(VuoMakeDependencies::getPlaceholderCompiledFilePath(), typeDependencies);
	}
	else
	{
		nodeClass = singleton->newNodeClassWithoutImplementation(nodeClassName, portNames, types);
	}

	return nodeClass;
}

/**
 * Returns true if the name has the format of a published input or output node class name.
 * (A node class by that name may or may not exist.)
 */
bool VuoCompilerPublishedNodeClass::isPublishedNodeClassName(const string &nodeClassName)
{
	return nodeClassName == VuoNodeClass::publishedInputNodeClassName ||
			nodeClassName == VuoNodeClass::publishedOutputNodeClassName ||
			VuoStringUtilities::beginsWith(nodeClassName, VuoNodeClass::publishedInputNodeClassName + ".") ||
			VuoStringUtilities::beginsWith(nodeClassName, VuoNodeClass::publishedOutputNodeClassName + ".");
}

/**
 * If @a nodeClassName is a valid published input/output node class name, copies the port names and types from the node class name
 * into @a portNames and @a typeNames and returns true. Otherwise, returns false.
 */
bool VuoCompilerPublishedNodeClass::parseNodeClassName(string nodeClassName, vector<string> &portNames, vector<string> &typeNames)
{
	string countAndNamesAndTypesStr = VuoStringUtilities::substrAfter(nodeClassName, getNodeClassNamePrefix() + ".");
	if (countAndNamesAndTypesStr.empty())
		return false;

	vector<string> countAndNamesAndTypes = VuoStringUtilities::split(countAndNamesAndTypesStr, '.');
	if (countAndNamesAndTypes.empty())
		return false;

	string countStr = countAndNamesAndTypes[0];
	int count = atoi(countStr.c_str());
	if (count != (countAndNamesAndTypes.size() - 1) / 2)
		return false;

	for (int i = 0; i < count; ++i)
	{
		string portName = countAndNamesAndTypes.at(1 + i);
		portNames.push_back(portName);

		string typeName = countAndNamesAndTypes.at(1 + count + i);
		typeNames.push_back(typeName);
	}

	return true;
}

/**
 * Creates a class name for a published input/output node with the given published input/output port names and types.
 */
string VuoCompilerPublishedNodeClass::buildNodeClassName(const vector<string> &portNames, const vector<VuoType *> &types)
{
	vector<string> typeNames;
	for (VuoType *type : types)
		typeNames.push_back(type ? type->getModuleKey() : "event");

	return buildNodeClassName(portNames, typeNames);
}

/**
 * Creates a class name for a published input/output node with the given published input/output port names and type names.
 */
string VuoCompilerPublishedNodeClass::buildNodeClassName(const vector<string> &portNames, const vector<string> &typeNames)
{
	vector<string> nodeClassNameParts;
	nodeClassNameParts.push_back(getNodeClassNamePrefix());

	ostringstream oss;
	oss << portNames.size();
	nodeClassNameParts.push_back(oss.str());

	for (string portName : portNames)
		nodeClassNameParts.push_back(portName);

	for (string typeName : typeNames)
		nodeClassNameParts.push_back(typeName);

	return VuoStringUtilities::join(nodeClassNameParts, '.');
}

/**
 * Creates a class name for a published input/output node with the given published input/output ports.
 */
string VuoCompilerPublishedNodeClass::buildNodeClassNameFromPorts(const vector<VuoPublishedPort *> &publishedPorts)
{
	vector<string> portNames = formUniquePortNames(publishedPorts);

	vector<VuoType *> types;
	for (VuoPublishedPort *publishedPort : publishedPorts)
	{
		VuoType *type = static_cast<VuoCompilerPort *>(publishedPort->getCompiler())->getDataVuoType();
		types.push_back(type);
	}

	return buildNodeClassName(portNames, types);
}

/**
 * Returns the list of input port names that a published input/output node would have corresponding to @a publishedPorts.
 *
 * If there are any name collisions between @a publishedPorts and the ports that appear on every published input/output node
 * (such as the refresh port), the port in the node class corresponding to the published port is assigned a unique name.
 */
vector<string> VuoCompilerPublishedNodeClass::formUniquePortNames(const vector<VuoPublishedPort *> &publishedPorts)
{
	set<string> takenPortNames = getReservedPortNames();

	vector<string> portNames;
	for (VuoPublishedPort *publishedPort : publishedPorts)
	{
		string uniquePortName = VuoStringUtilities::formUniqueIdentifier(takenPortNames, publishedPort->getClass()->getName());
		portNames.push_back(uniquePortName);
	}

	return portNames;
}

/**
 * Constructs the value for the "specializedModule" key to be added to `VuoModuleMetadata`.
 */
json_object * VuoCompilerPublishedNodeClass::buildSpecializedModuleDetails(const vector<VuoType *> &types)
{
	map<string, string> specializedForGenericTypeName;
	for (unsigned int i = 0; i < types.size(); ++i)
		if (types[i])
			specializedForGenericTypeName[VuoGenericType::createGenericTypeName(i)] = types[i]->getModuleKey();

	return VuoCompilerSpecializedNodeClass::buildSpecializedModuleDetails(specializedForGenericTypeName);
}

/**
 * Returns the file name (with extension) that this node class should have when saved to file —
 * a mangled (shortened) file name so as not to exceed the operating system's limit on file name length.
 */
string VuoCompilerPublishedNodeClass::getFileName(void)
{
	ostringstream hash;
	hash << VuoStringUtilities::hash(getBase()->getModuleKey());

	return getNodeClassNamePrefix() + "." + hash.str() + ".vuonode";
}

/**
 * Returns the file name (with extension) that a published input/output node class with the given module key
 * would have when saved to file.
 */
string VuoCompilerPublishedNodeClass::getFileNameForModuleKey(const string &moduleKey)
{
	string prefix = VuoStringUtilities::beginsWith(moduleKey, VuoNodeClass::publishedInputNodeClassName) ?
						VuoNodeClass::publishedInputNodeClassName :
						VuoNodeClass::publishedOutputNodeClassName;

	ostringstream hash;
	hash << VuoStringUtilities::hash(moduleKey);

	return prefix + "." + hash.str() + ".vuonode";
}

/**
 * Returns true — a node class with the given file name (with extension) would have a module key
 * other than the file name minus extension.
 */
bool VuoCompilerPublishedNodeClass::isModuleKeyMangledInFileName(const string &fileName)
{
	return true;
}

/**
 * Returns a fully specialized node of class @a backingNodeClassName that will replace the node class of @a nodeToBack.
 */
VuoCompilerNode * VuoCompilerPublishedNodeClass::createReplacementBackingNode(VuoNode *nodeToBack, string backingNodeClassName, VuoCompiler *compiler)
{
	vector<string> portNames;
	vector<string> typeNames;
	bool ok = parseNodeClassName(backingNodeClassName, portNames, typeNames);
	if (! ok)
		return NULL;

	vector<VuoPublishedPort *> publishedPorts;
	for (int i = 0; i < portNames.size(); ++i)
	{
		string portName = portNames[i];
		string typeName = typeNames[i];
		VuoType *type = (typeName == "event" ? NULL : compiler->getType(typeName)->getBase());
		VuoCompilerPublishedPort *publishedPort = VuoCompilerPublishedPort::newPort(portName, type);
		publishedPorts.push_back( static_cast<VuoPublishedPort *>(publishedPort->getBase()) );
	}

	return (dynamic_cast<VuoCompilerPublishedInputNodeClass *>(this) ?
				compiler->createPublishedInputNode(publishedPorts)->getCompiler() :
				compiler->createPublishedOutputNode(publishedPorts)->getCompiler());
}

/**
 * Returns this port's type in the (hypothetical) unspecialized published input node class.
 */
VuoType * VuoCompilerPublishedNodeClass::getOriginalPortType(VuoPortClass *portClass)
{
	/// @todo https://b33p.net/kosada/node/7655
	return NULL;
}

/**
 * Returns the original node's class name (without any type suffixes).
 */
string VuoCompilerPublishedNodeClass::getOriginalGenericNodeClassName(void)
{
	return getNodeClassNamePrefix();
}

/**
 * Returns the original node's class description (i.e., nothing).
 */
string VuoCompilerPublishedNodeClass::getOriginalGenericNodeClassDescription(void)
{
	return "";
}

/**
 * Returns the original node's node set (i.e., none).
 */
VuoNodeSet * VuoCompilerPublishedNodeClass::getOriginalGenericNodeSet(void)
{
	return NULL;
}

/**
 * Returns the name for the published input node class that would result if the given ports were changed back to their original types.
 */
string VuoCompilerPublishedNodeClass::createUnspecializedNodeClassName(set<VuoPortClass *> portClassesToUnspecialize)
{
	/// @todo https://b33p.net/kosada/node/7655
	return "";
}

/**
 * Returns the name for the published input node class that would result if the given specialized type were substituted for the
 * generic item type.
 */
string VuoCompilerPublishedNodeClass::createSpecializedNodeClassNameWithReplacement(string genericTypeName, string specializedTypeName)
{
	/// @todo https://b33p.net/kosada/node/7655
	return "";
}
