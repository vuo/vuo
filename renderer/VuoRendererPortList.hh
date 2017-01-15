/**
 * @file
 * VuoRendererPortList interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUORENDERERPORTLIST_HH
#define VUORENDERERPORTLIST_HH

class VuoRendererNode;
class VuoRendererPort;

/**
 * A @c QGraphicsItemGroup containing only instances of @c VuoRendererPort.
 */
class VuoRendererPortList : public QGraphicsItemGroup
{
public:
	VuoRendererPortList(VuoRendererNode *parent);

	QRectF boundingRect(void) const;

	QList<VuoRendererPort *> childItems() const;
};

#endif // VUORENDERERPORTLIST_HH
