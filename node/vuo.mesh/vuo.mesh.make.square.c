/**
 * @file
 * vuo.vertices.make.square node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					 "title" : "Make Square Mesh",
					 "keywords" : [ "quad", "rectangle", "plane", "4-gon", "4gon", "shape", "billboard", "sprite" ],
					 "version" : "2.0.0",
					 "node": {
						  "exampleCompositions" : [ ],
						  "isDeprecated": true
					 }
				 });

void nodeEvent
(
		VuoOutputData(VuoMesh) mesh
)
{
	*mesh = VuoMesh_makeQuad();
}
