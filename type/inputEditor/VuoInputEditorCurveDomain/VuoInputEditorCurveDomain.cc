/**
 * @file
 * VuoInputEditorCurveDomain implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorCurveDomain.hh"

extern "C"
{
	#include "VuoCurveDomain.h"
}

/**
 * Constructs a VuoInputEditorCurveDomain object.
 */
VuoInputEditor * VuoInputEditorCurveDomainFactory::newInputEditor()
{
	return new VuoInputEditorCurveDomain();
}

/**
 * Creates the tree that models the menu.
 */
VuoInputEditorMenuItem * VuoInputEditorCurveDomain::setUpMenuTree()
{
	VuoInputEditorMenuItem *curveDomainOptionsTree = new VuoInputEditorMenuItem("root");

	json_object *optionAsJson_Clamp = VuoCurveDomain_jsonFromValue(VuoCurveDomain_Clamp);
	const char *optionSummary_Clamp = VuoCurveDomain_summaryFromValue(VuoCurveDomain_Clamp);

	json_object *optionAsJson_Infinite = VuoCurveDomain_jsonFromValue(VuoCurveDomain_Infinite);
	const char *optionSummary_Infinite = VuoCurveDomain_summaryFromValue(VuoCurveDomain_Infinite);

	json_object *optionAsJson_Wrap = VuoCurveDomain_jsonFromValue(VuoCurveDomain_Wrap);
	const char *optionSummary_Wrap = VuoCurveDomain_summaryFromValue(VuoCurveDomain_Wrap);

	json_object *optionAsJson_Mirror = VuoCurveDomain_jsonFromValue(VuoCurveDomain_Mirror);
	const char *optionSummary_Mirror = VuoCurveDomain_summaryFromValue(VuoCurveDomain_Mirror);


	VuoInputEditorMenuItem *optionItem_Clamp = new VuoInputEditorMenuItem(optionSummary_Clamp, optionAsJson_Clamp);
	VuoInputEditorMenuItem *optionItem_Infinite = new VuoInputEditorMenuItem(optionSummary_Infinite, optionAsJson_Infinite);
	VuoInputEditorMenuItem *optionItem_Wrap = new VuoInputEditorMenuItem(optionSummary_Wrap, optionAsJson_Wrap);
	VuoInputEditorMenuItem *optionItem_Mirror = new VuoInputEditorMenuItem(optionSummary_Mirror, optionAsJson_Mirror);

	curveDomainOptionsTree->addItem(optionItem_Clamp);
	curveDomainOptionsTree->addItem(optionItem_Infinite);
	curveDomainOptionsTree->addItem(optionItem_Wrap);
	curveDomainOptionsTree->addItem(optionItem_Mirror);

	return curveDomainOptionsTree;
}
