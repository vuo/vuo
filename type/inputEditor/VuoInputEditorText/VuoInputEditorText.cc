/**
 * @file
 * VuoInputEditorText implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoInputEditorText.hh"

extern "C"
{
	#include "VuoText.h"
}

/**
 * Constructs a VuoInputEditorText object.
 */
VuoInputEditor * VuoInputEditorTextFactory::newInputEditor()
{
	return new VuoInputEditorText();
}
