/**
 * @file
 * ExampleLanguage interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef EXAMPLELANGUAGE_H
#define EXAMPLELANGUAGE_H

typedef enum {
	ExampleLanguage_English,
	ExampleLanguage_Spanish,
	ExampleLanguage_Mandarin
} ExampleLanguage;

ExampleLanguage ExampleLanguage_valueFromJson(struct json_object * js);
struct json_object * ExampleLanguage_jsonFromValue(const ExampleLanguage value);
char * ExampleLanguage_summaryFromValue(const ExampleLanguage value);

#endif
