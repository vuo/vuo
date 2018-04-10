/**
 * @file
 * VuoInputEditorCurveEasing implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorCurveEasing.hh"

extern "C"
{
#include "VuoCurve.h"
#include "VuoCurveEasing.h"
}

#include "../VuoInputEditorCurve/VuoInputEditorCurveRenderer.hh"

/**
 * Constructs a VuoInputEditorCurveEasing object.
 */
VuoInputEditor * VuoInputEditorCurveEasingFactory::newInputEditor()
{
	return new VuoInputEditorCurveEasing();
}

/**
 * Creates the tree that models the menu.
 */
VuoInputEditorMenuItem * VuoInputEditorCurveEasing::setUpMenuTree()
{
	VuoInputEditorMenuItem *rootMenuItem = new VuoInputEditorMenuItem("root");

	for (VuoCurveEasing i = VuoCurveEasing_In; i <= VuoCurveEasing_Middle; ++i)
	{
		json_object *js = VuoCurveEasing_getJson(i);
		if (shouldIncludeValue(js))
			rootMenuItem->addItem(new VuoInputEditorMenuItem(VuoCurveEasing_getSummary(i), js, renderMenuIconWithCurve(VuoCurve_Quadratic, i)));
	}

	return rootMenuItem;
}
