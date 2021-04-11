/**
 * @file
 * VuoRendererPublishedPort interface.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoRendererPort.hh"
#include "VuoPublishedPort.hh"
#include "VuoRendererCable.hh"

class VuoType;

/**
 * A published input or output port.
 */
class VuoRendererPublishedPort : public VuoRendererPort
{
public:
	VuoRendererPublishedPort(VuoPublishedPort *basePublishedPort, bool isPublishedOutput);

	void setName(string name);
	bool canAccommodateInternalPort(VuoRendererPort *internalPort, bool eventOnlyConnection);
	bool isCompatibleAliasWithoutSpecializationForInternalPort(VuoRendererPort *port, bool eventOnlyConnection);
	bool isCompatibleAliasWithSpecializationForInternalPort(VuoRendererPort *port, bool eventOnlyConnection);
	bool isCompatibleAliasWithSpecializationForInternalPort(VuoRendererPort *internalPort, bool eventOnlyConnection, VuoRendererPort **portToSpecialize, string &specializedTypeName);
	bool canBeMergedWith(VuoPublishedPort *otherExternalPort, bool mergeWillAddData);
	QPoint getCompositionViewportPos(void) const;
	void setCompositionViewportPos(QPoint pos);
	void setCurrentlyActive(bool active);
	bool getCurrentlyActive();
	void setPermanent(bool permanent);
	bool isPermanent(void);

protected:
	QPainterPath getWirelessAntennaPath() const;

private:
	QPoint compositionViewportPos;
	QPoint globalPos;
	bool isActive;
	bool permanent;
};
