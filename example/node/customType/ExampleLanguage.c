/**
 * @file
 * ExampleLanguage implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <string.h>
#include "type.h"
#include "ExampleLanguage.h"

VuoModuleMetadata({
					  "title" : "Language",
					  "version" : "1.0.0"
				  });

ExampleLanguage ExampleLanguage_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	ExampleLanguage value = ExampleLanguage_English;
	if (! strcmp(valueAsString, "english"))
		value = ExampleLanguage_English;
	else if (! strcmp(valueAsString, "spanish"))
		value = ExampleLanguage_Spanish;
	else if (! strcmp(valueAsString, "mandarin"))
		value = ExampleLanguage_Mandarin;

	return value;
}

json_object * ExampleLanguage_getJson(const ExampleLanguage value)
{
	char *valueAsString = "";

	switch (value) {
		case ExampleLanguage_English:
			valueAsString = "english";
			break;
		case ExampleLanguage_Spanish:
			valueAsString = "spanish";
			break;
		case ExampleLanguage_Mandarin:
			valueAsString = "mandarin";
	}

	return json_object_new_string(valueAsString);
}

char * ExampleLanguage_getSummary(const ExampleLanguage value)
{
	char *valueAsString = "";

	switch (value) {
		case ExampleLanguage_English:
			valueAsString = "English";
			break;
		case ExampleLanguage_Spanish:
			valueAsString = "Español";
			break;
		case ExampleLanguage_Mandarin:
			valueAsString = "官话";
			break;
	}

	return strdup(valueAsString);
}
