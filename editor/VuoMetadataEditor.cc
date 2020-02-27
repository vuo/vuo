/**
 * @file
 * VuoMetadataEditor implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoMetadataEditor.hh"
#include "ui_VuoMetadataEditor.h"

#include "VuoComposition.hh"
#include "VuoCompositionMetadata.hh"
#include "VuoEditor.hh"
#include "VuoEditorComposition.hh"
#include "VuoEditorWindow.hh"
#include "VuoFileUtilities.hh"
#include "VuoStringUtilities.hh"

map<QString, QString> VuoMetadataEditor::descriptionForLicense;
const QString VuoMetadataEditor::otherLicenseTitle = "Other";
const QString VuoMetadataEditor::placeholderIconText = "Icon for Exported App";

/**
 * Creates an instance of a dialog to edit a composition's metadata.
 */
VuoMetadataEditor::VuoMetadataEditor(VuoEditorComposition *composition, QWidget *parent, Qt::WindowFlags flags, bool isCodeEditor) :
	QDialog(parent, flags),
	ui(new Ui::VuoMetadataEditor)
{
	this->composition = composition;

	ui->setupUi(this);
	ui->tabWidget->setCurrentIndex(0);
	setFixedSize(size());

	ui->descriptionDetailsLabel->setText("<style>a{color:#0099a9;}</style>"
		+ tr("What is the purpose of this composition?  You can use <a href='%1'>Markdown</a> formatting.")
		  .arg("https://daringfireball.net/projects/markdown/basics"));

	// @todo https://b33p.net/kosada/node/4946 Limit drop area to icon preview frame.
	setAcceptDrops(true);

	populateLicenseInfo();

	// Dynamically update placeholder texts that are derived from other text field values.
	connect(ui->name, &QLineEdit::textEdited, this, &VuoMetadataEditor::updatePlaceholderTexts);

	// Update license text and link for the current menu selection.
	connect(ui->licenseTitle, &QComboBox::currentTextChanged, this, &VuoMetadataEditor::updateLicenseFields);
	connect(ui->licenseText, &QTextEdit::textChanged, this, &VuoMetadataEditor::recordCustomLicenseText);
	connect(ui->licenseLink, &QLineEdit::textChanged, this, &VuoMetadataEditor::recordCustomLicenseText);

	connect(ui->homepageLink, &QLineEdit::editingFinished, this, &VuoMetadataEditor::sanitizeURL);
	connect(ui->documentationLink, &QLineEdit::editingFinished, this, &VuoMetadataEditor::sanitizeURL);

	// If editing text code, hide fields that apply only to compositions.
	ui->iconFrame->setVisible(! isCodeEditor);
	ui->iconLabel->setVisible(! isCodeEditor);
	ui->bundleIdentifier->setVisible(! isCodeEditor);
	ui->bundleIdentifierLabel->setVisible(! isCodeEditor);
	ui->bundleIdentifierDetailsLabel->setVisible(! isCodeEditor);
	ui->fxPlugGroup->setVisible(! isCodeEditor);
	ui->fxPlugGroupLabel->setVisible(! isCodeEditor);
	ui->fxPlugGroupDetailsLabel->setVisible(! isCodeEditor);

	// If editing a composition, hide fields that apply only to text code.
	ui->categories->setVisible(isCodeEditor);
	ui->categoriesLabel->setVisible(isCodeEditor);
	ui->categoriesDetailsLabel->setVisible(isCodeEditor);
}

/**
 * Retrieves the composition's current metadata values and displays the dialog.
 */
