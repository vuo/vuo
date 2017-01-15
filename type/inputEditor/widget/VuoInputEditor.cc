/**
 * @file
 * VuoInputEditor implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditor.hh"

/**
 * Returns the font that input editors are recommended to use.
 */
QFont VuoInputEditor::getDefaultFont(void)
{
	return QFont("Signika", 12, QFont::Light, false);
}

/**
 * Returns a CSS representation of the font that input editors are recommended to use.
 */
QString VuoInputEditor::getDefaultFontCss(void)
{
	return QString("font-family: Signika; font-size: 12pt; font-weight: lighter;");
}

bool VuoInputEditor::supportsTabbingBetweenPorts(void)
{
	return false;
}
