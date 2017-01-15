/**
 * @file
 * example.greet node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "ExampleLanguage.h"

VuoModuleMetadata({
					 "title" : "Greet",
					 "description" : "Outputs a greeting in the selected language.",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "dependencies" : [ ],
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(ExampleLanguage, {"default":"english"}) language,
		VuoOutputData(VuoText) greeting
)
{
	const char *greetingAsChars = "";
	switch (language)
	{
		case ExampleLanguage_English :
			greetingAsChars = "Hello";
			break;
		case ExampleLanguage_Spanish :
			greetingAsChars = "Hola";
			break;
		case ExampleLanguage_Mandarin :
			greetingAsChars = "你好";
			break;
	}
	*greeting = VuoText_make(greetingAsChars);
}
