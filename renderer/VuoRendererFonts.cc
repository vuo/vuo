/**
 * @file
 * VuoRendererFonts implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoRendererFonts.hh"
#include "VuoCompiler.hh"


VuoRendererFonts *VuoRendererFonts::sharedFonts = NULL;

const qreal VuoRendererFonts::thickPenWidth = 20;
const qreal VuoRendererFonts::midPenWidth = VuoRendererFonts::thickPenWidth/10.0;

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
	addFont("Signika-Light.otf");
	addFont("Signika-Semibold.otf");
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

	fprintf(stderr, "Failed to open \"%s\"\n", font.toUtf8().data());
}

/**
 * Returns the font for a node's title.
 */
QFont VuoRendererFonts::nodeTitleFont(void)
{
	return QFont("Signika", thickPenWidth*7.0/8.0, QFont::Bold, false);
}

/**
 * Returns the font for a node's class name.
 */
QFont VuoRendererFonts::nodeClassFont(void)
{
	return QFont("Signika", thickPenWidth*9.0/16.0, QFont::Normal, false);
}

/**
 * Returns the font for a port's title or a constant flag's text.
 */
QFont VuoRendererFonts::nodePortTitleFont(void)
{
	return QFont("Signika", thickPenWidth*9.0/16.0, QFont::Normal, false);
}

/**
 * Returns the font for a port's popover text.
 */
QFont VuoRendererFonts::portPopoverFont(void)
{
	return QFont("Signika", thickPenWidth*9.0/16.0, QFont::Normal, false);
}
