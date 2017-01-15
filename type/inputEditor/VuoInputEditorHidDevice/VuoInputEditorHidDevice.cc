/**
 * @file
 * VuoInputEditorHidDevice implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorHidDevice.hh"

extern "C"
{
	#include "VuoHid.h"
}

/**
 * Constructs a VuoInputEditorHidDevice object.
 */
VuoInputEditor * VuoInputEditorHidDeviceFactory::newInputEditor()
{
	return new VuoInputEditorHidDevice();
}

/**
 * Creates the tree that models the menu.
 */
VuoInputEditorMenuItem * VuoInputEditorHidDevice::setUpMenuTree()
{
	VuoInputEditorMenuItem *optionsTree = new VuoInputEditorMenuItem("root");

	optionsTree->addItem(new VuoInputEditorMenuItem("None", NULL));

	optionsTree->addSeparator();
	optionsTree->addItem(new VuoInputEditorMenuItem("Specific device", NULL, NULL, false));

	VuoList_VuoHidDevice devices = VuoHid_getDeviceList();

	unsigned long deviceCount = VuoListGetCount_VuoHidDevice(devices);
	if (deviceCount)
		for (unsigned long i = 1; i <= deviceCount; ++i)
		{
			VuoHidDevice device = VuoListGetValue_VuoHidDevice(devices, i);
			optionsTree->addItem(new VuoInputEditorMenuItem(VuoText_format("      %s", device.name), VuoHidDevice_getJson(device)));
		}
	else
		optionsTree->addItem(new VuoInputEditorMenuItem("      (no devices found)", NULL, NULL, false));

	return optionsTree;
}
