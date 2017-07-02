/**
 * @file
 * VuoInputEditorArtNetInputDevice implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorArtNetInputDevice.hh"

extern "C"
{
#include "VuoArtNet.h"
}

/**
 * Whether we've started listening for available Art-Net Input devices.
 */
static bool listening = false;

/**
 * Constructs a VuoInputEditorArtNetInputDevice object.
 */
VuoInputEditor * VuoInputEditorArtNetInputDeviceFactory::newInputEditor()
{
	if (!listening)
	{
		VuoArtNet_use();
		listening = true;
	}

	return new VuoInputEditorArtNetInputDevice();
}

/**
 * Creates the tree that models the menu.
 */
VuoInputEditorMenuItem * VuoInputEditorArtNetInputDevice::setUpMenuTree()
{
	VuoInputEditorMenuItem *optionsTree = new VuoInputEditorMenuItem("root");

	optionsTree->addItem(new VuoInputEditorMenuItem("Any device (net 0, sub-net 0, universe 0)", NULL));

	optionsTree->addSeparator();
	optionsTree->addItem(new VuoInputEditorMenuItem("Specific device", NULL, NULL, false));

	VuoList_VuoArtNetInputDevice devices = VuoArtNet_getInputDevices();

	unsigned long deviceCount = VuoListGetCount_VuoArtNetInputDevice(devices);
	if (deviceCount)
		for (unsigned long i = 1; i <= deviceCount; ++i)
		{
			VuoArtNetInputDevice device = VuoListGetValue_VuoArtNetInputDevice(devices, i);
			optionsTree->addItem(new VuoInputEditorMenuItem(VuoText_format("      %s (net %lld, sub-net %lld, universe %lld) (%s)",
																		   device.name,
																		   device.address.net,
																		   device.address.subNet,
																		   device.address.universe,
																		   device.ipAddress),
															VuoArtNetInputDevice_getJson(device)));
		}
	else
		optionsTree->addItem(new VuoInputEditorMenuItem("      (no devices found)", NULL, NULL, false));

	return optionsTree;
}
