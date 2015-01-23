/**
 * @file
 * VuoTestFloat implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "VuoTestFloat.h"

/// @{
#include "type.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "TestFloat"
				 });
#endif
/// @}


VuoTestFloat VuoTestFloat_valueFromJson(struct json_object *js)
{
	return (VuoTestFloat)json_object_get_double(js);
}

struct json_object * VuoTestFloat_jsonFromValue(const VuoTestFloat value)
{
	return json_object_new_double(value);
}

char * VuoTestFloat_stringFromValue(const VuoTestFloat value);

char * VuoTestFloat_summaryFromValue(const VuoTestFloat value)
{
	return VuoTestFloat_stringFromValue(value);
}
