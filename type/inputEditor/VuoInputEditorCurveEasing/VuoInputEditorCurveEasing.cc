/**
 * @file
 * VuoInputEditorCurveEasing implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoInputEditorCurveEasing.hh"

extern "C"
{
}

#include "VuoInputEditorCurveRenderer.hh"

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

	for (int c = VuoCurveEasing_In; c <= VuoCurveEasing_Middle; ++c)
	{
		VuoCurveEasing i = (VuoCurveEasing)c;
		json_object *js = VuoCurveEasing_getJson(i);
		if (shouldIncludeValue(js))
			rootMenuItem->addItem(new VuoInputEditorMenuItem(VuoCurveEasing_getSummary(i), js, renderMenuIconWithCurve(VuoCurve_Quadratic, i, isInterfaceDark())));
	}

	return rootMenuItem;
}
