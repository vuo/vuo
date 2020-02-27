/**
 * @file
 * ExampleLanguage interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

typedef enum {
	ExampleLanguage_English,
	ExampleLanguage_Spanish,
	ExampleLanguage_Mandarin
} ExampleLanguage;

ExampleLanguage ExampleLanguage_makeFromJson(struct json_object * js);
struct json_object * ExampleLanguage_getJson(const ExampleLanguage value);
char * ExampleLanguage_getSummary(const ExampleLanguage value);
