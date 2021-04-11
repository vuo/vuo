/**
 * @file
 * VuoInputEditorVideoInputDevice implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoInputEditorVideoInputDevice.hh"

extern "C"
{
	#include "VuoVideoInputDevice.h"
	#include "VuoVideoCapture.h"
}

/**
 * Enable tethering immediately when the dylib is loaded,
 * so tethered devices show up the first time the input editor menu is shown.
 */
void __attribute__((constructor)) VuoInputEditorVideoInputDevice_init()
{
	VuoVideoCapture_getInputDevices();
}

/**
 * Constructs a VuoInputEditorVideoInputDevice object.
 */
VuoInputEditor * VuoInputEditorVideoInputDeviceFactory::newInputEditor()
{
	return new VuoInputEditorVideoInputDevice();
}

/**
 * Creates the tree that models the menu.
 */
VuoInputEditorMenuItem * VuoInputEditorVideoInputDevice::setUpMenuTree()
{
	VuoInputEditorMenuItem *optionsTree = new VuoInputEditorMenuItem("root");

	optionsTree->addItem(new VuoInputEditorMenuItem("Default device", NULL));
	optionsTree->addSeparator();
	optionsTree->addItem(new VuoInputEditorMenuItem("Specific device", NULL, NULL, false));

	VuoList_VuoVideoInputDevice devices = VuoVideoCapture_getInputDevices();
	unsigned long deviceCount = VuoListGetCount_VuoVideoInputDevice(devices);

	if (deviceCount)
		for (unsigned long i = 1; i <= deviceCount; ++i)
		{
			VuoVideoInputDevice device = VuoListGetValue_VuoVideoInputDevice(devices, i);

			// https://b33p.net/kosada/node/9583
			// When selecting a device via the input editor, only match by ID,
			// since one device's name ("Logitech Camera") might be a substring of another device's name ("Logitech Camera #2").
			device.matchType = VuoVideoInputDevice_MatchId;

			optionsTree->addItem(new VuoInputEditorMenuItem(VuoText_format("      %s", device.name), VuoVideoInputDevice_getJson(device)));
		}
	else
		optionsTree->addItem(new VuoInputEditorMenuItem("      (no devices found)", NULL, NULL, false));

	return optionsTree;
}
