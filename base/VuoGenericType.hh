/**
 * @file
 * VuoGenericType interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoType.hh"

/**
 * A generic port type.
 *
 * The generic type is a placeholder until the port is specialized (i.e., its type is replaced) with a non-generic type.
 */
class VuoGenericType : public VuoType
{
public:
	/**
	 * Descriptions of the specialized types with which a generic type is compatible.
	 */
	enum Compatibility
	{
		anyType, ///< Compatible with any specialized singleton type.
		anyListType, ///< Compatible with any specialized VuoList type.
		whitelistedTypes ///< Compatible only with certain types.
	};

	VuoGenericType(string typeName, vector<string> compatibleSpecializedTypes);
	bool isSpecializedTypeCompatible(const string &typeName);
	bool isGenericTypeCompatible(VuoGenericType *otherType);
	vector<string> getCompatibleSpecializedTypes(Compatibility &compatibility);

	static bool isGenericTypeName(string typeName);
	static vector<string> findGenericTypeNames(const string &stringToSearch);
	static string replaceInnermostGenericTypeName(string genericTypeName, string replacementTypeName);
	static bool replaceGenericTypeNamesInString(string &stringToSearch, const map<string, string> &specializedForGenericTypeName, vector<string> &orderedGenericTypeNames);
	static string createGenericTypeName(unsigned int suffix);
	static void sortGenericTypeNames(vector<string> &genericTypeNames);

private:
	vector<string> compatibleSpecializedTypes;

	static const string genericTypeNamePrefix;

	static size_t findGenericTypeName(string stringToSearch, size_t startPos, string &genericTypeName);
	static bool isPairOfGenericTypesSorted(string type1, string type2);
	static string extractSuffixStringFromGenericTypeName(string genericTypeName);
	static unsigned int extractSuffixFromGenericTypeName(string genericTypeName);
};
