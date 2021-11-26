/**
 * @file
 * libUserDylib.dylib implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include <stdint.h>

int64_t calculate(int64_t x)
{
	return 2*x;
}
