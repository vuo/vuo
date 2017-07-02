/**
 * @file
 * VuoInputEditorMidiOutputDevice implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorMidiOutputDevice.hh"

extern "C"
{
	#include "VuoMidi.h"
}

#include<CoreMIDI/CoreMIDI.h>
#include "VuoOsStatus.h"

/**
 * Initialize CoreMIDI when the dylib is loaded,
 * so the input editor shows up immediately the first time.
 */
static void __attribute__((constructor)) VuoInputEditorMidiOutputDevice_init()
{
	MIDIClientRef client;
	OSStatus result = MIDIClientCreate( CFStringCreateWithCString( NULL, "blah", kCFStringEncodingASCII ), NULL, NULL, &client);
	if (result != noErr)
		VUserLog("Warning: Apple CoreMIDI initialization failed: %s", VuoOsStatus_getText(result));
}

/**
 * Constructs a VuoInputEditorMidiOutputDevice object.
 */
VuoInputEditor * VuoInputEditorMidiOutputDeviceFactory::newInputEditor()
{
	return new VuoInputEditorMidiOutputDevice();
}

/**
 * Creates the tree that models the menu.
 */
VuoInputEditorMenuItem * VuoInputEditorMidiOutputDevice::setUpMenuTree()
{
	VuoInputEditorMenuItem *optionsTree = new VuoInputEditorMenuItem("root");

	optionsTree->addItem(new VuoInputEditorMenuItem("First available device", NULL));
	optionsTree->addSeparator();
	optionsTree->addItem(new VuoInputEditorMenuItem("Specific device", NULL, NULL, false));

	VuoList_VuoMidiOutputDevice devices = VuoMidi_getOutputDevices();
	unsigned long deviceCount = VuoListGetCount_VuoMidiOutputDevice(devices);

	if (deviceCount)
		for (unsigned long i = 1; i <= deviceCount; ++i)
		{
			VuoMidiOutputDevice device = VuoListGetValue_VuoMidiOutputDevice(devices, i);
			device.id = -1;
			optionsTree->addItem(new VuoInputEditorMenuItem(VuoText_format("      %s", device.name), VuoMidiOutputDevice_getJson(device)));
		}
	else
		optionsTree->addItem(new VuoInputEditorMenuItem("      (no devices found)", NULL, NULL, false));


	return optionsTree;
}
