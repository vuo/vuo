/**
 * @file
 * vuo.font.make node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoFont.h"

VuoModuleMetadata({
					 "title" : "Make Font",
					 "version" : "1.0.0",
					 "node": {
						 "exampleCompositions": [ "RenderMovieTrailer.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoText, {"default":"Helvetica"}) fontName,
		VuoInputData(VuoReal, {"default":28.,"suggestedMin":0.0001,"suggestedMax":512.}) pointSize,
		VuoInputData(VuoBoolean, {"default":false}) underlined,
		VuoInputData(VuoColor,{"default":{"r":1,"g":1,"b":1,"a":1}}) color,
		/// @todo add alignment port + input editor
		VuoInputData(VuoReal, {"default":1.,"suggestedMin":0.,"suggestedMax":2.}) characterSpacing,
		VuoInputData(VuoReal, {"default":1.,"suggestedMin":0.,"suggestedMax":2.}) lineSpacing,
		VuoOutputData(VuoFont) font
)
{
	*font = VuoFont_make(fontName, pointSize, underlined, color, VuoHorizontalAlignment_Left, characterSpacing, lineSpacing);
}
