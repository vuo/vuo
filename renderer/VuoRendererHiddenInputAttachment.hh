/**
 * @file
 * VuoRendererHiddenInputAttachment interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUORENDERERHIDDENINPUTATTACHMENT_HH
#define VUORENDERERHIDDENINPUTATTACHMENT_HH

#include "VuoRendererInputAttachment.hh"

/**
 * Represents an input attachment with no visible rendering.
 */
class VuoRendererHiddenInputAttachment : public VuoRendererInputAttachment
{
public:
	VuoRendererHiddenInputAttachment(VuoNode *baseNode, VuoRendererSignaler *signaler);

	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	QRectF boundingRect(void) const;
	QPainterPath shape() const;

private:
	void layoutPorts(void);
	bool paintingDisabled(void) const;
};

#endif // VUORENDERERHIDDENINPUTATTACHMENT_HH
