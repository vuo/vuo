/**
 * @file
 * VuoInputEditorAudioOutputDevice implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorAudioOutputDevice.hh"

extern "C"
{
#include "VuoAudio.h"
}

/**
 * Constructs a VuoInputEditorAudioOutputDevice object.
 */
VuoInputEditor * VuoInputEditorAudioOutputDeviceFactory::newInputEditor()
{
	return new VuoInputEditorAudioOutputDevice();
}

/**
 * Creates the tree that models the menu.
 */
VuoInputEditorMenuItem * VuoInputEditorAudioOutputDevice::setUpMenuTree()
{
	VuoInputEditorMenuItem *optionsTree = new VuoInputEditorMenuItem("root");

	optionsTree->addItem(new VuoInputEditorMenuItem("Default device", NULL));

	optionsTree->addSeparator();
	optionsTree->addItem(new VuoInputEditorMenuItem("Specific device", NULL, NULL, false));

	VuoList_VuoAudioOutputDevice devices = VuoAudio_getOutputDevices();

	unsigned long deviceCount = VuoListGetCount_VuoAudioOutputDevice(devices);
	if (deviceCount)
		for (unsigned long i = 1; i <= deviceCount; ++i)
		{
			VuoAudioOutputDevice device = VuoListGetValue_VuoAudioOutputDevice(devices, i);

			json_object *deviceJson = json_object_new_object();
			json_object_object_add(deviceJson, "name", VuoText_getJson(device.name));

			optionsTree->addItem(new VuoInputEditorMenuItem(VuoText_format("      %s (%lld output channels)", device.name, device.channelCount), deviceJson));
		}
	else
		optionsTree->addItem(new VuoInputEditorMenuItem("      (no devices found)", NULL, NULL, false));

	return optionsTree;
}
