/**
 * @file
 * vuo.text.format.number node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoNumberFormat.h"

VuoModuleMetadata({
					  "title" : "Format Number",
					  "keywords" : [
						  "currency", "dollar", "euro", "percentage", "round",
						  "convert", "real", // so it shows up when searching for "convert real text"
					  ],
					  "version" : "1.1.0",
					  "dependencies" : [],
					  "node" : {
						  "exampleCompositions" : [ "ShowMousePosition.vuo" ]
					  }
				  });

void nodeEvent
(
	VuoInputData(VuoReal, {"default":"1.0"}) value,
	VuoInputData(VuoNumberFormat, {"default":"decimal"}) format,
	VuoInputData(VuoInteger, {"default":0, "suggestedMin":0}) minimumIntegerDigits,
	VuoInputData(VuoInteger, {"default":0, "suggestedMin":0}) minimumDecimalPlaces,
	VuoInputData(VuoInteger, {"default":2, "suggestedMin":0, "name":"Maximum Decimal Places"}) decimalPlaces,
	VuoInputData(VuoBoolean, {"default":true}) showThousandSeparator,
	VuoOutputData(VuoText) text
)
{
	*text =	VuoNumberFormat_format(value, format, minimumIntegerDigits, minimumDecimalPlaces, decimalPlaces, showThousandSeparator);
}
