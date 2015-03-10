/**
 * @file
 * VuoRendererPublishedPort interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
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
	void setName(string name);
	bool canAccommodateInternalPort(VuoRendererPort *internalPort);
	bool isCompatibleAliasForInternalPort(VuoRendererPort *port);
	bool canBeMergedWith(VuoPublishedPort *otherExternalPort);
	void addConnectedPort(VuoPort *port);
	void removeConnectedPort(VuoPort *port);
	QPointF getGlobalPos(void) const;
	void setGlobalPos(QPointF pos);

private:
	void updateNameRect();

	QPointF globalPos;

};

#endif // VUORENDERERPUBLISHEDPORT_HH
