﻿/**
 * @file
 * vuo.type.integer.real node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					 "title" : "Convert Integer to Real Number",
					 "keywords" : [ ],
					 "version" : "1.0.0"
				 });

void nodeEvent
(
	VuoInputData(VuoInteger, {"default":0}) integer,
	VuoOutputData(VuoReal) real
)
{
	*real = integer;
}
