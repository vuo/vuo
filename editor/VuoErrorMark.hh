/**
 * @file
 * VuoErrorMark interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoRendererItem.hh"

class VuoRendererCable;
class VuoRendererNode;

/**
 * A red outline around nodes and cables involved in a composition build error.
 */
class VuoErrorMark : public VuoRendererItem
{
public:
	VuoErrorMark();
	void addMarkedComponents(set<VuoRendererNode *> nodes, set<VuoRendererCable *> cables);
	QRectF boundingRect(void) const;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	void removeFromScene();
	void updateErrorMarkPath();

private:
	QPainterPath getErrorMarkPath() const;
	QPainterPath errorMarkPath;
	set<VuoRendererNode *> nodes;
	set<VuoRendererCable *> cables;
};
