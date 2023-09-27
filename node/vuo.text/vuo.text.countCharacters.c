/**
 * @file
 * vuo.text.countCharacters node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					 "title" : "Count Characters",
					 "keywords" : [ "letter", "length", "size", "strlen", "string" ],
					 "version" : "1.0.0",
					 "node": {
						  "exampleCompositions" : [ "CountCharactersInGreetings.vuo", "RevealWords.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoText, {"default":""}) text,
		VuoOutputData(VuoInteger) characterCount
)
{
	*characterCount = VuoText_length(text);
}
