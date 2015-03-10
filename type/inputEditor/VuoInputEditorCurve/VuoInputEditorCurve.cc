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
	VuoInputEditorMenuItem *curveTree = new VuoInputEditorMenuItem("root");

	// Create strings for names used more than once
	const char *inAsString = "In";
	const char *outAsString = "Out";
	const char *inOutAsString = "In-out";
	const char *outInAsString = "Out-in";

	// Create first-level items
	VuoInputEditorMenuItem *linear = new VuoInputEditorMenuItem(VuoCurve_summaryFromValue(VuoCurve_Linear), VuoCurve_jsonFromValue(VuoCurve_Linear));
	VuoInputEditorMenuItem *sine = new VuoInputEditorMenuItem(VuoCurve_summaryFromValue(VuoCurve_Sine), VuoCurve_jsonFromValue(VuoCurve_Sine));

	VuoInputEditorMenuItem *window = new VuoInputEditorMenuItem("Window");
	VuoInputEditorMenuItem *quadratic = new VuoInputEditorMenuItem("Quadratic");
	VuoInputEditorMenuItem *exponential = new VuoInputEditorMenuItem("Exponential");
	VuoInputEditorMenuItem *circular = new VuoInputEditorMenuItem("Circular");
	VuoInputEditorMenuItem *spring = new VuoInputEditorMenuItem("Spring");

	// Create second-level items
	VuoInputEditorMenuItem *gaussian = new VuoInputEditorMenuItem(VuoCurve_summaryFromValue(VuoCurve_Window_Gaussian), VuoCurve_jsonFromValue(VuoCurve_Window_Gaussian));
	VuoInputEditorMenuItem *planck = new VuoInputEditorMenuItem(VuoCurve_summaryFromValue(VuoCurve_Window_Planck), VuoCurve_jsonFromValue(VuoCurve_Window_Planck));
	VuoInputEditorMenuItem *kaiser = new VuoInputEditorMenuItem(VuoCurve_summaryFromValue(VuoCurve_Window_Kaiser), VuoCurve_jsonFromValue(VuoCurve_Window_Kaiser));
	VuoInputEditorMenuItem *dolphChebyshev = new VuoInputEditorMenuItem(VuoCurve_summaryFromValue(VuoCurve_Window_DolphChebyshev), VuoCurve_jsonFromValue(VuoCurve_Window_DolphChebyshev));
	VuoInputEditorMenuItem *poisson = new VuoInputEditorMenuItem(VuoCurve_summaryFromValue(VuoCurve_Window_Poisson), VuoCurve_jsonFromValue(VuoCurve_Window_Poisson));
	VuoInputEditorMenuItem *lanczos = new VuoInputEditorMenuItem(VuoCurve_summaryFromValue(VuoCurve_Window_Lanczos), VuoCurve_jsonFromValue(VuoCurve_Window_Lanczos));
	VuoInputEditorMenuItem *parzen = new VuoInputEditorMenuItem(VuoCurve_summaryFromValue(VuoCurve_Window_Parzen), VuoCurve_jsonFromValue(VuoCurve_Window_Parzen));

	VuoInputEditorMenuItem *qIn = new VuoInputEditorMenuItem(inAsString, VuoCurve_jsonFromValue(VuoCurve_Quadratic_In));
	VuoInputEditorMenuItem *qOut = new VuoInputEditorMenuItem(outAsString, VuoCurve_jsonFromValue(VuoCurve_Quadratic_Out));
	VuoInputEditorMenuItem *qInOut = new VuoInputEditorMenuItem(inOutAsString, VuoCurve_jsonFromValue(VuoCurve_Quadratic_InOut));
	VuoInputEditorMenuItem *qOutIn = new VuoInputEditorMenuItem(outInAsString, VuoCurve_jsonFromValue(VuoCurve_Quadratic_OutIn));
	VuoInputEditorMenuItem *eIn = new VuoInputEditorMenuItem(inAsString, VuoCurve_jsonFromValue(VuoCurve_Exponential_In));
	VuoInputEditorMenuItem *eOut = new VuoInputEditorMenuItem(outAsString, VuoCurve_jsonFromValue(VuoCurve_Exponential_Out));
	VuoInputEditorMenuItem *eInOut = new VuoInputEditorMenuItem(inOutAsString, VuoCurve_jsonFromValue(VuoCurve_Exponential_InOut));
	VuoInputEditorMenuItem *eOutIn = new VuoInputEditorMenuItem(outInAsString, VuoCurve_jsonFromValue(VuoCurve_Exponential_OutIn));
	VuoInputEditorMenuItem *cIn = new VuoInputEditorMenuItem(inAsString, VuoCurve_jsonFromValue(VuoCurve_Circular_In));
	VuoInputEditorMenuItem *cOut = new VuoInputEditorMenuItem(outAsString, VuoCurve_jsonFromValue(VuoCurve_Circular_Out));
	VuoInputEditorMenuItem *cInOut = new VuoInputEditorMenuItem(inOutAsString, VuoCurve_jsonFromValue(VuoCurve_Circular_InOut));
	VuoInputEditorMenuItem *cOutIn = new VuoInputEditorMenuItem(outInAsString, VuoCurve_jsonFromValue(VuoCurve_Circular_OutIn));
	VuoInputEditorMenuItem *sIn = new VuoInputEditorMenuItem(inAsString, VuoCurve_jsonFromValue(VuoCurve_Spring_In));
	VuoInputEditorMenuItem *sOut = new VuoInputEditorMenuItem(outAsString, VuoCurve_jsonFromValue(VuoCurve_Spring_Out));
	VuoInputEditorMenuItem *sInOut = new VuoInputEditorMenuItem(inOutAsString, VuoCurve_jsonFromValue(VuoCurve_Spring_InOut));
	VuoInputEditorMenuItem *sOutIn = new VuoInputEditorMenuItem(outInAsString, VuoCurve_jsonFromValue(VuoCurve_Spring_OutIn));

	// Build tree
	window->addItem(gaussian);
	window->addItem(planck);
	window->addItem(kaiser);
	window->addItem(dolphChebyshev);
	window->addItem(poisson);
	window->addItem(lanczos);
	window->addItem(parzen);

	quadratic->addItem(qIn);
	quadratic->addItem(qOut);
	quadratic->addItem(qInOut);
	quadratic->addItem(qOutIn);

	exponential->addItem(eIn);
	exponential->addItem(eOut);
	exponential->addItem(eInOut);
	exponential->addItem(eOutIn);


	circular->addItem(cIn);
	circular->addItem(cOut);
	circular->addItem(cInOut);
	circular->addItem(cOutIn);


	spring->addItem(sIn);
	spring->addItem(sOut);
	spring->addItem(sInOut);
	spring->addItem(sOutIn);


	curveTree->addItem(linear);
	curveTree->addItem(sine);
	curveTree->addItem(window);
	curveTree->addItem(quadratic);
	curveTree->addItem(exponential);
	curveTree->addItem(circular);
	curveTree->addItem(spring);

	return curveTree;
}