void VuoMetadataEditor::show()
{
	// Name
	ui->name->setText(QString::fromStdString( composition->getBase()->getMetadata()->getCustomizedName() ));

	// Version
	ui->version->setText(composition->getBase()->getMetadata()->getVersion().c_str());

	// Icon
	iconURL = composition->getBase()->getMetadata()->getIconURL().c_str();
	if (!iconURL.isEmpty())
	{
		string relativeDir, file, ext;
		VuoFileUtilities::splitPath(iconURL.toUtf8().constData(), relativeDir, file, ext);
		string iconFileName = file;
		if (!ext.empty())
		{
			iconFileName += ".";
			iconFileName += ext;
		}

		QDir compositionDir(QDir(composition->getBase()->getDirectory().c_str()).canonicalPath());
		QString iconAbsolutePath = compositionDir.filePath(QDir(relativeDir.c_str()).filePath(iconFileName.c_str()));

		QImageReader reader(iconAbsolutePath);
		if (reader.canRead())
		{
			QIcon icon(iconAbsolutePath);
			QPixmap pixmap = icon.pixmap(icon.actualSize(ui->iconLabel->size()-QSize(4,4)));
			ui->iconLabel->setPixmap(pixmap);
		}
		else
			ui->iconLabel->setText(placeholderIconText);
	}
	else // if (iconURL.isEmpty())
	{
		ui->iconLabel->setText(placeholderIconText);
	}

	// Description
	ui->description->setText(composition->getBase()->getMetadata()->getDescription().c_str());
	ui->description->setFocus();
	ui->description->moveCursor(QTextCursor::End);

	// Keywords
	string keywords = VuoStringUtilities::join(composition->getBase()->getMetadata()->getKeywords(), ", ");
	ui->keywords->setText(QString::fromStdString(keywords));
	ui->keywords->moveCursor(QTextCursor::End);

	// Copyright
	ui->copyright->setText(composition->getBase()->getMetadata()->getCopyright().c_str());
	ui->copyright->moveCursor(QTextCursor::End);

	// License
	QString license = composition->getBase()->getMetadata()->getLicense().c_str();
	QPair<QString, QString> licenseFields = getLicenseTextAndLinkFromDescription(license);
	auto licenseFieldsHTTPS = QPair<QString, QString>(licenseFields.first.replace( "http:", "https:"),
													  licenseFields.second.replace("http:", "https:"));

	bool foundLicenseMatchInStandardOptions = false;
	for (int i = 0; i < ui->licenseTitle->count() && !foundLicenseMatchInStandardOptions; ++i)
	{
		QString currentLicenseTitle = ui->licenseTitle->itemText(i);
		QString currentLicenseDescription = descriptionForLicense[currentLicenseTitle];
		QPair<QString, QString> currentLicenseFields = getLicenseTextAndLinkFromDescription(currentLicenseDescription);

		if (((licenseFields.first      == currentLicenseFields.first) && (licenseFields.second      == currentLicenseFields.second))
		 || ((licenseFieldsHTTPS.first == currentLicenseFields.first) && (licenseFieldsHTTPS.second == currentLicenseFields.second)))
		{
			ui->licenseTitle->setCurrentText(currentLicenseTitle);
			foundLicenseMatchInStandardOptions = true;
		}
	}
	if (!foundLicenseMatchInStandardOptions)
	{
		descriptionForLicense[otherLicenseTitle] = license;
		ui->licenseTitle->setCurrentText(otherLicenseTitle);
	}

	updateLicenseFields(ui->licenseTitle->currentText());

	// Homepage URL
	ui->homepageLink->setText(composition->getBase()->getMetadata()->getHomepageURL().c_str());

	// Documentation URL
	ui->documentationLink->setText(composition->getBase()->getMetadata()->getDocumentationURL().c_str());

	// Bundle identifier
	ui->bundleIdentifier->setText(composition->getBase()->getMetadata()->getBundleIdentifier().c_str());

	// FxPlug group
	ui->fxPlugGroup->setText(composition->getBase()->getMetadata()->getFxPlugGroup().c_str());

	// Categories
	string categories = VuoStringUtilities::join(composition->getBase()->getMetadata()->getCategories(), ", ");
	ui->categories->setText(QString::fromStdString(categories));
	ui->categories->moveCursor(QTextCursor::End);

	updatePlaceholderTexts();

	QDialog::show();
}

