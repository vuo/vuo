/**
 * @file
 * vuo.transform.get.quaternion node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					  "title" : "Get Transform Quaternion",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "node": {
						  "exampleCompositions" : [ ]
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoTransform) transform,
		VuoOutputData(VuoPoint4d) quaternion
)
{
	*quaternion = VuoTransform_getQuaternion(transform);
}
