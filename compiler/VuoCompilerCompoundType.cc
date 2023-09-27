/**
 * @file
 * VuoCompilerCompoundType implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCompilerCompoundType.hh"
#include "VuoCompiler.hh"
#include "VuoCompilerException.hh"
#include "VuoCompilerIssue.hh"
#include "VuoGenericType.hh"
#include "VuoJsonUtilities.hh"
#include "VuoNodeSet.hh"
#include "VuoStringUtilities.hh"
#include "VuoType.hh"
#include <sstream>

/**
 * Creates a type from the generic type definition and specialized types indicated by @a typeName (e.g. `VuoDictionary_VuoText_VuoReal`).
 *
 * The beginning of @a typeName (e.g. `VuoDictionary`) is the generic type whose source code will be used.
 *
 * The remaining parts of @a typeName (e.g. `VuoText`, `VuoReal`) are the types that will replace the generic types in the source code.
 * The first specialized type replaces `VuoGenericType1`, the second replaces `VuoGenericType2`, etc.
 */
VuoCompilerCompoundType * VuoCompilerCompoundType::newType(const string &typeName, VuoCompiler *compiler, dispatch_queue_t llvmQueue)
{
	auto getVuoType = [compiler] (const string &moduleKey) { return compiler->getType(moduleKey); };

	VuoCompilerType *genericType = parseGenericCompoundType(typeName, getVuoType);
	if (! genericType)
		return nullptr;

	map<string, string> specializedForGenericTypeName = parseSpecializedTypes(typeName, genericType->getBase()->getModuleKey());

	string sourcePath;
	string sourceCode = getSourceCode(genericType, sourcePath);

	if (sourceCode.empty())
	{
		VuoCompilerIssue issue(VuoCompilerIssue::Error, "specializing compound type", "",
							   "Missing source code",
							   "%module uses generic types, but its source code isn't included in its node set.");
		issue.setModule(genericType->getBase());
		throw VuoCompilerException(issue);
	}

	VuoModuleCompilerResults results = compiler->compileModuleInMemory(sourcePath, sourceCode, specializedForGenericTypeName);

	__block VuoCompilerCompoundType *type = nullptr;
	dispatch_sync(llvmQueue, ^{
		type = new VuoCompilerCompoundType(typeName, results.module);
	});

	type->makeDependencies = results.makeDependencies;

	return type;
}

/**
 * Creates a type from the type definition in the module if it defines a compound type, otherwise returns null.
 */
VuoCompilerCompoundType * VuoCompilerCompoundType::newType(const string &typeName, Module *module)
{
	if (isSpecializedModule(module, typeName))
		return new VuoCompilerCompoundType(typeName, module);

	return nullptr;
}

/**
 * Creates a type from the type definition in the module.
 */
VuoCompilerCompoundType::VuoCompilerCompoundType(const string &typeName, Module *module) :
	VuoCompilerType(typeName, module)
{
	json_object *specializedDetails = nullptr;
	if (json_object_object_get_ex(moduleDetails, "specializedModule", &specializedDetails))
	{
		string title = VuoJsonUtilities::parseString(specializedDetails, "title");
		if (! title.empty())
			getBase()->setDefaultTitle(title);

		string genericTypeName = VuoJsonUtilities::parseString(specializedDetails, "genericModule");
		dependencies.insert(genericTypeName);

		map<string, string> specializedTypes = VuoJsonUtilities::parseObjectWithStringValues(specializedDetails, "specializedTypes");
		for (auto i : specializedTypes)
			dependencies.insert(i.second);
	}
}

/**
 * Returns the type whose name forms the prefix of @a specializedCompoundTypeName.
 */
VuoCompilerType * VuoCompilerCompoundType::parseGenericCompoundType(const string &specializedCompoundTypeName,
																	std::function<VuoCompilerType *(const string &)> getVuoType)
{
	vector<string> parts = VuoStringUtilities::split(specializedCompoundTypeName, '_');
	for (size_t i = 1; i < parts.size(); ++i)
	{
		vector<string> firstParts(parts.begin(), parts.begin() + i);
		string potentialGenericTypeName = VuoStringUtilities::join(firstParts, '_');

		VuoCompilerType *genericType = getVuoType(potentialGenericTypeName);
		if (genericType)
			return genericType;
	}

	return nullptr;
}

/**
 * Returns the type name that forms the prefix of @a specializedCompoundTypeName.
 */
string VuoCompilerCompoundType::parseGenericCompoundTypeName(const string &specializedCompoundTypeName, size_t genericTypeCount)
{
	vector<string> parts = VuoStringUtilities::split(specializedCompoundTypeName, '_');
	vector<string> genericNameParts(parts.begin(), parts.begin() + parts.size() - genericTypeCount);
	return VuoStringUtilities::join(genericNameParts, '_');
}

/**
 * Returns the mapping from generic to specialized types specified in the suffix of @a specializedCompoundTypeName
 * that follows @a genericCompoundTypeName.
 */
