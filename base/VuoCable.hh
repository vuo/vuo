/**
 * @file
 * VuoCable interface.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoBase.hh"

class VuoCompilerCable;
class VuoRendererCable;
class VuoNode;
class VuoPort;

/**
 * Represents a connection from a node's output port to a node's input port.
 */
class VuoCable : public VuoBase<VuoCompilerCable,VuoRendererCable>
{
public:
	VuoCable(VuoNode * fromNode, VuoPort * fromPort, VuoNode * toNode, VuoPort * toPort, bool addCableToPorts = true);

	VuoNode * getFromNode(void);
	VuoPort * getFromPort(void);
	VuoNode * getToNode(void);
	VuoPort * getToPort(void);
	void setFrom(VuoNode *fromNode, VuoPort *fromPort);
	void setTo(VuoNode *toNode, VuoPort *toPort);
	bool isPublished(void);
	bool isPublishedInputCable(void);
	bool isPublishedOutputCable(void);

private:
	VuoNode * fromNode;
	VuoPort * fromPort;
	VuoNode * toNode;
	VuoPort * toPort;
};
