/**
 * @file
 * VuoInputEditorOscInputDevice implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorOscInputDevice.hh"

extern "C"
{
#include "VuoOsc.h"
}

/**
 * Listen for available OSC Input devices.
 */
static void __attribute__((constructor)) VuoInputEditorOscInputDevice_init(void)
{
	VuoOsc_use();
}

/**
 * Constructs a VuoInputEditorOscInputDevice object.
 */
VuoInputEditor * VuoInputEditorOscInputDeviceFactory::newInputEditor()
{
	return new VuoInputEditorOscInputDevice();
}

/**
 * Creates the tree that models the menu.
 */
VuoInputEditorMenuItem * VuoInputEditorOscInputDevice::setUpMenuTree()
{
	VuoInputEditorMenuItem *optionsTree = new VuoInputEditorMenuItem("root");

	optionsTree->addItem(new VuoInputEditorMenuItem("Automatic", NULL));

	optionsTree->addSeparator();
	optionsTree->addItem(new VuoInputEditorMenuItem("Specific device", NULL, NULL, false));

	VuoList_VuoOscInputDevice devices = VuoOsc_getInputDeviceList();

	unsigned long deviceCount = VuoListGetCount_VuoOscInputDevice(devices);
	if (deviceCount)
		for (unsigned long i = 1; i <= deviceCount; ++i)
		{
			VuoOscInputDevice device = VuoListGetValue_VuoOscInputDevice(devices, i);
			optionsTree->addItem(new VuoInputEditorMenuItem(VuoText_format("      %s (%s:%lld)", device.name, device.ipAddress, device.port), VuoOscInputDevice_getJson(device)));
		}
	else
		optionsTree->addItem(new VuoInputEditorMenuItem("      (no devices found)", NULL, NULL, false));

	return optionsTree;
}
