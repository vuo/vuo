/**
 * @file
 * VuoRendererKeyListForReadOnlyDictionary interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUORENDERERKEYLISTFORREADONLYDICTIONARY_HH
#define VUORENDERERKEYLISTFORREADONLYDICTIONARY_HH

#include "VuoRendererHiddenInputAttachment.hh"

/**
 * Represents the compact form of a "Make List" node that outputs a list of keys
 * as input to a read-only input "Make Dictionary" node.
 */
class VuoRendererKeyListForReadOnlyDictionary : public VuoRendererHiddenInputAttachment
{
public:
	VuoRendererKeyListForReadOnlyDictionary(VuoNode *baseNode, VuoRendererSignaler *signaler);
	VuoPort * getRenderedHostPort();
	VuoNode * getRenderedHostNode();
	set<VuoNode *> getCoattachments(void);
	VuoNode * getValueListNode(void);

	static bool isKeyListForReadOnlyDictionary(VuoNode *baseNode);
};

#endif // VUORENDERERKEYLISTFORREADONLYDICTIONARY_HH
