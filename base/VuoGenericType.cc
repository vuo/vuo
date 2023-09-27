/**
 * @file
 * VuoGenericType implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoGenericType.hh"
#include "VuoStringUtilities.hh"
#include <sstream>

/// The common beginning of all generic port type names.
const string VuoGenericType::genericTypeNamePrefix = "VuoGenericType";

/**
 * Creates a generic type.
 */
VuoGenericType::VuoGenericType(string typeName, vector<string> compatibleSpecializedTypes)
	: VuoType(typeName)
{
	this->compatibleSpecializedTypes = compatibleSpecializedTypes;

	string compatiblesDescription;
	Compatibility compatibility;
	vector<string> compatibles = getCompatibleSpecializedTypes(compatibility);
	if (compatibility == anyType)
		compatiblesDescription = "any type";
	else if (compatibility == anyListType)
		compatiblesDescription = "any list type";
	else
	{
		for (size_t i = 0; i < compatibles.size(); ++i)
		{
			compatiblesDescription += compatibles[i];
			if (compatibles.size() >= 3)
			{
				if (i < compatibles.size() - 2)
					compatiblesDescription += ", ";
				else if (i == compatibles.size() - 2)
					compatiblesDescription += ", or ";
			}
			else if (compatibles.size() == 2 && i == 0)
				compatiblesDescription += " or ";
		}
	}
	string suffix = extractSuffixStringFromGenericTypeName(typeName);
	string typeDescription = "Generic #" + suffix + (VuoType::isListTypeName(typeName) ? " List" : "");
	setDefaultTitle( "(" + typeDescription + " — can connect to " + compatiblesDescription + ")" );
}

/**
 * Returns true if this generic type is allowed to be specialized with the given type.
 */
bool VuoGenericType::isSpecializedTypeCompatible(const string &typeName)
{
	Compatibility compatibility;
	vector<string> compatibles = getCompatibleSpecializedTypes(compatibility);

	if ((compatibility == anyType && ! VuoType::isListTypeName(typeName)) ||
			(compatibility == anyListType && VuoType::isListTypeName(typeName)))
		return true;

	return find(compatibles.cbegin(), compatibles.cend(), typeName) != compatibles.cend();
}

/**
 * Returns true if this generic type has any compatible specialized types in common with the given generic type.
 */
bool VuoGenericType::isGenericTypeCompatible(VuoGenericType *otherType)
{
	Compatibility thisCompatibility;
	vector<string> thisCompatibles = getCompatibleSpecializedTypes(thisCompatibility);
	Compatibility otherCompatibility;
	vector<string> otherCompatibles = otherType->getCompatibleSpecializedTypes(otherCompatibility);

	if ((thisCompatibility == anyType && ! VuoType::isListTypeName(otherType->getModuleKey())) ||
			(otherCompatibility == anyType && ! VuoType::isListTypeName(getModuleKey())) ||
			(thisCompatibility == anyListType && VuoType::isListTypeName(otherType->getModuleKey())) ||
			(otherCompatibility == anyListType && VuoType::isListTypeName(getModuleKey())))
		return true;

	std::sort(thisCompatibles.begin(), thisCompatibles.end());
	std::sort(otherCompatibles.begin(), otherCompatibles.end());

	set<string> bothCompatibles;
	set_intersection(thisCompatibles.begin(), thisCompatibles.end(),
					 otherCompatibles.begin(), otherCompatibles.end(),
					 std::insert_iterator<set<string> >(bothCompatibles, bothCompatibles.begin() ));
	return (! bothCompatibles.empty());
}

/**
 * Returns a set of the non-generic types compatible with the given type, or
 * an empty set if all types or all list types are compatible.
 *
 * @param[out] compatibility If the returned set is empty, this becomes @ref anyType or @ref anyListType.
 *		Otherwise, this becomes @ref whitelistedTypes.
 *
 * @see VuoModuleMetadata
 */
vector<string> VuoGenericType::getCompatibleSpecializedTypes(Compatibility &compatibility)
{
	compatibility = (compatibleSpecializedTypes.empty() ?
						 (VuoType::isListTypeName(getModuleKey()) ?
							  anyListType :
							  anyType) :
						 whitelistedTypes);

	return compatibleSpecializedTypes;
}

/**
 * Returns true if the name has the format of a generic type name.
 */
bool VuoGenericType::isGenericTypeName(string typeName)
{
	string genericTypeName;
	findGenericTypeName(typeName, 0, genericTypeName);
	return genericTypeName == typeName;
}

/**
 * Finds all unique generic type names in @a stringToSearch.
 */
vector<string> VuoGenericType::findGenericTypeNames(const string &stringToSearch)
{
	vector<string> genericTypeNames;

	size_t pos = 0;
	string genericTypeName;
	while ((pos = VuoGenericType::findGenericTypeName(stringToSearch, pos, genericTypeName)) != string::npos)
	{
		pos += genericTypeName.length();
		genericTypeName = VuoType::extractInnermostTypeName(genericTypeName);
		if (find(genericTypeNames.begin(), genericTypeNames.end(), genericTypeName) == genericTypeNames.end())
			genericTypeNames.push_back(genericTypeName);
	}

	return genericTypeNames;
}

/**
 * Finds the next generic type name in @a stringToSearch, starting from @a startPos.
 *
 * The generic type name may be a singleton type (e.g. @c VuoGeneric1) or a collection type (e.g. @c VuoList_VuoGeneric1).
 *
 * @param stringToSearch The string in which to search.
 * @param startPos The index in @a stringToSearch where the search should begin.
 * @param[out] genericTypeName The generic type name that begins at the returned index.
 * @return The index of the found generic type name in @a stringToSearch, or @c string::npos if none is found.
 */