VuoMetadataEditor::~VuoMetadataEditor()
{
	delete ui;
}

/**
  * Sets up-to-date placeholder texts derived from currently entered metadata values.
  */
void VuoMetadataEditor::updatePlaceholderTexts()
{
	QString name = QString::fromStdString( composition->getBase()->getMetadata()->getName() );
	QString defaultName = QString::fromStdString( composition->getBase()->getMetadata()->getDefaultName() );
	ui->name->setPlaceholderText( composition->formatCompositionFileNameForDisplay(defaultName) );
	ui->version->setPlaceholderText("1.0");
	ui->homepageLink->setPlaceholderText("http://");
	ui->documentationLink->setPlaceholderText("http://");
	ui->bundleIdentifier->setPlaceholderText( generateBundleIdentifierForCompositionName(name) );
	ui->fxPlugGroup->setPlaceholderText(generateFxPlugGroupName());
}

/**
  * Generates a generic bundle identifier for the provided composition name.
  */
QString VuoMetadataEditor::generateBundleIdentifierForCompositionName(QString name)
{
	return QString("com.")
			.append(static_cast<VuoEditor *>(qApp)->getUserName().c_str())
			.append(".")
			.append(VuoEditorWindow::deriveBaseNodeClassNameFromDisplayName(name.toUtf8().constData()).c_str());
}

/**
  * Generates a generic FxPlug group name for the current user.
  */
QString VuoMetadataEditor::generateFxPlugGroupName()
{
	return QString(static_cast<VuoEditor *>(qApp)->getUserName().c_str())
			.append("'s Plugins");
}

/**
 * Returns the metadata entered by the user.
 */
VuoCompositionMetadata * VuoMetadataEditor::toMetadata(void)
{
	VuoCompositionMetadata *metadata = new VuoCompositionMetadata(*composition->getBase()->getMetadata());

	metadata->setName( ui->name->text().toStdString() );
	metadata->setVersion( ui->version->text().toStdString() );
	metadata->setDescription( ui->description->toPlainText().toStdString() );
	metadata->setKeywords( commaSeparatedListToStdVector(ui->keywords->toPlainText()) );
	metadata->setCopyright( ui->copyright->toPlainText().toStdString() );
	metadata->setLicense( getLicense().toStdString() );
	metadata->setIconURL( iconURL.toStdString() );
	metadata->setHomepageURL( ui->homepageLink->text().toStdString() );
	metadata->setDocumentationURL( ui->documentationLink->text().toStdString() );
	metadata->setBundleIdentifier( ui->bundleIdentifier->text().toStdString() );
	metadata->setFxPlugGroup( ui->fxPlugGroup->text().toStdString() );
	metadata->setCategories( commaSeparatedListToStdVector(ui->categories->toPlainText()) );

	return metadata;
}

/**
 * Returns the license text (description and link) entered by the user.
 */
QString VuoMetadataEditor::getLicense()
{
	QString licenseText = ui->licenseText->toPlainText();
	QString licenseLink = ui->licenseLink->text();

	QString fullLicenseString = licenseText;
	if (!licenseText.isEmpty() && !licenseLink.isEmpty())
		fullLicenseString.append(" ");
	if (!licenseLink.isEmpty())
	{
		fullLicenseString.append("For more information, see ");
		fullLicenseString.append(licenseLink);
		fullLicenseString.append(" .");
	}

	return fullLicenseString;
}

/**
 * Splits comma-separated text and trims whitespace from each item.
 */
vector<string> VuoMetadataEditor::commaSeparatedListToStdVector(const QString &listText)
{
	vector<string> vec;

	QStringList list = listText.split(',', QString::SkipEmptyParts);
	for (const QString &item : list)
		vec.push_back(item.trimmed().toStdString());

	return vec;
}

