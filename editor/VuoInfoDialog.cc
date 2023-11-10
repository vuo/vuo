/**
 * @file
 * VuoInfoDialog implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
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
 * Creates a dialog without a checkbox that is ready to be shown.
 *
 * @param parent Parent widget.
 * @param summary Brief message, translated.
 * @param details Longer message, translated and wrapped in `<p>` tags.
 */
VuoInfoDialog::VuoInfoDialog(QWidget *parent, QString summary, QString details)
	: QMessageBox(parent)
{
	this->summary = summary;
	this->details = details;
}

/**
 * Displays a modal dialog, unless the user has previously unchecked the box to show it.
 */
void VuoInfoDialog::show()
{
	bool hasCheckbox = ! settingsKey.isEmpty() && ! checkboxLabel.isEmpty();

	QSettings *settings = nullptr;
	if (hasCheckbox)
	{
		settings = new QSettings;
		if (!settings->value(settingsKey, true).toBool())
		{
			emit notShown();
			return;
		}
	}

	VuoRendererFonts *fonts = VuoRendererFonts::getSharedFonts();

	setWindowFlags(Qt::Sheet);
	setWindowModality(Qt::WindowModal);
	setFont(fonts->dialogHeadingFont());
	setTextFormat(Qt::RichText);
	setText(summary);
	setInformativeText("<style>p{" + fonts->getCSS(fonts->dialogBodyFont()) + "}</style>" + details);
	setIcon(QMessageBox::Information);

	// Make the dialog wide enough to show "⚠️ The app requires macOS on an Apple Silicon (ARM64/M1/M2/M3) CPU." without wrapping.
	setStyleSheet("QLabel{min-width: 410px;}");

	QCheckBox *cb = nullptr;
	if (hasCheckbox)
	{
		cb = new QCheckBox(this);
		cb->setFont(fonts->dialogBodyFont());
		cb->setText(checkboxLabel);
		cb->setChecked(true);
		setCheckBox(cb);
	}

	if (hasCheckbox)
	{
		connect(this, &QMessageBox::finished, [=](){
			if (!cb->isChecked())
				settings->setValue(settingsKey, false);
			delete settings;
		});
	}

	open();
}
