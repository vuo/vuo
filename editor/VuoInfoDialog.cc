/**
 * @file
 * VuoInfoDialog implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoInfoDialog.hh"

#include "VuoRendererFonts.hh"

/**
 * Creates a dialog that is ready to be shown.
 *
 * @param parent Parent widget.
 * @param summary Brief message, translated.
 * @param details Longer message, translated and wrapped in `<p>` tags.
 * @param checkboxLabel "Show this dialog" message to appear next to checkbox, translated.
 * @param settingsKey For looking up and storing checkbox status in `QSettings`.
 */
VuoInfoDialog::VuoInfoDialog(QWidget *parent, QString summary, QString details, QString checkboxLabel, QString settingsKey)
	: QMessageBox(parent)
{
	this->summary = summary;
	this->details = details;
	this->checkboxLabel = checkboxLabel;
	this->settingsKey = settingsKey;
}

/**
 * Displays a modal dialog, unless the user has previously unchecked the box to show it.
 */
void VuoInfoDialog::show()
{
	auto settings = new QSettings;
	if (!settings->value(settingsKey, true).toBool())
	{
		emit notShown();
		return;
	}

	VuoRendererFonts *fonts = VuoRendererFonts::getSharedFonts();

	setWindowFlags(Qt::Sheet);
	setWindowModality(Qt::WindowModal);
	setFont(fonts->dialogHeadingFont());
	setTextFormat(Qt::RichText);
	setText(summary);
	setInformativeText("<style>p{" + fonts->getCSS(fonts->dialogBodyFont()) + "}</style>" + details);
	setIcon(QMessageBox::Information);

	QCheckBox *cb = new QCheckBox(this);
	cb->setFont(fonts->dialogBodyFont());
	cb->setText(checkboxLabel);
	cb->setChecked(true);
	setCheckBox(cb);

	connect(this, &QMessageBox::finished, [=](){
		if (!cb->isChecked())
			settings->setValue(settingsKey, false);
		delete settings;
	});

	open();
}