size_t VuoGenericType::findGenericTypeName(string stringToSearch, size_t startPos, string &genericTypeName)
{
	size_t genericStartPos = startPos;

	while (true)
	{
		genericStartPos = stringToSearch.find(genericTypeNamePrefix, genericStartPos);
		if (genericStartPos == string::npos)
		{
			genericTypeName = "";
			break;
		}

		string suffix;
		for (size_t i = genericStartPos + genericTypeNamePrefix.length(); i < stringToSearch.length(); ++i)
		{
			if ('0' <= stringToSearch[i] && stringToSearch[i] <= '9')
				suffix += stringToSearch[i];
			else
				break;
		}

		if (! suffix.empty())
		{
			string listPrefix;
			if (genericStartPos >= VuoType::listTypeNamePrefix.length())
			{
				size_t listPrefixStartPos = genericStartPos - VuoType::listTypeNamePrefix.length();
				if (stringToSearch.substr(listPrefixStartPos, VuoType::listTypeNamePrefix.length()) == VuoType::listTypeNamePrefix)
				{
					listPrefix = VuoType::listTypeNamePrefix;
					genericStartPos = listPrefixStartPos;
				}
			}

			genericTypeName = listPrefix + genericTypeNamePrefix + suffix;
			break;
		}

		genericStartPos += genericTypeNamePrefix.length();
	}

	return genericStartPos;
}

/**
 * Returns a type name created by replacing the innermost type name in @a genericTypeName
 * (either the innermost item type if @a genericTypeName is a list type, or else @a genericTypeName itself)
 * with @a replacementTypeName.
 */
string VuoGenericType::replaceInnermostGenericTypeName(string genericTypeName, string replacementTypeName)
{
	if (! isGenericTypeName(genericTypeName))
		return genericTypeName;

	string typeName = genericTypeName;
	string innermostTypeName = VuoType::extractInnermostTypeName(genericTypeName);
	size_t innermostTypeNamePos = typeName.find(innermostTypeName);
	typeName.replace(innermostTypeNamePos, innermostTypeName.length(), replacementTypeName);
	return typeName;
}

/**
 * Finds each generic type name from @a specializedForGenericTypeName in @a stringToSearch and replaces it with the corresponding
 * specialized type name from @a specializedForGenericTypeName.
 *
 * This function searches for the longest matching generic type name. For example, if @a stringToSearch is
 * `VuoGenericType11_fun` and @a specializedForGenericTypeName contains both `VuoGenericType1` and `VuoGenericType11`, then
 * the substring `VuoGenericType11` will be replaced.
 *
 * For @a orderedGenericTypeNames, pass an empty vector on the first call and the same (modified) vector
 * on any subsequent calls that use the same @a specializedForGenericTypeName.
 */
bool VuoGenericType::replaceGenericTypeNamesInString(string &stringToSearch, const map<string, string> &specializedForGenericTypeName,
													 vector<string> &orderedGenericTypeNames)
{
	if (orderedGenericTypeNames.empty())
	{
		std::transform(specializedForGenericTypeName.begin(), specializedForGenericTypeName.end(),
					   std::inserter(orderedGenericTypeNames, orderedGenericTypeNames.end()),
					   [](const pair<string, string> &p) { return p.first; });

		std::sort(orderedGenericTypeNames.begin(), orderedGenericTypeNames.end(),
				  [](const string &name1, const string &name2) { return name1.length() > name2.length(); });
	}

	size_t replacementCount = 0;
	for (const string &genericTypeName : orderedGenericTypeNames)
	{
		auto specializedTypeNameIter = specializedForGenericTypeName.find(genericTypeName);
		if (specializedTypeNameIter != specializedForGenericTypeName.end())
			replacementCount += VuoStringUtilities::replaceAll(stringToSearch, genericTypeName, specializedTypeNameIter->second);
	}

	return replacementCount > 0;
}

/**
 * Creates a generic type name by appending the given numerical suffix.
 */
string VuoGenericType::createGenericTypeName(unsigned int suffix)
{
	ostringstream oss;
	oss << genericTypeNamePrefix << suffix;
	return oss.str();
}

/**
 * Puts the list of generic type names in ascending order of their numerical suffix.
 */
void VuoGenericType::sortGenericTypeNames(vector<string> &genericTypeNames)
{
	sort(genericTypeNames.begin(), genericTypeNames.end(), isPairOfGenericTypesSorted);
}

/**
 * Helper function for sorting a list of generic type names.
 */
bool VuoGenericType::isPairOfGenericTypesSorted(string type1, string type2)
{
	unsigned int suffix1 = extractSuffixFromGenericTypeName(type1);
	unsigned int suffix2 = extractSuffixFromGenericTypeName(type2);
	return (suffix1 < suffix2);
}

/**
 * Returns the numerical suffix at the end of the generic type's name.
 */
string VuoGenericType::extractSuffixStringFromGenericTypeName(string genericTypeName)
{
	if (! isGenericTypeName(genericTypeName))
		return "";

	size_t prefixStartPos = genericTypeName.find(genericTypeNamePrefix);
	size_t suffixStartPos = prefixStartPos + genericTypeNamePrefix.length();
	return genericTypeName.substr(suffixStartPos);
}

/**
 * Returns the numerical suffix at the end of the generic type's name.
 */
unsigned int VuoGenericType::extractSuffixFromGenericTypeName(string genericTypeName)
{
	string suffixStr = extractSuffixStringFromGenericTypeName(genericTypeName);
	unsigned int suffix = 0;
	istringstream iss(suffixStr);
	iss >> suffix;
	return suffix;
}
