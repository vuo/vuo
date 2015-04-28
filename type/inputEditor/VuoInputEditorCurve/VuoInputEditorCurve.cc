/**
 * @file
 * VuoInputEditorCurve implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
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

	rootMenuItem->addItem(new VuoInputEditorMenuItem(VuoCurve_summaryFromValue(VuoCurve_Linear),		VuoCurve_jsonFromValue(VuoCurve_Linear),		renderMenuIconWithCurve(VuoCurve_Linear)));
	rootMenuItem->addItem(new VuoInputEditorMenuItem(VuoCurve_summaryFromValue(VuoCurve_Quadratic),		VuoCurve_jsonFromValue(VuoCurve_Quadratic),		renderMenuIconWithCurve(VuoCurve_Quadratic)));
	rootMenuItem->addItem(new VuoInputEditorMenuItem(VuoCurve_summaryFromValue(VuoCurve_Cubic),			VuoCurve_jsonFromValue(VuoCurve_Cubic),			renderMenuIconWithCurve(VuoCurve_Cubic)));
	rootMenuItem->addItem(new VuoInputEditorMenuItem(VuoCurve_summaryFromValue(VuoCurve_Circular),		VuoCurve_jsonFromValue(VuoCurve_Circular),		renderMenuIconWithCurve(VuoCurve_Circular)));
	rootMenuItem->addItem(new VuoInputEditorMenuItem(VuoCurve_summaryFromValue(VuoCurve_Exponential),	VuoCurve_jsonFromValue(VuoCurve_Exponential),	renderMenuIconWithCurve(VuoCurve_Exponential)));

	return rootMenuItem;
}
