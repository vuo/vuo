/**
 * @file
 * VuoMetadataEditor interface.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

class VuoEditorComposition;
class VuoCompositionMetadata;

namespace Ui
{
	class VuoMetadataEditor;
}

/**
 * Provides a dialog to edit a composition's metadata.
 */
class VuoMetadataEditor : public QDialog
{
	Q_OBJECT

public:
	explicit VuoMetadataEditor(VuoEditorComposition *composition, QWidget *parent, Qt::WindowFlags flags, bool isCodeEditor=false);
	~VuoMetadataEditor();
	void show();
	VuoCompositionMetadata * toMetadata(void);

	static QString generateBundleIdentifierForCompositionName(QString name);
	static QString generateFxPlugGroupName();

protected:
	void dragEnterEvent(QDragEnterEvent *event);
	void dropEvent(QDropEvent *event);
	void keyPressEvent(QKeyEvent *event);

private:
	Ui::VuoMetadataEditor *ui;
	VuoEditorComposition *composition;
	QString iconURL;
	static map<QString, QString> descriptionForLicense;
	static const QString otherLicenseTitle;
	static const QString placeholderIconText;

	void populateLicenseInfo();
	QPair<QString, QString> getLicenseTextAndLinkFromDescription(QString description);
	QString getLicense();
	static vector<string> commaSeparatedListToStdVector(const QString &listText);

private slots:
	void updateLicenseFields(QString licenseTitle);
	void recordCustomLicenseText();
	void updatePlaceholderTexts();
	void sanitizeURL();
};
