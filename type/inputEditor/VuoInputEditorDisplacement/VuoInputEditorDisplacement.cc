/**
 * @file
 * VuoInputEditorDisplacement implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorDisplacement.hh"

extern "C"
{
	#include "VuoDisplacement.h"
}

#include "../VuoInputEditorCurve/VuoInputEditorCurveRenderer.hh"

/**
 * Constructs a VuoInputEditorDisplacement object.
 */
VuoInputEditor * VuoInputEditorDisplacementFactory::newInputEditor()
{
	return new VuoInputEditorDisplacement();
}

/**
 * Renders an icon representing the specified displacement type.
 */
static QIcon *renderMenuIconWithDisplacement(VuoDisplacement displacement)
{
	return VuoInputEditorIcon::renderIcon(^(QPainter &p){
											  if (displacement == VuoDisplacement_Transverse)
											  {
												  QPainterPath path;
												  for (VuoReal x = 0; x <= 15; ++x)
												  {
													  VuoReal y = 8-3.5*sin(2.*M_PI*x/7.4);
													  if (x == 0)
														  path.moveTo(0.5+x, y);
													  else
														  path.lineTo(0.5+x, y);
												  }
												  p.strokePath(path, QPen(Qt::black, 1.));
											  }
											  else if (displacement == VuoDisplacement_Longitudinal)
												  for (VuoReal x = 0; x <= 15; ++x)
												  {
													  p.setPen(QPen(QColor(0,0,0,sin(2.*M_PI*x/7.4)*112+128), 1.));
													  p.drawLine(QPointF(0.5+x, 7.5-3), QPointF(0.5+x, 7.5+4));
												  }
										  });
}

/**
 * Creates the tree that models the menu.
 */
VuoInputEditorMenuItem * VuoInputEditorDisplacement::setUpMenuTree()
{
	VuoInputEditorMenuItem *optionsTree = new VuoInputEditorMenuItem("root");

	for (int i = 0; i <= VuoDisplacement_Longitudinal; ++i)
	{
		json_object *optionAsJson = VuoDisplacement_getJson( (VuoDisplacement)i );
		char *optionSummary = VuoDisplacement_getSummary( (VuoDisplacement)i );
		VuoInputEditorMenuItem *optionItem = new VuoInputEditorMenuItem(optionSummary, optionAsJson, renderMenuIconWithDisplacement((VuoDisplacement)i));
		free(optionSummary);
		optionsTree->addItem(optionItem);
	}

	return optionsTree;
}
