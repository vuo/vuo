/**
 * @file
 * VuoInputEditor implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoInputEditor.hh"

/**
 * Returns the font that input editors are recommended to use.
 */
QFont VuoInputEditor::getDefaultFont(void)
{
	// Can't use VuoRendererFonts since the dependencies are complicated.
	return QFont("PT Sans", 12, QFont::Light, false);
}

bool VuoInputEditor::supportsTabbingBetweenPorts(void)
{
	return false;
}
