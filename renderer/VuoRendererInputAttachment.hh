/**
 * @file
 * VuoRendererInputAttachment interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUORENDERERINPUTATTACHMENT_HH
#define VUORENDERERINPUTATTACHMENT_HH

#include "VuoRendererNode.hh"
#include "VuoNode.hh"
#include "VuoPort.hh"

/**
 * Represents a node that is rendered as an attachment to another node's input port.
 */
class VuoRendererInputAttachment : public VuoRendererNode
{
public:
	VuoRendererInputAttachment(VuoNode *baseNode, VuoRendererSignaler *signaler);
	VuoPort * getUnderlyingHostPort();
	VuoNode * getUnderlyingHostNode();
	virtual VuoPort * getRenderedHostPort();
	virtual VuoNode * getRenderedHostNode();
	virtual set<VuoNode *> getCoattachments(void);

protected:
	static VuoPort * getUnderlyingHostPortForNode(VuoNode *node);
	static VuoNode * getUnderlyingHostNodeForNode(VuoNode *node);
	static VuoNode * getListNodeConnectedToInputPort(VuoPort *port);
};

#endif // VUORENDERERINPUTATTACHMENT_HH
