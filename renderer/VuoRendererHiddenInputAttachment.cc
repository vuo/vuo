/**
 * @file
 * VuoRendererHiddenInputAttachment implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoRendererHiddenInputAttachment.hh"
#include "VuoRendererPort.hh"

/**
 * Creates an input attachment with no visible rendering.
 */
VuoRendererHiddenInputAttachment::VuoRendererHiddenInputAttachment(VuoNode *baseNode, VuoRendererSignaler *signaler)
	: VuoRendererInputAttachment(baseNode, signaler)
{
	layoutPorts();
}

/**
 * Draws an input attachment with no visible rendering.
 */
void VuoRendererHiddenInputAttachment::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	if (!isSelected())
	{
		VuoNode *renderedHostNode = getRenderedHostNode();
		if (renderedHostNode && renderedHostNode->hasRenderer() && renderedHostNode->getRenderer()->isSelected())
			this->setSelected(true);

		else
		{
			set<VuoNode *> coattachments = getCoattachments();
			foreach (VuoNode *coattachment, coattachments)
			{
				if (coattachment->hasRenderer() && coattachment->getRenderer()->isSelected())
				{
					this->setSelected(true);
					break;
				}
			}
		}
	}

	return;
}

/**
 * Returns the bounding rectangle of this hidden input attachment.
 */
QRectF VuoRendererHiddenInputAttachment::boundingRect(void) const
{
	return QRectF();
}

/**
 * Returns the shape of the hidden input attachment, for use in collision detection,
 * hit tests, and QGraphicsScene::items() functions.
 */
QPainterPath VuoRendererHiddenInputAttachment::shape() const
{
	return QPainterPath();
}

/**
  * Calculates and sets the positions of the hidden input attachment's child ports relative to the node.
  */
void VuoRendererHiddenInputAttachment::layoutPorts(void)
{
	// Do not display input ports.
	QList<VuoRendererPort *> inputPorts = this->inputPorts->childItems();
	for (unsigned int i = 0; i < inputPorts.size(); ++i)
		inputPorts[i]->setVisible(false);

	// Do not display output ports.
	QList<VuoRendererPort *> outputPorts = this->outputPorts->childItems();
	for (unsigned int i = 0; i < outputPorts.size(); ++i)
		outputPorts[i]->setVisible(false);
}

/**
 * Returns a boolean indicating whether painting is currently disabled for this node.
 */
bool VuoRendererHiddenInputAttachment::paintingDisabled() const
{
	return true;
}
