/**
 * @file
 * VuoInputEditorSerialDevice implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorSerialDevice.hh"

extern "C"
{
	#include "VuoSerial.h"
}

/**
 * Constructs a VuoInputEditorSerialDevice object.
 */
VuoInputEditor * VuoInputEditorSerialDeviceFactory::newInputEditor()
{
	return new VuoInputEditorSerialDevice();
}

/**
 * Creates the tree that models the menu.
 */
VuoInputEditorMenuItem * VuoInputEditorSerialDevice::setUpMenuTree()
{
	VuoInputEditorMenuItem *optionsTree = new VuoInputEditorMenuItem("root");

	{
		VuoSerialDevice device;
		device.matchType = VuoSerialDevice_MatchName;
		device.name = VuoText_make("");
		device.path = NULL;
		VuoSerialDevice_retain(device);
		optionsTree->addItem(new VuoInputEditorMenuItem("First available device", VuoSerialDevice_getJson(device)));
		VuoSerialDevice_release(device);
	}

	optionsTree->addSeparator();
	optionsTree->addItem(new VuoInputEditorMenuItem("Specific device", NULL, NULL, false));

	VuoList_VuoSerialDevice devices = VuoSerial_getDeviceList();

	unsigned long deviceCount = VuoListGetCount_VuoSerialDevice(devices);
	if (deviceCount)
		for (unsigned long i = 1; i <= deviceCount; ++i)
		{
			VuoSerialDevice device = VuoListGetValue_VuoSerialDevice(devices, i);
			optionsTree->addItem(new VuoInputEditorMenuItem(VuoText_format("      %s", device.name), VuoSerialDevice_getJson(device)));
		}
	else
		optionsTree->addItem(new VuoInputEditorMenuItem("      (no devices found)", NULL, NULL, false));

	return optionsTree;
}
