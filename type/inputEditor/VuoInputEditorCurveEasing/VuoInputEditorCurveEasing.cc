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

	rootMenuItem->addItem(new VuoInputEditorMenuItem(VuoCurveEasing_getSummary(VuoCurveEasing_In),		VuoCurveEasing_getJson(VuoCurveEasing_In),		renderMenuIconWithCurve(VuoCurve_Quadratic, VuoCurveEasing_In)));
	rootMenuItem->addItem(new VuoInputEditorMenuItem(VuoCurveEasing_getSummary(VuoCurveEasing_Out),		VuoCurveEasing_getJson(VuoCurveEasing_Out),		renderMenuIconWithCurve(VuoCurve_Quadratic, VuoCurveEasing_Out)));
	rootMenuItem->addItem(new VuoInputEditorMenuItem(VuoCurveEasing_getSummary(VuoCurveEasing_InOut),		VuoCurveEasing_getJson(VuoCurveEasing_InOut),		renderMenuIconWithCurve(VuoCurve_Quadratic, VuoCurveEasing_InOut)));
	rootMenuItem->addItem(new VuoInputEditorMenuItem(VuoCurveEasing_getSummary(VuoCurveEasing_Middle),	VuoCurveEasing_getJson(VuoCurveEasing_Middle),	renderMenuIconWithCurve(VuoCurve_Quadratic, VuoCurveEasing_Middle)));

	return rootMenuItem;
}
