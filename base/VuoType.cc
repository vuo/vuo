/**
 * @file
 * VuoType implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoType.hh"
#include "VuoStringUtilities.hh"


/// The common beginning of all VuoList type names (before the item type name).
const string VuoType::listTypeNamePrefix = "VuoList_";

/// The common beginning of all VuoDictionary type names (before the key and value type names).
const string VuoType::dictionaryTypeNamePrefix = "VuoDictionary_";


/**
 * Creates a type.
 *
 * @param typeName A unique (across all types and node classes) name for this type.
 */
VuoType::VuoType(string typeName)
	: VuoBase<VuoCompilerType,void>("VuoType"),
	  VuoModule(typeName)
{
}

VuoType::~VuoType(void)
{
}

/**
 * Returns true if @a potentialTypeName has the format of a type's module key.
 * (A type by that name may or may not exist.)
 */
bool VuoType::isTypeName(const string &potentialTypeName)
{
	return potentialTypeName.find(".") == string::npos;
}

/**
 * Returns true if the type name is for a list type.
 */
bool VuoType::isListTypeName(const string &typeName)
{
	return VuoStringUtilities::beginsWith(typeName, listTypeNamePrefix);
}

/**
 * Returns true if the type name is for a dictionary type.
 */
bool VuoType::isDictionaryTypeName(const string &typeName)
{
	return VuoStringUtilities::beginsWith(typeName, dictionaryTypeNamePrefix);
}

/**
 * If the type name is a list (or list of lists, etc.), returns the innermost item type name.
 * Otherwise, returns the type name itself.
 */
string VuoType::extractInnermostTypeName(const string &typeName)
{
	string t{typeName};
	while (VuoStringUtilities::beginsWith(t, listTypeNamePrefix))
		t = VuoStringUtilities::substrAfter(t, listTypeNamePrefix);

	return t;
}
