/**
 * @file
 * VuoCompilerGenericType implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerGenericType.hh"
#include "VuoCompiler.hh"
#include "VuoCompilerNodeClass.hh"
#include "VuoGenericType.hh"
#include "VuoStringUtilities.hh"


/// A type to replace generic types with if they haven't been specialized.
const string VuoCompilerGenericType::defaultBackingTypeName = "VuoInteger";


/**
 * Creates a VuoCompilerGenericType and makes it the compiler detail object for @a baseType.
 */
VuoCompilerGenericType * VuoCompilerGenericType::newGenericType(VuoGenericType *baseType, VuoCompiler *compiler)
{
	return newGenericType(baseType, compiler->getTypes());
}

/**
 * Creates a VuoCompilerGenericType and makes it the compiler detail object for @a baseType.
 */
VuoCompilerGenericType * VuoCompilerGenericType::newGenericType(VuoGenericType *baseType, map<string, VuoCompilerType *> types)
{
	VuoGenericType::Compatibility compatibility;
	vector<string> compatibleTypeNames = baseType->getCompatibleSpecializedTypes(compatibility);
	string backingTypeName = VuoCompilerGenericType::chooseBackingTypeName(baseType->getModuleKey(), compatibleTypeNames);

	map<string, VuoCompilerType *>::iterator backingTypeIter = types.find(backingTypeName);
	if (backingTypeIter == types.end())
		return NULL;

	return new VuoCompilerGenericType(baseType, backingTypeIter->second);
}

/**
 * Creates a VuoCompilerGenericType and makes it the compiler detail object for @a baseType.
 */
VuoCompilerGenericType::VuoCompilerGenericType(VuoGenericType *baseType, VuoCompilerType *backingType)
	: VuoCompilerType(backingType->getBase()->getModuleKey(), backingType->getModule())
{
	setBase(baseType);
	baseType->setCompiler(this);
}

/**
 * Returns the name of a non-generic type to use for compiling/linking a node class or composition with
 * the given generic type.
 *
 * @param genericTypeName The generic type's name.
 * @param compatibleTypeNames The compatible specialized type names, as specified in a node class's "genericTypes" metadata.
 *		If this is non-empty, then the first item is chosen.
 *
 * @see VuoModuleMetadata
 */
string VuoCompilerGenericType::chooseBackingTypeName(string genericTypeName, vector<string> compatibleTypeNames)
{
	return (! compatibleTypeNames.empty() ?
				compatibleTypeNames.front() :
				VuoGenericType::replaceInnermostGenericTypeName(genericTypeName, defaultBackingTypeName));
}

/**
 * Returns the non-generic type to use for compiling/linking a node class or composition with this generic type.
 */
string VuoCompilerGenericType::getBackingTypeName(void)
{
	VuoGenericType *baseType = static_cast<VuoGenericType *>(getBase());
	VuoGenericType::Compatibility compatibility;
	return chooseBackingTypeName(baseType->getModuleKey(), baseType->getCompatibleSpecializedTypes(compatibility));
}
