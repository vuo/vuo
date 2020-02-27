/**
 * @file
 * VuoErrorDialog implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoErrorDialog.hh"

#include "VuoEditorUtilities.hh"
#include "VuoRendererFonts.hh"

/**
 * Displays a modal dialog.
 */
void VuoErrorDialog::show(QWidget *parent, QString summary, QString details, QString disclosureDetails)
{
	VuoRendererFonts *fonts = VuoRendererFonts::getSharedFonts();

	QMessageBox messageBox(parent);
	messageBox.setWindowFlags(Qt::Sheet);
	messageBox.setWindowModality(Qt::WindowModal);
	messageBox.setFont(fonts->dialogHeadingFont());
	messageBox.setTextFormat(Qt::RichText);
	messageBox.setText(summary);

	// Capitalize, so VuoCompiler exceptions (which typically start with a lowercase letter) look better.
	details[0] = details[0].toUpper();
	messageBox.setInformativeText("<style>p{" + fonts->getCSS(fonts->dialogBodyFont()) + "}</style><p>" + details + "</p>");

	messageBox.setDetailedText(disclosureDetails);
	messageBox.setStandardButtons(QMessageBox::Ok);
	messageBox.setIconPixmap(VuoEditorUtilities::vuoLogoForDialogs());

	foreach (QObject *child, messageBox.children())
	{
		// Customize the style of the disclosure details text.
		if (QString(child->metaObject()->className()) == "QMessageBoxDetailsText")
		{
			// Match the font of the informative text (child widget with objectName() =="qt_msgbox_informativelabel")
			(dynamic_cast<QWidget *>(child))->setFont(fonts->dialogBodyFont());

			// Increase the height of the disclosure details text box so that when a composition has one missing node,
			// the entire detailed text can be viewed without scrolling.
			foreach (QObject *grandchild, child->children())
			{
				if (dynamic_cast<QTextEdit *>(grandchild))
					(dynamic_cast<QTextEdit *>(grandchild))->setFixedHeight(110);
			}
		}
	}

	messageBox.exec();
}
