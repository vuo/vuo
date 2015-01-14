/**
 * @file
 * VuoTestFloat C type definition.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOTESTFLOAT_H
#define VUOTESTFLOAT_H

struct json_object;

/**
 * A 32-bit floating-point number, for testing.
 */
typedef float VuoTestFloat;

VuoTestFloat VuoTestFloat_valueFromJson(struct json_object *js);
struct json_object * VuoTestFloat_jsonFromValue(const VuoTestFloat value);
char * VuoTestFloat_summaryFromValue(const VuoTestFloat value);

#endif
