/**
 * @file
 * MyPairOfReals implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "MyPairOfReals.h"
#include "VuoText.h"

VuoModuleMetadata({
					 "title" : "My Pair of Reals",
					 "description" : "A pair of real numbers.",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "c"
					 ]
				 });

MyPairOfReals MyPairOfReals_valueFromJson(json_object * js)
{
	MyPairOfReals value = {0,0};
	json_object *o = NULL;

	if (json_object_object_get_ex(js, "first", &o))
		value.first = VuoReal_valueFromJson(o);

	if (json_object_object_get_ex(js, "second", &o))
		value.second = VuoReal_valueFromJson(o);

	return value;
}

json_object * MyPairOfReals_jsonFromValue(const MyPairOfReals value)
{
	json_object *js = json_object_new_object();

	json_object *firstObject = VuoReal_jsonFromValue(value.first);
	json_object_object_add(js, "first", firstObject);

	json_object *secondObject = VuoReal_jsonFromValue(value.second);
	json_object_object_add(js, "second", secondObject);

	return js;
}

char * MyPairOfReals_summaryFromValue(const MyPairOfReals value)
{
	return VuoText_format("first = %g, second = %g", value.first, value.second);
}
