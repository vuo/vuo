/**
 * @file
 * VuoCable interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCABLE_HH
#define VUOCABLE_HH

#include "VuoBase.hh"
#include "VuoNode.hh"

class VuoCompilerCable;
class VuoRendererCable;

/**
 * Represents a connection from a node's output port to a node's input port.
 */
class VuoCable : public VuoBase<VuoCompilerCable,VuoRendererCable>
{
public:
	VuoCable(VuoNode * fromNode, VuoPort * fromPort, VuoNode * toNode, VuoPort * toPort);

	VuoNode * getFromNode(void);
	VuoPort * getFromPort(void);
	VuoNode * getToNode(void);
	VuoPort * getToPort(void);
	void setFrom(VuoNode *fromNode, VuoPort *fromPort);
	void setTo(VuoNode *toNode, VuoPort *toPort);
	bool isPublishedCable(void);
	bool isPublishedInputCable(void);
	bool isPublishedOutputCable(void);

private:
	VuoNode * fromNode;
	VuoPort * fromPort;
	VuoNode * toNode;
	VuoPort * toPort;
};

#endif // VUOCABLE_HH
