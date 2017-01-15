/**
 * @file
 * VuoRendererValueListForReadOnlyDictionary interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUORENDERERVALUELISTFORREADONLYDICTIONARY_HH
#define VUORENDERERVALUELISTFORREADONLYDICTIONARY_HH

#include "VuoRendererInputDrawer.hh"
#include "VuoRendererKeyListForReadOnlyDictionary.hh"

/**
 * Represents the compact form of a "Make List" node that outputs a list of values
 * as input to a read-only input "Make Dictionary" node.
 */
class VuoRendererValueListForReadOnlyDictionary : public VuoRendererInputDrawer
{
public:
	VuoRendererValueListForReadOnlyDictionary(VuoNode *baseNode, VuoRendererSignaler *signaler);
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	QRectF boundingRect(void) const;
	QRectF getOuterNodeFrameBoundingRect(void) const;
	QPainterPath shape() const;
	VuoPort * getRenderedHostPort();
	VuoNode * getRenderedHostNode();
	set<VuoNode *> getCoattachments(void);
	VuoNode * getKeyListNode(void);

	static bool isValueListForReadOnlyDictionary(VuoNode *baseNode);

private:
	QPainterPath getDrawerPath() const;
	string getRenderedPortNameFormForText(string text);
};

#endif // VUORENDERERVALUELISTFORREADONLYDICTIONARY_HH
