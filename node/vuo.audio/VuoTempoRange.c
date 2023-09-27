/**
 * @file
 * VuoTempoRange implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoTempoRange.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Tempo Range",
					  "description" : "A range of BPM values.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
					  "VuoList_VuoTempoRange"
					  ]
				  });
#endif
/// @}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   "moderato"
 * }
 */
VuoTempoRange VuoTempoRange_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoTempoRange value = VuoTempoRange_Andante;

	if (strcmp(valueAsString, "moderato") == 0)
		value = VuoTempoRange_Moderato;
	else if (strcmp(valueAsString, "allegro") == 0)
		value = VuoTempoRange_Allegro;
	else if (strcmp(valueAsString, "presto") == 0)
		value = VuoTempoRange_Presto;
	else if (strcmp(valueAsString, "prestissimo") == 0)
		value = VuoTempoRange_Prestissimo;

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object *VuoTempoRange_getJson(const VuoTempoRange value)
{
	char *valueAsString = "andante";

	if (value == VuoTempoRange_Moderato)
		valueAsString = "moderato";
	else if (value == VuoTempoRange_Allegro)
		valueAsString = "allegro";
	else if (value == VuoTempoRange_Presto)
		valueAsString = "presto";
	else if (value == VuoTempoRange_Prestissimo)
		valueAsString = "prestissimo";

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoTempoRange VuoTempoRange_getAllowedValues(void)
{
	VuoList_VuoTempoRange l = VuoListCreate_VuoTempoRange();
	VuoListAppendValue_VuoTempoRange(l, VuoTempoRange_Andante);
	VuoListAppendValue_VuoTempoRange(l, VuoTempoRange_Moderato);
	VuoListAppendValue_VuoTempoRange(l, VuoTempoRange_Allegro);
	VuoListAppendValue_VuoTempoRange(l, VuoTempoRange_Presto);
	VuoListAppendValue_VuoTempoRange(l, VuoTempoRange_Prestissimo);
	return l;
}

/**
 * Returns a compact string representation of @c value.
 */
char *VuoTempoRange_getSummary(const VuoTempoRange value)
{
	// Each BPM range actually covers N to 2N BPM, but only works well for the middle ~60% of the range.

	char *valueAsString = " 70–110 BPM";      // actually 60–120 BPM
	// FIGURE SPACE U+2007, to match the width of the numeral

	if (value == VuoTempoRange_Moderato)
		valueAsString = "100–140 BPM";        // actually 80-160 BPM
	else if (value == VuoTempoRange_Allegro)
		valueAsString = "120–180 BPM";        // actually 100–200 BPM
	else if (value == VuoTempoRange_Presto)
		valueAsString = "170–250 BPM";        // actually 140-280 BPM
	else if (value == VuoTempoRange_Prestissimo)
		valueAsString = "220–320 BPM";        // actually 180-360 BPM

	return strdup(valueAsString);
}

/**
 * Returns the lower bound of the specified tempo range.
 * The upper bound is double the lower bound.
 */
int VuoTempoRange_getBaseBPM(const VuoTempoRange value)
{
	if (value == VuoTempoRange_Andante)
		return 60;
	else if (value == VuoTempoRange_Allegro)
		return 100;
	else if (value == VuoTempoRange_Presto)
		return 140;
	else if (value == VuoTempoRange_Prestissimo)
		return 180;
	else
		return 80;
}

/**
 * Returns true if the two values are equal.
 */
bool VuoTempoRange_areEqual(const VuoTempoRange valueA, const VuoTempoRange valueB)
{
	return valueA == valueB;
}

/**
 * Returns true if `valueA` is less than `valueB`.
 */
bool VuoTempoRange_isLessThan(const VuoTempoRange valueA, const VuoTempoRange valueB)
{
	return valueA < valueB;
}
