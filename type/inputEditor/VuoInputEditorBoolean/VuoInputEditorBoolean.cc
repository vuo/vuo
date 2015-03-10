/**
 * @file
 * VuoInputEditorBoolean implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorBoolean.hh"

extern "C"
{
	#include "VuoBoolean.h"
}

/**
 * Constructs a VuoInputEditorBoolean object.
 */
VuoInputEditor * VuoInputEditorBooleanFactory::newInputEditor()
{
	return new VuoInputEditorBoolean();
}

/**
 * Creates the tree that models the menu.
 */
VuoInputEditorMenuItem * VuoInputEditorBoolean::setUpMenuTree()
{
	VuoInputEditorMenuItem *rootMenuItem = new VuoInputEditorMenuItem("root");

	rootMenuItem->addItem(new VuoInputEditorMenuItem(VuoBoolean_summaryFromValue(false), VuoBoolean_jsonFromValue(false)));
	rootMenuItem->addItem(new VuoInputEditorMenuItem(VuoBoolean_summaryFromValue(true), VuoBoolean_jsonFromValue(true)));

	return rootMenuItem;
}
