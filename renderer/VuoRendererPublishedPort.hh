/**
 * @file
 * VuoRendererPublishedPort interface.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUORENDERERPUBLISHEDPORT_HH
#define VUORENDERERPUBLISHEDPORT_HH

#include "VuoBaseDetail.hh"
#include "VuoNode.hh"
#include "VuoPort.hh"
#include "VuoPublishedPort.hh"
#include "VuoRendererItem.hh"

/**
 * A published input or output port.
 */
class VuoRendererPublishedPort : public VuoRendererItem, public VuoBaseDetail<VuoPublishedPort>
{
public:
	VuoRendererPublishedPort(VuoPublishedPort *basePublishedPort);

	QRectF boundingRect(void) const;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	bool canAccommodateInternalPort(VuoRendererPort *internalPort);
	bool isCompatibleAliasForInternalPort(VuoRendererPort *port);
	bool canBeMergedWith(VuoPublishedPort *otherExternalPort);
	QPointF getGlobalPos(void) const;
	void setGlobalPos(QPointF pos);

private:
	QPointF globalPos;

};

#endif // VUORENDERERPUBLISHEDPORT_HH
