/**
 * @file
 * VuoEditorAboutBox interface.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

namespace Ui
{
	class VuoEditorAboutBox;
}

#include "VuoDialogWithoutTitlebar.hh"

/**
 * Provides the Vuo Editor's "About Vuo Editor" dialog.
 */
class VuoEditorAboutBox : public VuoDialogWithoutTitlebar
{
	Q_OBJECT

public:
	explicit VuoEditorAboutBox(QWidget *parent = 0);
	~VuoEditorAboutBox();

private:
	static const QString htmlHead;
	Ui::VuoEditorAboutBox *ui;
	QFont contributorFont;
	QFont licenseFont;
	QString contributorText;
	map<QString, QString> licenseTextForDependency;
	QTreeWidgetItem *contributors;
	QTreeWidgetItem *dependencyLicenses;

	void populateContributorsList();
	void populateLicenseTexts();

private slots:
	void updateDisplayedTextContent();
};