/**
 * Decide whether we can accept dragged data.
 */
void VuoMetadataEditor::dragEnterEvent(QDragEnterEvent *event)
{
	const QMimeData *mimeData = event->mimeData();

	// Accept drags of icon (.icns or image) urls.
	if (mimeData->hasFormat("text/uri-list"))
	{
		QList<QUrl> urls = mimeData->urls();
		if (urls.size() == 1)
		{
			QString iconAbsolutePath = urls[0].path();
			QImageReader reader(iconAbsolutePath);
			if (reader.canRead())
			{
				event->accept();
				return;
			}
		}
	}

	event->setDropAction(Qt::IgnoreAction);
	event->accept();
}

/**
 * Handle drop events.
 */
void VuoMetadataEditor::dropEvent(QDropEvent *event)
{
	const QMimeData *mimeData = event->mimeData();
	if (mimeData->hasFormat("text/uri-list"))
	{
		// Retrieve the composition directory so that file paths may be specified relative to it.
		QDir compositionDir(QDir(composition->getBase()->getDirectory().c_str()).canonicalPath());
		QList<QUrl> urls = mimeData->urls();

		if (urls.size() == 1)
		{
			QString iconAbsolutePath = urls[0].path();
			string dir, file, ext;
			VuoFileUtilities::splitPath(iconAbsolutePath.toUtf8().constData(), dir, file, ext);

			QImageReader reader(iconAbsolutePath);
			if (reader.canRead())
			{
				iconURL = compositionDir.relativeFilePath(iconAbsolutePath);

				QIcon icon(iconAbsolutePath);
				QPixmap pixmap = icon.pixmap(icon.actualSize(ui->iconLabel->size()-QSize(4,4)));
				ui->iconLabel->setPixmap(pixmap);

				event->accept();
				return;
			}
		}
	}

	event->ignore();
}

/**
 * Handle keypress events.
 */
void VuoMetadataEditor::keyPressEvent(QKeyEvent *event)
{
	if (ui->iconLabel->hasFocus() && ((event->key() == Qt::Key_Backspace) ||
									  (event->key() == Qt::Key_Delete)))
	{
		this->iconURL = "";
		ui->iconLabel->setText(placeholderIconText);
		event->accept();
	}
	else
		QDialog::keyPressEvent(event);
}

#undef BSD

/**
 * Populates the text for standard license options.
 */
