/**
 * @file
 * VuoInputEditorImageWrapMode implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorImageWrapMode.hh"

/**
 * Constructs a VuoInputEditorImageWrapMode object.
 */
VuoInputEditor * VuoInputEditorImageWrapModeFactory::newInputEditor()
{
	return new VuoInputEditorImageWrapMode();
}
