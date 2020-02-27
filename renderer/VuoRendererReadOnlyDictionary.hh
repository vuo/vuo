/**
 * @file
 * VuoRendererReadOnlyDictionary interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoRendererHiddenInputAttachment.hh"

class VuoNode;
class VuoRendererSignaler;

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