map<string, string> VuoCompilerCompoundType::parseSpecializedTypes(const string &specializedCompoundTypeName, const string &genericCompoundTypeName)
{
	map<string, string> specializedForGenericTypeName;

	string specializedTypeNames = VuoStringUtilities::substrAfter(specializedCompoundTypeName, genericCompoundTypeName + "_");
	if (! specializedTypeNames.empty())
	{
		vector<string> specializedParts = VuoStringUtilities::split(specializedTypeNames, '_');
		for (size_t i = 0; i < specializedParts.size(); ++i)
		{
			ostringstream genericTypeName;
			genericTypeName << "VuoGenericType" << (i + 1);
			specializedForGenericTypeName[genericTypeName.str()] = specializedParts[i];
		}
	}

	return specializedForGenericTypeName;
}

/**
 * Returns the name that a specialized compound type would have with a prefix of @a genericCompoundTypeName
 * followed by @a specializedTypeNames.
 */
string VuoCompilerCompoundType::buildSpecializedCompoundTypeName(const string &genericCompoundTypeName, const vector<string> &specializedTypeNames)
{
	return genericCompoundTypeName + "_" + VuoStringUtilities::join(specializedTypeNames, "_");
}

/**
 * Returns the name that a specialized compound type would have with a prefix of @a genericCompoundTypeName
 * followed by @a genericTypeCount generic type names.
 */
string VuoCompilerCompoundType::buildUnspecializedCompoundTypeName(const string &genericCompoundTypeName, size_t genericTypeCount)
{
	vector<string> genericTypeNames;
	for (size_t i = 0; i < genericTypeCount; ++i)
	{
		ostringstream genericTypeName;
		genericTypeName << "VuoGenericType" << (i + 1);
		genericTypeNames.push_back(genericTypeName.str());
	}

	return buildSpecializedCompoundTypeName(genericCompoundTypeName, genericTypeNames);
}

/**
 * Constructs the value for the "specializedModule" key to be added to `VuoModuleMetadata`.
 */
json_object * VuoCompilerCompoundType::buildSpecializedModuleDetails(VuoCompilerType *genericCompoundType, const map<string, string> &specializedForGenericTypeName,
																	 const map<string, VuoCompilerType *> &specializedTypes)
{
	json_object *specializedDetails = json_object_new_object();

	string title = buildDefaultTitle(genericCompoundType, specializedForGenericTypeName, specializedTypes);
	json_object_object_add(specializedDetails, "title", json_object_new_string(title.c_str()));

	json_object_object_add(specializedDetails, "genericModule", json_object_new_string(genericCompoundType->getBase()->getModuleKey().c_str()));

	json_object *specializedTypesObj = json_object_new_object();
	json_object_object_add(specializedDetails, "specializedTypes", specializedTypesObj);
	for (auto i : specializedForGenericTypeName)
		json_object_object_add(specializedTypesObj, i.first.c_str(), json_object_new_string(i.second.c_str()));

	return specializedDetails;
}

/**
 * Constructs the title that would be returned by VuoModule::getDefaultTitle() for the type with module key @a specializedCompoundTypeName.
 */
string VuoCompilerCompoundType::buildDefaultTitle(const string &specializedCompoundTypeName,
												  std::function<VuoCompilerType *(const string &)> getVuoType)
{
	VuoCompilerType *genericCompoundType = parseGenericCompoundType(specializedCompoundTypeName, getVuoType);
	map<string, string> specializedForGenericTypeName = parseSpecializedTypes(specializedCompoundTypeName, genericCompoundType->getBase()->getModuleKey());

	map<string, VuoCompilerType *> specializedTypes;
	for (auto i : specializedForGenericTypeName)
		specializedTypes[i.second] = getVuoType(i.second);

	return buildDefaultTitle(genericCompoundType, specializedForGenericTypeName, specializedTypes);
}

/**
 * Constructs the title that would be returned by VuoModule::getDefaultTitle() for the given specialization of @a genericCompoundType.
 */
string VuoCompilerCompoundType::buildDefaultTitle(VuoCompilerType *genericCompoundType, const map<string, string> &specializedForGenericTypeName,
												  const map<string, VuoCompilerType *> &specializedTypes)
{
	string title = genericCompoundType->getBase()->getDefaultTitle();

	map<string, string> specializedTitleForGenericTypeName;
	for (auto i : specializedForGenericTypeName)
	{
		auto specializedTypeIter = specializedTypes.find(i.second);
		specializedTitleForGenericTypeName[i.first] = specializedTypeIter != specializedTypes.end() ?
																				 specializedTypeIter->second->getBase()->getDefaultTitle() :
																				 i.second;
	}

	vector<string> unused;
	VuoGenericType::replaceGenericTypeNamesInString(title, specializedTitleForGenericTypeName, unused);

	return title;
}

/**
 * Looks up the source code and source path of @a genericCompoundType.
 */
string VuoCompilerCompoundType::getSourceCode(VuoCompilerType *genericCompoundType, string &sourcePath)
{
	string sourceCode;

	VuoNodeSet *nodeSet = genericCompoundType->getBase()->getNodeSet();
	if (nodeSet)
	{
		string relativePath = nodeSet->getModuleSourcePath( genericCompoundType->getBase()->getModuleKey() );
		sourceCode = nodeSet->getFileContents(relativePath);
		sourcePath = nodeSet->getArchivePath() + "/" + relativePath;
	}
	else
	{
		sourceCode = genericCompoundType->getSourceCode();
		sourcePath = genericCompoundType->getSourcePath();
	}

	return sourceCode;
}
