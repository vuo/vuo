/**
 * @file
 * VuoInputEditorOscOutputDevice implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorOscOutputDevice.hh"

extern "C"
{
#include "VuoOsc.h"
}

/**
 * Listen for available OSC output devices.
 */
static void __attribute__((constructor)) VuoInputEditorOscOutputDevice_init(void)
{
	VuoOsc_use();
}

/**
 * Constructs a VuoInputEditorOscOutputDevice object.
 */
VuoInputEditor * VuoInputEditorOscOutputDeviceFactory::newInputEditor()
{
	return new VuoInputEditorOscOutputDevice();
}

/**
 * Creates the tree that models the menu.
 */
VuoInputEditorMenuItem * VuoInputEditorOscOutputDevice::setUpMenuTree()
{
	VuoInputEditorMenuItem *optionsTree = new VuoInputEditorMenuItem("root");

	optionsTree->addItem(new VuoInputEditorMenuItem("Automatic", NULL));

	optionsTree->addSeparator();
	optionsTree->addItem(new VuoInputEditorMenuItem("Specific device", NULL, NULL, false));

	VuoList_VuoOscOutputDevice devices = VuoOsc_getOutputDeviceList();

	unsigned long deviceCount = VuoListGetCount_VuoOscOutputDevice(devices);
	if (deviceCount)
		for (unsigned long i = 1; i <= deviceCount; ++i)
		{
			VuoOscOutputDevice device = VuoListGetValue_VuoOscOutputDevice(devices, i);
			optionsTree->addItem(new VuoInputEditorMenuItem(VuoText_format("      %s (%s:%lld)", device.name, device.ipAddress, device.port), VuoOscOutputDevice_getJson(device)));
		}
	else
		optionsTree->addItem(new VuoInputEditorMenuItem("      (no devices found)", NULL, NULL, false));

	return optionsTree;
}
