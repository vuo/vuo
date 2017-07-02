/**
 * @file
 * VuoInputEditorSyphonServerDescription implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorSyphonServerDescription.hh"

extern "C"
{
	#include "VuoSyphon.h"
}

#include <set>
#include <string>
using namespace std;

/**
 * Constructs a VuoInputEditorSyphonServerDescription object.
 */
VuoInputEditor * VuoInputEditorSyphonServerDescriptionFactory::newInputEditor()
{
	return new VuoInputEditorSyphonServerDescription();
}

/**
 * Creates the tree that models the menu.
 */
VuoInputEditorMenuItem * VuoInputEditorSyphonServerDescription::setUpMenuTree()
{
	VuoInputEditorMenuItem *optionsTree = new VuoInputEditorMenuItem("root");

	optionsTree->addItem(new VuoInputEditorMenuItem("First available server", NULL));
	optionsTree->addSeparator();
	optionsTree->addItem(new VuoInputEditorMenuItem("Specific server (by name)", NULL, NULL, false));

	VuoList_VuoSyphonServerDescription devices = VuoSyphon_getAvailableServerDescriptions();
	unsigned long deviceCount = VuoListGetCount_VuoSyphonServerDescription(devices);
	set<string> uniqueServerNames, uniqueApplicationNames;
	for (unsigned long i = 1; i <= deviceCount; ++i)
	{
		VuoSyphonServerDescription device = VuoListGetValue_VuoSyphonServerDescription(devices, i);
		if (device.serverName && strlen(device.serverName))
			uniqueServerNames.insert(device.serverName);
		if (device.applicationName && strlen(device.applicationName))
			uniqueApplicationNames.insert(device.applicationName);
	}

	if (!uniqueServerNames.empty())
		for (set<string>::iterator it = uniqueServerNames.begin(); it != uniqueServerNames.end(); ++it)
			optionsTree->addItem(new VuoInputEditorMenuItem(VuoText_format("      %s", it->c_str()), VuoSyphonServerDescription_getJson(VuoSyphonServerDescription_make(NULL, it->c_str(), NULL))));
	else
		optionsTree->addItem(new VuoInputEditorMenuItem("      (no servers found)", NULL, NULL, false));

	optionsTree->addSeparator();
	optionsTree->addItem(new VuoInputEditorMenuItem("Specific server (by app)", NULL, NULL, false));

	if (!uniqueApplicationNames.empty())
		for (set<string>::iterator it = uniqueApplicationNames.begin(); it != uniqueApplicationNames.end(); ++it)
			optionsTree->addItem(new VuoInputEditorMenuItem(VuoText_format("      %s", it->c_str()), VuoSyphonServerDescription_getJson(VuoSyphonServerDescription_make(NULL, NULL, it->c_str()))));
	else
		optionsTree->addItem(new VuoInputEditorMenuItem("      (no servers found)", NULL, NULL, false));

	return optionsTree;
}
