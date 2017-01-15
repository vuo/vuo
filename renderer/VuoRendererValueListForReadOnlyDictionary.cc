/**
 * @file
 * VuoRendererValueListForReadOnlyDictionary implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoRendererValueListForReadOnlyDictionary.hh"
#include "VuoRendererReadOnlyDictionary.hh"
#include "VuoRendererCable.hh"
#include "VuoRendererInputListDrawer.hh"
#include "VuoRendererTypecastPort.hh"
#include "VuoRendererColors.hh"
#include "VuoRendererFonts.hh"
#include "VuoCompilerMakeListNodeClass.hh"
#include "VuoCompilerInputEventPort.hh"
#include "VuoStringUtilities.hh"

/**
 * Creates the compact form for a collapsed "Make List" node that outputs a list of values
 * as input to a read-only input "Make Dictionary" node.
 */
VuoRendererValueListForReadOnlyDictionary::VuoRendererValueListForReadOnlyDictionary(VuoNode *baseNode, VuoRendererSignaler *signaler)
	: VuoRendererInputDrawer(baseNode, signaler)
{
	VuoNode *makeKeyListNode = getKeyListNode();
	vector<VuoPort *> makeKeyListNodeInputs;
	if (makeKeyListNode)
		makeKeyListNodeInputs = makeKeyListNode->getInputPorts();

	vector<VuoPort *> inputPorts = this->getBase()->getInputPorts();
	for (int i = VuoNodeClass::unreservedInputPortStartIndex; i < inputPorts.size(); ++i)
	{
		string keyName = " ";
		if (i < makeKeyListNodeInputs.size())
		{
			VuoCompilerInputEventPort *keyPort= static_cast<VuoCompilerInputEventPort *>(makeKeyListNodeInputs[i]->getCompiler());
			keyName = keyPort->getData()->getInitialValue();
		}

		inputPorts[i]->getRenderer()->setPortNameToRender(getRenderedPortNameFormForText(keyName));
	}
}

/**
 * Returns the string representation of the provided VuoText value as it should be
 * rendered as a port title within the body of this list attachment.
 * Modelled after VuoRendererPort::getConstantAsStringToRender().
 */
string VuoRendererValueListForReadOnlyDictionary::getRenderedPortNameFormForText(string text)
{
	json_object *js = json_tokener_parse(text.c_str());
	string textWithoutQuotes;
	if (json_object_get_type(js) == json_type_string)
		textWithoutQuotes = json_object_get_string(js);
	json_object_put(js);

	return textWithoutQuotes;
}

/**
 * Draws a collapsed "Make List" node that outputs a list of values as input to
 * a read-only input "Make Dictionary" node.
 */
void VuoRendererValueListForReadOnlyDictionary::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
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

	drawBoundingRect(painter);

	VuoRendererColors::SelectionType selectionType = (isSelected()? VuoRendererColors::directSelection : VuoRendererColors::noSelection);
	qint64 timeOfLastActivity = (getRenderActivity()? timeLastExecutionEnded : VuoRendererItem::notTrackingActivity);
	VuoRendererColors *drawerColors = new VuoRendererColors(getBase()->getTintColor(),
													  selectionType,
													  false,
													  VuoRendererColors::noHighlight,
													  timeOfLastActivity);


	// Drawer frame
	painter->setPen(QPen(drawerColors->nodeFrame(),1,Qt::SolidLine,Qt::SquareCap,Qt::MiterJoin));
	painter->setBrush(drawerColors->nodeFill());
	QPainterPath drawerPath = getDrawerPath();
	painter->drawPath(drawerPath);

	// Child input ports
	layoutPorts();

	delete drawerColors;
}

/**
 * Returns the bounding rectangle of this collapsed "Make List" node.
 */
QRectF VuoRendererValueListForReadOnlyDictionary::boundingRect(void) const
{
	QRectF r = getDrawerPath().boundingRect();

	// Antialiasing bleed
	r.adjust(-1,-1,1,1);

	return r.toAlignedRect();
}

/**
 * Returns the shape of the rendered node, for use in collision detection,
 * hit tests, and QGraphicsScene::items() functions.
 */
QPainterPath VuoRendererValueListForReadOnlyDictionary::shape() const
{
	QPainterPath p;
	p.addPath(getDrawerPath());

	return p;
}

/**
 * Returns a boolean indicating whether the provided @c baseNode should be rendered
 * as a collapsed "Make List" node that outputs a list of values as input to a read-only
 * input "Make Dictionary" node, as determined from its connected components.
 */
bool VuoRendererValueListForReadOnlyDictionary::isValueListForReadOnlyDictionary(VuoNode *baseNode)
{
	if (!VuoCompilerMakeListNodeClass::isMakeListNodeClassName(baseNode->getNodeClass()->getClassName()))
		return false;

	VuoPort *hostPort = getUnderlyingHostPortForNode(baseNode);
	VuoNode *hostNode = getUnderlyingHostNodeForNode(baseNode);
	bool providesValuesToMakeReadOnlyDictionaryNode = (hostPort &&
												(hostPort->getClass()->getName() == "values") &&
												(VuoRendererReadOnlyDictionary::isReadOnlyDictionary(hostNode)));
	return providesValuesToMakeReadOnlyDictionaryNode;
}

