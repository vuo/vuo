/**
 * @file
 * VuoCompilerConstantsCache implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCompilerCodeGenUtilities.hh"
#include "VuoCompilerConstantsCache.hh"
#include "VuoStringUtilities.hh"

/**
 * Creates an object that will generate code into @a module for constants that are not yet cached.
 */
VuoCompilerConstantsCache::VuoCompilerConstantsCache(Module *module)
{
	this->module = module;
}

/**
 * Returns a constant containing the value of the given string, generating code for the constant if needed.
 */
Constant * VuoCompilerConstantsCache::get(const string &s)
{
	auto iter = constantStrings.find(s);
	if (iter != constantStrings.end())
		return iter->second;

	string variableName = VuoStringUtilities::prefixSymbolName("string", module->getModuleIdentifier());
	Constant *c = VuoCompilerCodeGenUtilities::generatePointerToConstantString(module, s, variableName);
	constantStrings[s] = c;
	return c;
}

/**
 * Returns a constant array containing the given numbers as unsigned longs, generating code for the constant if needed.
 */
Constant * VuoCompilerConstantsCache::get(const vector<size_t> &a)
{
	auto iter = constantArraysOfUnsignedLongs.find(a);
	if (iter != constantArraysOfUnsignedLongs.end())
		return iter->second;

	string variableName = VuoStringUtilities::prefixSymbolName("array", module->getModuleIdentifier());
	Constant *c = VuoCompilerCodeGenUtilities::generatePointerToConstantArrayOfUnsignedLongs(module, a, variableName);
	constantArraysOfUnsignedLongs[a] = c;
	return c;
}
