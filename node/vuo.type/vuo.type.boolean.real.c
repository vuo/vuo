/**
 * @file
 * vuo.type.boolean.real node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					 "title" : "Convert Boolean to Real",
					 "keywords" : [ "0", "1", "true", "false" ],
					 "version" : "1.0.0"
				 });

void nodeEvent
(
	VuoInputData(VuoBoolean, {"default":false}) boolean,
	VuoOutputData(VuoReal) real
)
{
	*real = boolean ? 1 : 0;
}
