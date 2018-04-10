/**
 * @file
 * VuoInputEditorAudioInputDevice implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorAudioInputDevice.hh"

extern "C"
{
#include "VuoAudio.h"
}

/**
 * Constructs a VuoInputEditorAudioInputDevice object.
 */
VuoInputEditor * VuoInputEditorAudioInputDeviceFactory::newInputEditor()
{
	return new VuoInputEditorAudioInputDevice();
}

/**
 * Creates the tree that models the menu.
 */
VuoInputEditorMenuItem * VuoInputEditorAudioInputDevice::setUpMenuTree()
{
	VuoInputEditorMenuItem *optionsTree = new VuoInputEditorMenuItem("root");

	optionsTree->addItem(new VuoInputEditorMenuItem("Default device", NULL));

	optionsTree->addSeparator();
	optionsTree->addItem(new VuoInputEditorMenuItem("Specific device (by model)", NULL, NULL, false));

	VuoList_VuoAudioInputDevice devices = VuoAudio_getInputDevices();

	unsigned long deviceCount = VuoListGetCount_VuoAudioInputDevice(devices);
	if (deviceCount)
		for (unsigned long i = 1; i <= deviceCount; ++i)
		{
			VuoAudioInputDevice device = VuoListGetValue_VuoAudioInputDevice(devices, i);

			json_object *deviceJson = json_object_new_object();
			json_object_object_add(deviceJson, "modelUid", VuoText_getJson(device.modelUid));
			json_object_object_add(deviceJson, "name", VuoText_getJson(device.name));

			optionsTree->addItem(new VuoInputEditorMenuItem(VuoText_format("      %s (%lld input channels)", device.name, device.channelCount), deviceJson));
		}
	else
		optionsTree->addItem(new VuoInputEditorMenuItem("      (no devices found)", NULL, NULL, false));

	return optionsTree;
}