/**
  * Returns the "Make List" node that outputs the list of keys corresponding to
  * this node's output list of values.
  */
VuoNode * VuoRendererValueListForReadOnlyDictionary::getKeyListNode()
{
	VuoNode *hostNode = getUnderlyingHostNode();
	if (!hostNode)
		return NULL;

	VuoPort *keyListInputPort = hostNode->getInputPortWithName("keys");
	return getListNodeConnectedToInputPort(keyListInputPort);
}

/**
 * Returns a closed path representing the drawer of the collapsed "Make List" node.
 */
QPainterPath VuoRendererValueListForReadOnlyDictionary::getDrawerPath() const
{
	// Create a hybrid rect having the width of the port's inset rect and the customized
	// height of a constant flag, so that the "Make List" arm has the desired height but
	// directly adjoins the inset port shape.
	QRectF hostPortRect = VuoRendererPort::getPortRect();
	vector<VuoRendererPort *> drawerPorts = getDrawerPorts();
	QRectF hostPortHybridRect = QRectF(hostPortRect.x(), -0.5*VuoRendererPort::constantFlagHeight, hostPortRect.width(), VuoRendererPort::constantFlagHeight);

	qreal hostPortContactPointX = horizontalDrawerOffset -0.5*hostPortRect.width() + VuoRendererPort::portInset;
	qreal drawerBottomExtensionWidth = getMaxDrawerLabelWidth();

	QPainterPath p;

	// Drawer top left
	p.moveTo(0, -0.5*hostPortHybridRect.height());
	// Arm top edge
	p.lineTo(hostPortContactPointX-0.5*hostPortHybridRect.height(), -0.5*hostPortHybridRect.height());
	// Arm right triangular point
	p.lineTo(hostPortContactPointX, 0);
	p.lineTo(hostPortContactPointX-0.5*hostPortHybridRect.height(), 0.5*hostPortHybridRect.height());
	// Arm bottom edge
	p.lineTo(drawerBottomExtensionWidth, 0.5*hostPortHybridRect.height());
	// Right drawer wall
	p.lineTo(drawerBottomExtensionWidth, 0.5*hostPortHybridRect.height() + drawerBottomExtensionHeight);
	// Far drawer bottom
	p.lineTo(0, 0.5*hostPortHybridRect.height() + drawerBottomExtensionHeight);
	p.closeSubpath();

	// Carve the input child ports out of the frame.
	QPainterPath drawerPortsPath;

	for (int i=0; i < drawerPorts.size(); ++i)
	{
		VuoRendererPort *p = drawerPorts[i];
		VuoRendererTypecastPort *tp = dynamic_cast<VuoRendererTypecastPort *>(p);
		QPainterPath insetPath;

		if (tp)
			insetPath = tp->getPortPath(true, false, NULL);
		else
			insetPath = p->getPortPath(VuoRendererPort::portInset);

		drawerPortsPath.addPath(insetPath.translated(p->pos()));
	}

	p = p.subtracted(drawerPortsPath);

	return p;
}

/**
  * Returns the input port to which this item is visually attached in the rendered composition.
  */
VuoPort * VuoRendererValueListForReadOnlyDictionary::getRenderedHostPort()
{
	// The port that will ultimately make use of this node's output values is two nodes downstream.
	return getUnderlyingHostPortForNode(getUnderlyingHostNodeForNode(this->getBase()));
}

/**
  * Returns the node to which this item is visually attached in the composition rendering.
  */
VuoNode * VuoRendererValueListForReadOnlyDictionary::getRenderedHostNode()
{
	// The node that will ultimately make use of this node's output values is two nodes downstream.
	return getUnderlyingHostNodeForNode(getUnderlyingHostNodeForNode(this->getBase()));
}

/**
 * Returns the bounding rect for the drawer.
 */
QRectF VuoRendererValueListForReadOnlyDictionary::getOuterNodeFrameBoundingRect(void) const
{
	return getDrawerPath().boundingRect();
}

/**
 * Returns the set of co-attachments expected to co-exist with this attachment.
 * For a value list, that includes its sibling key list and the "Make Dictionary"
 * node that they each provide to.
 */
set<VuoNode *> VuoRendererValueListForReadOnlyDictionary::getCoattachments(void)
{
	VuoNode *dictionaryNode = getUnderlyingHostNode();
	VuoNode *keyListNode = getKeyListNode();

	set<VuoNode *> coattachments;
	if (dictionaryNode)
		coattachments.insert(dictionaryNode);
	if (keyListNode)
		coattachments.insert(keyListNode);
	return coattachments;
}
