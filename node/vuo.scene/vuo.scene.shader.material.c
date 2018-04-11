/**
 * @file
 * vuo.scene.shader.material node implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoSceneObject.h"

VuoModuleMetadata({
					 "title" : "Change Shaders with Name",
					  "keywords" : [ "swap", "replace", "texture" ],
					 "version" : "2.0.1",
					 "node": {
						  "exampleCompositions" : [ "ChangeTrafficSignal.vuo" ]
					 }
				 });

void nodeEvent
(
	VuoInputData(VuoSceneObject) object,
	VuoInputData(VuoText, {"name":"Shader Name"}) material,
	VuoInputData(VuoShader) shader,
	VuoOutputData(VuoSceneObject) shadedObject
)
{
	VuoSceneObject copy = VuoSceneObject_copy(object);

	VuoShader s = shader;
	if (!s)
		s = VuoShader_makeDefaultShader();

	bool applyToAll = !material || strlen(material) == 0;
	VuoSceneObject_apply(&copy, ^(VuoSceneObject *currentObject, float modelviewMatrix[16]){
							if (!currentObject->shader)
								return;

							if (applyToAll || strstr(currentObject->shader->name, material))
								currentObject->shader = s;
						 });

	*shadedObject = copy;
}
