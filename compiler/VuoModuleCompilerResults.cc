/**
 * @file
 * VuoModuleCompilerResults implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoModuleCompilerResults.hh"
#include "VuoMakeDependencies.hh"

/**
 * Constructs an object with empty values for the results.
 */
VuoModuleCompilerResults::VuoModuleCompilerResults(void)
{
	module = nullptr;
}
