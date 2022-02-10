/**
 * @file
 * VuoRendererFonts implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoRendererFonts.hh"

VuoRendererFonts *VuoRendererFonts::sharedFonts = NULL;

const qreal VuoRendererFonts::thickPenWidth = 20;
const qreal VuoRendererFonts::midPenWidth = VuoRendererFonts::thickPenWidth/10.0;

const QString VuoRendererFonts::fontFamily = "PT Sans";

const qreal VuoRendererFonts::nodeTitleFontSize = 13;
const qreal VuoRendererFonts::nodeClassFontSize = 10;
const qreal VuoRendererFonts::portTitleFontSize = 11;
const qreal VuoRendererFonts::portDetailFontSize = 10;

/**
 * Returns a shared font provider.
 *
 * Don't free this object.
 */
VuoRendererFonts * VuoRendererFonts::getSharedFonts(void)
{
	if (! sharedFonts)
		sharedFonts = new VuoRendererFonts();

	return sharedFonts;
}

/**
 * Loads fonts.
 *
 * This private constructor should only be called once per process.
 */
VuoRendererFonts::VuoRendererFonts(void)
{
	addFont("PTS55F.ttf");
	addFont("PTS75F.ttf");
}

/**
 * Loads the font with the given file name.
 */
void VuoRendererFonts::addFont(QString font)
{
	// Try loading from the Editor app bundle
	{
		QString fontPath = QDir(QApplication::applicationDirPath().append("/../Resources/").append(font)).canonicalPath();
		if (fontPath.count() && QFontDatabase::addApplicationFont(fontPath) != -1)
			return;
	}

	// Try loading from the SDK
	{
		QString fontPath = QDir(QApplication::applicationDirPath().append("/resources/").append(font)).canonicalPath();
		if (fontPath.count() && QFontDatabase::addApplicationFont(fontPath) != -1)
			return;
	}

	// Try loading from the source tree (e.g., when running tests).
	{
		QString fontPath = QDir(QApplication::applicationDirPath().append("/../../renderer/font/").append(font)).canonicalPath();
		if (fontPath.count() && QFontDatabase::addApplicationFont(fontPath) != -1)
			return;
	}

	VUserLog("Error: Failed to open '%s'.", font.toUtf8().data());
}

/**
 * Returns the font for a node's title.
 */
QFont VuoRendererFonts::nodeTitleFont(void)
{
	return QFont(fontFamily, nodeTitleFontSize, QFont::Bold, false);
}

/**
 * Returns the font for a node's class name.
 */
QFont VuoRendererFonts::nodeClassFont(void)
{
	return QFont(fontFamily, nodeClassFontSize, QFont::Light, false);
}

/**
 * Returns the font for a port's title.
 */
QFont VuoRendererFonts::nodePortTitleFont(void)
{
	return QFont(fontFamily, portTitleFontSize, QFont::Light, false);
}

/**
 * Returns the font for a port's constant flag or typecast.
 */
QFont VuoRendererFonts::nodePortConstantFont(void)
{
	return QFont(fontFamily, portDetailFontSize, QFont::Light, false);
}

/**
 * Returns the font for a port's popover text.
 */
QFont VuoRendererFonts::portPopoverFont(void)
{
	return QFont(fontFamily, portTitleFontSize, QFont::Light, false);
}

/**
 * Returns the font for dialog box headings.
 */
QFont VuoRendererFonts::dialogHeadingFont()
{
	return QFont(fontFamily, 14, QFont::Bold, false);
}

/**
 * Returns the font for text in dialog boxes.
 */
QFont VuoRendererFonts::dialogBodyFont()
{
	return QFont(fontFamily, 12, QFont::Light, false);
}

/**
 * Returns the font for comments.
 */
QFont VuoRendererFonts::commentFont(void)
{
	return QFont(fontFamily, 14, QFont::Light, false);
}

/**
 * Returns the horizontal distance between the left side of the bounding box and the left side of the first glyph in @c text,
 * to compensate for the empty space left of the first glyph that causes multiple lines of text to appear misaligned.
 */
double VuoRendererFonts::getHorizontalOffset(const QFont font, const QString text)
{
	if (text.length() == 0)
		return 0;

	QPainterPath p;
	p.addText(0, 0, font, text[0]);
	return p.boundingRect().x();
}

/**
 * Returns a CSS string selecting the specified font.
 */
QString VuoRendererFonts::getCSS(QFont font)
{
	int qtweight = font.weight();
	int cssweight = 400; // "normal"
	if (qtweight < 40)
		cssweight = 300; // "light"
	else if (qtweight > 60)
		cssweight = 700; // "bold"

	return QString("font-family: %1; font-size: %2pt; font-weight: %3;")
			.arg(font.family())
			.arg(font.pointSizeF())
			.arg(cssweight);
}
