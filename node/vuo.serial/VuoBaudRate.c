/**
 * @file
 * VuoBaudRate implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoBaudRate.h"
#include "VuoList_VuoBaudRate.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Baud Rate",
					  "description" : "The speed of a serial connection.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoText",
						  "VuoList_VuoBaudRate"
					  ]
				  });
#endif
/// @}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   "75"
 * }
 */
VuoBaudRate VuoBaudRate_makeFromJson(json_object *js)
{
	return json_object_get_int64(js);
}

/**
 * Encodes @c value as a JSON object.
 */
json_object *VuoBaudRate_getJson(const VuoBaudRate value)
{
	return json_object_new_int64(value);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoBaudRate VuoBaudRate_getAllowedValues(void)
{
	VuoList_VuoBaudRate l = VuoListCreate_VuoBaudRate();
	VuoListAppendValue_VuoBaudRate(l, VuoBaudRate_200   );
	VuoListAppendValue_VuoBaudRate(l, VuoBaudRate_300   );
	VuoListAppendValue_VuoBaudRate(l, VuoBaudRate_600   );
	VuoListAppendValue_VuoBaudRate(l, VuoBaudRate_1200  );
	VuoListAppendValue_VuoBaudRate(l, VuoBaudRate_1800  );
	VuoListAppendValue_VuoBaudRate(l, VuoBaudRate_2400  );
	VuoListAppendValue_VuoBaudRate(l, VuoBaudRate_4800  );
	VuoListAppendValue_VuoBaudRate(l, VuoBaudRate_9600  );
	VuoListAppendValue_VuoBaudRate(l, VuoBaudRate_14400 );
	VuoListAppendValue_VuoBaudRate(l, VuoBaudRate_19200 );
	VuoListAppendValue_VuoBaudRate(l, VuoBaudRate_28800 );
	VuoListAppendValue_VuoBaudRate(l, VuoBaudRate_38400 );
	VuoListAppendValue_VuoBaudRate(l, VuoBaudRate_57600 );
	VuoListAppendValue_VuoBaudRate(l, VuoBaudRate_115200);
	VuoListAppendValue_VuoBaudRate(l, VuoBaudRate_230400);
	VuoListAppendValue_VuoBaudRate(l, VuoBaudRate_460800);
	VuoListAppendValue_VuoBaudRate(l, VuoBaudRate_921600);
	return l;
}

/**
 * Returns a compact string representation of @c value.
 */
char *VuoBaudRate_getSummary(const VuoBaudRate value)
{
	return VuoText_format("%d BPS", value);
}

/**
 * Returns true if the two values are equal.
 */
bool VuoBaudRate_areEqual(const VuoBaudRate valueA, const VuoBaudRate valueB)
{
	return valueA == valueB;
}

/**
 * Returns true if `valueA` is less than `valueB`.
 */
bool VuoBaudRate_isLessThan(const VuoBaudRate valueA, const VuoBaudRate valueB)
{
	return valueA < valueB;
}

