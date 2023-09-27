/**
 * @file
 * vuo.event.fireWhenValueEdited node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
	"title": "Fire when Value Edited",
	"keywords": [
		"input editor", "inspector", "Vuo editor", "double-click", "port",
		"debug", "troubleshoot",
		"slider", "knob", "track bar", "widget", "control", "GUI", "UI",
		"changed", "changing", "modified", "modifying", "altered", "adjusted", "watcher",
		"spin off", "spawn", "input splitter", "output splitter", "data",
		"pass", "preserve", "keep", "constant", "identity", "convert", "make", "create",
		"minimum", "maximum", "multiply", "range", "mix", "blend", "lerp", "interpolate",
	],
	"version": "1.0.0",
	"node": {
		"exampleCompositions": [ "ScaleRectangleWhenValueEdited.vuo" ],
	},
});

bool *nodeInstanceInit(void)
{
	return NULL;
}

void nodeInstanceTriggerUpdate(
	VuoInstanceData(bool *) instance,
	VuoInputData(VuoReal, {"default":0,"suggestedMin":0,"suggestedMax":1,"suggestedStep":0.01}) value,
	VuoInputData(VuoRange, {"default":{"minimum":0,"maximum":1},
							"requireMin":true, "requireMax":true}) scale,
	VuoOutputTrigger(valueEdited, VuoReal))
{
	valueEdited(VuoReal_lerp(scale.minimum, scale.maximum, value));
}

void nodeInstanceEvent(VuoInstanceData(bool *) instance)
{
}
