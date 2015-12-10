/**
 * @file
 * VuoInputEditor implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditor.hh"

/**
 * Returns the font that input editors are recommended to use.
 */
QFont VuoInputEditor::getDefaultFont(void)
{
//	return VuoRendererFonts::getSharedFonts()->nodePortTitleFont();  // This would be better, but it makes the input editors depend on VuoRendererFonts.
	return QFont("Signika", 20.0*8.0/16.0, QFont::Normal, false);
}

bool VuoInputEditor::supportsTabbingBetweenPorts(void)
{
	return false;
}
