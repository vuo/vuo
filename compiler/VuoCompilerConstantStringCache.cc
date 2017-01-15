/**
 * @file
 * VuoCompilerConstantStringCache implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerCodeGenUtilities.hh"
#include "VuoCompilerConstantStringCache.hh"

/**
 * Returns a constant containing the value of the given string, generating code for the constant if needed.
 *
 * Consecutive calls of this function should all pass the same @a module until clear() is called.
 */
Constant * VuoCompilerConstantStringCache::get(Module *module, const string &s)
{
	map<string, Constant *>::iterator iter = constantStrings.find(s);
	if (iter != constantStrings.end())
		return iter->second;

	Constant *c = VuoCompilerCodeGenUtilities::generatePointerToConstantString(module, s);
	constantStrings[s] = c;
	return c;
}

/**
 * Empties the cache. Call this function before switching to a different LLVM module.
 */
void VuoCompilerConstantStringCache::clear(void)
{
	constantStrings.clear();
}
