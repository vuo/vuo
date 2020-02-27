/**
 * @file
 * VuoTestFloat C type definition.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

struct json_object;

/**
 * A 32-bit floating-point number, for testing.
 */
typedef float VuoTestFloat;

VuoTestFloat VuoTestFloat_makeFromJson(struct json_object *js);
struct json_object * VuoTestFloat_getJson(const VuoTestFloat value);
char * VuoTestFloat_getSummary(const VuoTestFloat value);
