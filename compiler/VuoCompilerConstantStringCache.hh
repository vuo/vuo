/**
 * @file
 * VuoCompilerConstantStringCache interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/**
 * A cache of LLVM constant values generated for a module. Reduces the number of constants in generated bitcode.
 */
class VuoCompilerConstantStringCache
{
public:
	Constant * get(Module *module, const string &s);
	void clear(void);

private:
	map<string, Constant *> constantStrings;
};