void VuoMetadataEditor::populateLicenseInfo()
{
	const QString CC_A = "Creative Commons: Attribution";
	const QString CC_A_ND = "Creative Commons: Attribution + No Derivatives";
	const QString CC_A_NC = "Creative Commons: Attribution + Noncommercial";
	const QString CC_A_NC_ND = "Creative Commons: Attribution + Noncommercial + No Derivatives";
	const QString CC_A_NC_SA = "Creative Commons: Attribution + Noncommercial + Share Alike";
	const QString CC_A_SA = "Creative Commons: Attribution + Share Alike";
	const QString MIT = "MIT";
	const QString BSD = "BSD";
	const QString PublicDomain = "Public Domain";

	descriptionForLicense[CC_A] = "This composition may be modified and distributed under the terms of the Creative Commons: Attribution (CC BY) License. For more information, see https://creativecommons.org/licenses/by/4.0/ .";
	descriptionForLicense[CC_A_ND] = "This composition may be distributed under the terms of the Creative Commons: Attribution + No Derivatives (CC BY-ND) License. For more information, see https://creativecommons.org/licenses/by-nd/4.0/ .";
	descriptionForLicense[CC_A_NC] = "This composition may be modified and distributed under the terms of the Creative Commons: Attribution + Noncommercial (CC BY-NC) License. For more information, see https://creativecommons.org/licenses/by-nc/4.0/ .";
	descriptionForLicense[CC_A_NC_ND] = "This composition may be distributed under the terms of the Creative Commons: Attribution + Noncommercial + No Derivatives (CC BY-NC-ND) License. For more information, see https://creativecommons.org/licenses/by-nc-nd/4.0/ .";
	descriptionForLicense[CC_A_NC_SA] = "This composition may be modified and distributed under the terms of the Creative Commons: Attribution + Noncommercial + Share Alike (CC BY-NC-SA) License. For more information, see https://creativecommons.org/licenses/by-nc-sa/4.0/ .";
	descriptionForLicense[CC_A_SA] = "This composition may be modified and distributed under the terms of the Creative Commons: Attribution + Share Alike (CC BY-SA) License. For more information, see https://creativecommons.org/licenses/by-sa/4.0/ .";
	descriptionForLicense[MIT] = "This composition may be modified and distributed under the terms of the MIT License. For more information, see https://opensource.org/licenses/MIT .";
	descriptionForLicense[BSD] = "This composition may be modified and distributed under the terms of the BSD License. For more information, see https://opensource.org/licenses/BSD-3-Clause .";
	descriptionForLicense[PublicDomain] = "This composition is in the public domain and may be modified and distributed. For more information, see https://creativecommons.org/publicdomain/zero/1.0/ .";
	descriptionForLicense[otherLicenseTitle] = "";

	QList<QString> licenseOptions = QList<QString>() << CC_A << CC_A_ND << CC_A_NC << CC_A_NC_ND << CC_A_NC_SA << CC_A_SA << MIT << BSD << PublicDomain << otherLicenseTitle;
	foreach (QString licenseOption, licenseOptions)
		ui->licenseTitle->addItem(licenseOption);
}

/**
 * Extracts the license text and link from the provided full license description.
 */
QPair<QString, QString> VuoMetadataEditor::getLicenseTextAndLinkFromDescription(QString description)
{
	QRegExp descriptionTemplate("^((.*)\\s+)?For more information, see ([^\\s]*)\\s*\\.\\s*$");
	if (!descriptionTemplate.exactMatch(description))
		return QPair<QString, QString>(description, "");

	QString text = description;
	QString link = description;
	text.replace(descriptionTemplate, "\\2");
	link.replace(descriptionTemplate, "\\3");

	return QPair<QString, QString>(text, link);
}

/**
 * Updates the license text and link fields to provide information about the license named in the @c licenseTitle.
 */
void VuoMetadataEditor::updateLicenseFields(QString licenseTitle)
{
	QString description = descriptionForLicense[licenseTitle];
	QPair<QString, QString> licenseFields = getLicenseTextAndLinkFromDescription(description);
	ui->licenseText->setText(licenseFields.first);
	ui->licenseLink->setText(licenseFields.second);

	bool otherLicenseSelected = (licenseTitle == otherLicenseTitle);
	ui->licenseText->setReadOnly(!otherLicenseSelected);
	ui->licenseLink->setReadOnly(!otherLicenseSelected);

	if (otherLicenseSelected)
	{
		ui->licenseText->setFocus();
		ui->licenseText->moveCursor(QTextCursor::End);
	}
}

/**
 * Maintains an up-to-date record of the license description (text and link)
 * most recently associated with the "Other" option so that it may be restored
 * if the user navigates away from the "Other" option and back again.
 */
void VuoMetadataEditor::recordCustomLicenseText()
{
	if (ui->licenseTitle->currentText() == otherLicenseTitle)
		descriptionForLicense[otherLicenseTitle] = getLicense();
}

/**
 * Verifies that the text entered into the sending lineEdit contains a scheme.
 * Prepends "http://" to the existing text if not.
 */
void VuoMetadataEditor::sanitizeURL()
{
	VuoLineEdit *sender = static_cast<VuoLineEdit *>(QObject::sender());
	if (sender && !sender->text().isEmpty() && !sender->text().contains("://"))
		sender->setText(QString("http://").append(sender->text()));
}
