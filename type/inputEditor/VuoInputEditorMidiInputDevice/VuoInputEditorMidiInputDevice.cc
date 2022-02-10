/**
 * @file
 * VuoInputEditorMidiInputDevice implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoInputEditorMidiInputDevice.hh"

extern "C"
{
	#include "VuoMidi.h"
}

#include <CoreMIDI/CoreMIDI.h>
#include "VuoOsStatus.h"

/**
 * Initialize CoreMIDI when the dylib is loaded,
 * so the input editor shows up immediately the first time.
 */
static void __attribute__((constructor)) VuoInputEditorMidiInputDevice_init()
{
	dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
		MIDIClientRef client;
		OSStatus result = MIDIClientCreate( CFStringCreateWithCString( NULL, "blah", kCFStringEncodingASCII ), NULL, NULL, &client);
		if (result != noErr)
			VUserLog("Warning: Apple CoreMIDI initialization failed: %s", VuoOsStatus_getText(result));
	});
}

/**
 * Constructs a VuoInputEditorMidiInputDevice object.
 */
VuoInputEditor * VuoInputEditorMidiInputDeviceFactory::newInputEditor()
{
	return new VuoInputEditorMidiInputDevice();
}

/**
 * Creates the tree that models the menu.
 */
VuoInputEditorMenuItem * VuoInputEditorMidiInputDevice::setUpMenuTree()
{
	VuoInputEditorMenuItem *optionsTree = new VuoInputEditorMenuItem("root");

	optionsTree->addItem(new VuoInputEditorMenuItem("First available device", NULL));
	optionsTree->addSeparator();
	optionsTree->addItem(new VuoInputEditorMenuItem("Specific device", NULL, NULL, false));

	VuoList_VuoMidiInputDevice devices = VuoMidi_getInputDevices();
	unsigned long deviceCount = VuoListGetCount_VuoMidiInputDevice(devices);

	if (deviceCount)
		for (unsigned long i = 1; i <= deviceCount; ++i)
		{
			VuoMidiInputDevice device = VuoListGetValue_VuoMidiInputDevice(devices, i);
			device.id = -1;
			optionsTree->addItem(new VuoInputEditorMenuItem(VuoText_format("      %s", device.name), VuoMidiInputDevice_getJson(device)));
		}
	else
		optionsTree->addItem(new VuoInputEditorMenuItem("      (no devices found)", NULL, NULL, false));


	return optionsTree;
}
