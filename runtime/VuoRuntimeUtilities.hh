/**
 * @file
 * VuoRuntimeUtilities interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/**
 * Miscellaneous helper functions for runtime classes.
 */
class VuoRuntimeUtilities
{
public:
	static unsigned long hash(const char *str);
};
