/**
 * @file
 * VuoInputEditorDispersion implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorDispersion.hh"

extern "C"
{
	#include "VuoDispersion.h"
}

#include "../VuoInputEditorCurve/VuoInputEditorCurveRenderer.hh"

/**
 * Constructs a VuoInputEditorDispersion object.
 */
VuoInputEditor * VuoInputEditorDispersionFactory::newInputEditor()
{
	return new VuoInputEditorDispersion();
}

/**
 * Renders an icon representing the specified dispersion type.
 */
static QIcon *renderMenuIconWithDispersion(VuoDispersion dispersion)
{
	return VuoInputEditorIcon::renderIcon(^(QPainter &p){
											  p.setPen(QPen(Qt::black, 1.));
											  if (dispersion == VuoDispersion_Linear)
												  p.drawLine(QPointF(0.5, 7.5), QPointF(14.5, 7.5));
											  else if (dispersion == VuoDispersion_Radial)
												  for (int i = 1; i <= 7; i += 3)
													  p.drawEllipse(QPointF(7.5, 7.5), i, i);
										  });
}

/**
 * Creates the tree that models the menu.
 */
VuoInputEditorMenuItem * VuoInputEditorDispersion::setUpMenuTree()
{
	VuoInputEditorMenuItem *optionsTree = new VuoInputEditorMenuItem("root");

	for (int i = 0; i <= VuoDispersion_Radial; ++i)
	{
		json_object *optionAsJson = VuoDispersion_getJson( (VuoDispersion)i );
		char *optionSummary = VuoDispersion_getSummary( (VuoDispersion)i );
		VuoInputEditorMenuItem *optionItem = new VuoInputEditorMenuItem(optionSummary, optionAsJson, renderMenuIconWithDispersion((VuoDispersion)i));
		free(optionSummary);

		optionsTree->addItem(optionItem);
	}

	return optionsTree;
}
