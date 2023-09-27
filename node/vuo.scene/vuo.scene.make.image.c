/**
 * @file
 * vuo.scene.make.image node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					 "title" : "Make 3D Object from Image",
					 "keywords" : [ "quad", "rectangle", "plane", "4-gon", "4gon", "billboard", "sprite", "square",
						 "projector",
						 "lighting", "lit", "lighted",
						 "Blinn", "Phong", "Lambert" ],
					 "version" : "3.1.0",
					 "node": {
						 "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoImage) image,
		VuoInputData(VuoPoint3d, {"default":{"x":0.0,"y":0.0,"z":0.0},
								  "suggestedMin":{"x":-2.0,"y":-2.0,"z":-2.0},
								  "suggestedMax":{"x":2.0,"y":2.0,"z":2.0},
								  "suggestedStep":{"x":0.1,"y":0.1,"z":0.1}}) center,
		VuoInputData(VuoPoint3d, {"default":{"x":0.0,"y":0.0,"z":0.0},
								  "suggestedMin":{"x":-180,"y":-180,"z":-180},
								  "suggestedMax":{"x":180,"y":180,"z":180},
								  "suggestedStep":{"x":15.0,"y":15.0,"z":15.0}}) rotation,
		VuoInputData(VuoReal, {"name":"Size", "default":1.0, "suggestedMin":0.0, "suggestedStep":0.1}) width,
		VuoInputData(VuoOrientation, {"default":"horizontal"}) fixed,
		VuoInputData(VuoReal, {"default":1.0, "suggestedMin":0.0, "suggestedMax":1.0, "suggestedStep":0.1}) opacity,
		VuoInputData(VuoColor,{"default":{"r":1.,"g":1.,"b":1.,"a":1.}}) highlightColor,
		VuoInputData(VuoReal,{"default":0.9, "suggestedMin":0.0, "suggestedMax":1.0, "suggestedStep":0.1}) shininess,
		VuoOutputData(VuoSceneObject) object
)
{
	*object = VuoSceneObject_makeLitImage(image, center, rotation, width, fixed, opacity, highlightColor, shininess);
}
