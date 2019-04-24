/**
 * @file
 * VuoInputEditorArtNetOutputDevice implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorArtNetOutputDevice.hh"

extern "C"
{
#include "VuoArtNet.h"
}

/**
 * Whether we've started listening for available Art-Net Output devices.
 */
static bool listening = false;

/**
 * Constructs a VuoInputEditorArtNetOutputDevice object.
 */
VuoInputEditor * VuoInputEditorArtNetOutputDeviceFactory::newInputEditor()
{
	if (!listening)
	{
		VuoArtNet_use();
		listening = true;
	}

	return new VuoInputEditorArtNetOutputDevice();
}

/**
 * Creates the tree that models the menu.
 */
VuoInputEditorMenuItem * VuoInputEditorArtNetOutputDevice::setUpMenuTree()
{
	VuoInputEditorMenuItem *optionsTree = new VuoInputEditorMenuItem("root");

	optionsTree->addItem(new VuoInputEditorMenuItem("Broadcast (net 0, sub-net 0, universe 0)", NULL));

	optionsTree->addSeparator();
	optionsTree->addItem(new VuoInputEditorMenuItem("Unicast", NULL, NULL, false));

	VuoList_VuoArtNetOutputDevice devices = VuoArtNet_getOutputDevices();

	unsigned long deviceCount = VuoListGetCount_VuoArtNetOutputDevice(devices);
	if (deviceCount)
		for (unsigned long i = 1; i <= deviceCount; ++i)
		{
			VuoArtNetOutputDevice device = VuoListGetValue_VuoArtNetOutputDevice(devices, i);
			optionsTree->addItem(new VuoInputEditorMenuItem(VuoText_format("      %s (net %lld, sub-net %lld, universe %lld) (%s)",
																		   device.name,
																		   device.address.net,
																		   device.address.subNet,
																		   device.address.universe,
																		   device.ipAddress),
															VuoArtNetOutputDevice_getJson(device)));
		}
	else
		optionsTree->addItem(new VuoInputEditorMenuItem("      (no devices found)", NULL, NULL, false));

	return optionsTree;
}
