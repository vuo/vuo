/**
 * @file
 * VuoRendererInputDrawer interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUORENDERERINPUTDRAWER_HH
#define VUORENDERERINPUTDRAWER_HH

#include "VuoNode.hh"
#include "VuoRendererInputAttachment.hh"
#include "VuoRendererSignaler.hh"

/**
 * Represents the compact drawer form of a "Make List" node.
 */
class VuoRendererInputDrawer : public VuoRendererInputAttachment
{
public:
	VuoRendererInputDrawer(VuoNode *baseNode, VuoRendererSignaler *signaler);

	qreal getMaxDrawerLabelWidth(void) const;
	qreal getMaxDrawerChainedLabelWidth(void) const;
	vector<VuoRendererPort *> getDrawerPorts(void) const;
	void setHorizontalDrawerOffset(qreal offset);

	static const qreal drawerInteriorHorizontalPadding; ///< The amount of horizontal padding, in pixels, added to each drawer beyond what its text strictly requires.
	static const qreal drawerHorizontalSpacing; ///< The amount of space, in pixels, left as horizontal padding between underhangs of drawers attached to ports of the same node.

protected:
	void layoutPorts(void);

	vector<VuoRendererPort *> drawerPorts; ///< The vector of input ports whose values will be incorporated into the output list.
	qreal horizontalDrawerOffset; ///< The distance, in pixels, left of its attached port that the rightmost point of this drawer should be rendered.
	qreal drawerBottomExtensionHeight; ///< The height, in pixels, of the input drawer (excluding the arm and drag handle).
};

#endif // VUORENDERERINPUTDRAWER_HH
