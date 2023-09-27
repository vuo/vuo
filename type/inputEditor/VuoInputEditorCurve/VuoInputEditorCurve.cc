/**
 * @file
 * VuoInputEditorCurve implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoInputEditorCurve.hh"

extern "C"
{
	#include "VuoCurve.h"
}

#include "VuoInputEditorCurveRenderer.hh"

/**
 * Constructs a VuoInputEditorCurve object.
 */
VuoInputEditor * VuoInputEditorCurveFactory::newInputEditor()
{
	return new VuoInputEditorCurve();
}

/**
 * Creates the tree that models the menu.
 */
VuoInputEditorMenuItem * VuoInputEditorCurve::setUpMenuTree()
{
	VuoInputEditorMenuItem *rootMenuItem = new VuoInputEditorMenuItem("root");

	VuoCurve curves[] = {
		VuoCurve_Linear,
		VuoCurve_Quadratic,
		VuoCurve_Cubic,
		VuoCurve_Circular,
		VuoCurve_Exponential,
	};
	for (int i = 0; i < sizeof(curves)/sizeof(VuoCurve); ++i)
		rootMenuItem->addItem(new VuoInputEditorMenuItem(VuoCurve_getSummary(curves[i]),
														 VuoCurve_getJson(curves[i]),
														 renderMenuIconWithCurve(curves[i], VuoCurveEasing_In, isInterfaceDark())));

	return rootMenuItem;
}
