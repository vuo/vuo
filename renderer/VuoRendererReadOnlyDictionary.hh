/**
 * @file
 * VuoRendererReadOnlyDictionary interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUORENDERERREADONLYDICTIONARY_HH
#define VUORENDERERREADONLYDICTIONARY_HH

#include "VuoRendererHiddenInputAttachment.hh"
#include "VuoRendererPort.hh"
#include "VuoNode.hh"
#include "VuoPort.hh"
#include "VuoCable.hh"

/**
 * Represents the compact form of a read-only input "Make Dictionary" node.
 */
class VuoRendererReadOnlyDictionary : public VuoRendererHiddenInputAttachment
{
public:
	VuoRendererReadOnlyDictionary(VuoNode *baseNode, VuoRendererSignaler *signaler);
	set<VuoNode *> getCoattachments(void);

	static bool isReadOnlyDictionary(VuoNode *baseNode);

private:
	VuoNode * getKeyListNode();
	VuoNode * getValueListNode();
};

#endif // VUORENDERERREADONLYDICTIONARY_HH
