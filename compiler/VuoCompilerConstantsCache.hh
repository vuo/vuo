/**
 * @file
 * VuoCompilerConstantStringCache interface.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/**
 * A cache of LLVM constant values generated for a module. Reduces the number of constants in generated bitcode.
 */
class VuoCompilerConstantsCache
{
public:
	explicit VuoCompilerConstantsCache(Module *module);
	Constant * get(const string &s);
	Constant * get(const vector<size_t> &a);

private:
	Module *module;
	map<string, Constant *> constantStrings;
	map< vector<size_t>, Constant * > constantArraysOfUnsignedLongs;
};
