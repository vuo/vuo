/**
 * @file
 * VuoTableFormat implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoTableFormat.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Table Format",
					 "description" : "A text format for information structured in rows and columns",
					 "keywords" : [],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoList_VuoTableFormat"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoTableFormat
 * Decodes the JSON object @a js, expected to contain a string, to create a new `VuoTableFormat`.
 */
VuoTableFormat VuoTableFormat_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoTableFormat value = VuoTableFormat_Csv;

	if (! strcmp(valueAsString, "csv")) {
		value = VuoTableFormat_Csv;
	} else if (! strcmp(valueAsString, "tsv")) {
		value = VuoTableFormat_Tsv;
	}

	return value;
}

/**
 * @ingroup VuoTableFormat
 * Encodes @a value as a JSON object.
 */
json_object * VuoTableFormat_getJson(const VuoTableFormat value)
{
	char *valueAsString = "";

	switch (value) {
		case VuoTableFormat_Csv:
			valueAsString = "csv";
			break;
		case VuoTableFormat_Tsv:
			valueAsString = "tsv";
			break;
	}

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoTableFormat VuoTableFormat_getAllowedValues(void)
{
	VuoList_VuoTableFormat l = VuoListCreate_VuoTableFormat();
	VuoListAppendValue_VuoTableFormat(l, VuoTableFormat_Csv);
	VuoListAppendValue_VuoTableFormat(l, VuoTableFormat_Tsv);
	return l;
}

/**
 * @ingroup VuoTableFormat
 * Returns a human-readable description of @a value.
 */
char * VuoTableFormat_getSummary(const VuoTableFormat value)
{
	char *valueAsString = "";

	switch (value) {
		case VuoTableFormat_Csv:
			valueAsString = "CSV (comma-separated)";
			break;
		case VuoTableFormat_Tsv:
			valueAsString = "TSV (tab-separated)";
			break;
	}

	return strdup(valueAsString);
}
