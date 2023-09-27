/**
 * @file
 * Common headers for testing types.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "type.h"

#define QUOTE(...) #__VA_ARGS__

#define VUO_COMPARE_NAN(a,b) \
	if (!(isnan(a) && isnan(b))) \
		QCOMPARE(a, b)
