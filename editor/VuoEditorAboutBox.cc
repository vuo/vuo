/**
 * @file
 * VuoEditorAboutBox implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoStringUtilities.hh"
#include "VuoEditorAboutBox.hh"
#include "ui_VuoEditorAboutBox.h"

const QString VuoEditorAboutBox::htmlHead = "<head><style>a { color: #0099a9; }</style></head>";

/**
 * Creates an instance of the About Box dialog.
 */
VuoEditorAboutBox::VuoEditorAboutBox(QWidget *parent) :
	VuoDialogWithoutTitlebar(parent),
	ui(new Ui::VuoEditorAboutBox)
{
	ui->setupUi(this);

	QString buildDate = __DATE__;

	QString versionAndBuild = VUO_VERSION_AND_BUILD_STRING;
	QRegularExpressionMatch match = QRegularExpression("\\d+\\.\\d+\\.\\d+").match(versionAndBuild);
	QString versionNumber = match.captured(0);
	QString versionSuffix = versionAndBuild.mid(match.capturedEnd());
	if (!versionSuffix.isEmpty())
		versionSuffix = "<span style=\"font-size:20pt; color:rgba(0,0,0,.35)\">" + versionSuffix + "</span>";

	QString commitHash = VUO_GIT_COMMIT;
	if (!commitHash.isEmpty())
		commitHash = QString(" (revision ") + VUO_GIT_COMMIT + ")";

	// Keep this description in sync with VuoManual.txt.
	QString missionMarkdown = tr(
		"**Create memorable interactive experiences — without coding.**\n"
		"\n"
		"Drag, drop, and connect Vuo's simple building blocks to support your creative work:  \n"
		"VJ gigs, exhibits, installations, stage productions, dome graphics, visual signage,  \n"
		"video effects, maker projects, trade show booths, and more.  \n"
		"\n"
		"Learn more at %1.")
		.arg("[vuo.org](https://vuo.org)");

	QString missionHTML = QString::fromStdString(VuoStringUtilities::generateHtmlFromMarkdown(missionMarkdown.toStdString()));

	QString descriptiveText = \
			QString("<html>%3<body>"
					"<p><span style=\"font-size:28pt;\">Vuo</span>"
						"<span style=\"font-size:20pt;\"> %1</span>%5<br>"
						"<span style=\"font-size:12pt; color:rgba(0,0,0,.5)\">" + tr("Built on %2") + "<br>"
						"Copyright © 2012–2023 Kosada Incorporated</span></p>%4"
					"</body></html>").
			arg(
				versionNumber,
				buildDate + commitHash,
				htmlHead,
				missionHTML,
				versionSuffix
			);

	// Text style
	contributorFont = ui->textEdit->font();

	licenseFont.setFamily("Monaco");
	licenseFont.setStyleHint(QFont::Monospace);
	licenseFont.setFixedPitch(true);
	licenseFont.setPointSize(11);

	int spacesPerTab = 8;
	QFontMetricsF fontMetrics(licenseFont);
	ui->textEdit->setTabStopDistance(spacesPerTab * fontMetrics.horizontalAdvance(' '));

	// Set the introductory text.
	ui->textLabel->setText(descriptiveText);

	// Populate the "Details" pane.
	contributors = new QTreeWidgetItem(ui->treeWidget);
	contributors->setText(0, tr("Contributors"));
	dependencies = new QTreeWidgetItem(ui->treeWidget);
	dependencies->setText(0, tr("Dependencies"));

	// Populate the text file content panes.
	contributorsText = fetchMarkdown("CONTRIBUTORS.md", tr("Please see the list of contributors at <a href=\"%1\">%1</a>.").arg("https://community.vuo.org/badges"));
	dependenciesText = fetchMarkdown("DEPENDENCIES.md", tr("Please see the list of dependencies at <a href=\"%1\">%1</a>.").arg("https://github.com/vuo/vuo/blob/main/conanfile.py"));
	populateLicenseTexts();

	ui->textEdit->setReadOnly(true);

	// Update the text content in response to selection changes within the "Details" pane.
	connect(ui->treeWidget, &QTreeWidget::currentItemChanged, this, &VuoEditorAboutBox::updateDisplayedTextContent);

	// Select the contributors list by default.
	ui->treeWidget->setCurrentItem(contributors);

	setFixedSize(size());
	setAttribute(Qt::WA_DeleteOnClose, false);
}

/**
 * Loads the content of the specified Markdown file.
 * Falls back to placeholder text if the file doesn't exist.
 */
QString VuoEditorAboutBox::fetchMarkdown(const QString &filename, const QString &fallback)
{
	QFile f(QApplication::applicationDirPath().append("/../Resources/").append(filename));
	if (!f.open(QIODevice::ReadOnly))
		return fallback;

	QTextStream textStream(&f);
	return QString::fromStdString(VuoStringUtilities::generateHtmlFromMarkdown(textStream.readAll().toUtf8().constData()));
}

/**
 * Load the content of the dependency license texts.
 */
void VuoEditorAboutBox::populateLicenseTexts()
{
	QDir licenseTextDir(QApplication::applicationDirPath().append("/../Frameworks/Vuo.framework/Documentation/Licenses"));
	if (!licenseTextDir.exists())
		return;

	QStringList licenseFileList(licenseTextDir.entryList(QDir::Files|QDir::Readable));
	foreach (QString licenseFileName, licenseFileList)
	{
		QFile licenseFile(licenseTextDir.filePath(licenseFileName));
		if (licenseFile.open(QIODevice::ReadOnly))
		{
			QFileInfo licenseFileInfo(licenseFile);
			QString dependencyIdentifier = licenseFileInfo.completeBaseName();
			QTreeWidgetItem *dependencyLicenseItem = new QTreeWidgetItem();
			dependencyLicenseItem->setText(0, dependencyIdentifier);
			dependencies->addChild(dependencyLicenseItem);

			QTextStream textStream(&licenseFile);
			licenseTextForDependency[dependencyIdentifier] = textStream.readAll();
		}
	}
}

/**
 * Display the appropriate content within the text file content pane, given
 * the item currently selected within the "Details" pane.
 */
void VuoEditorAboutBox::updateDisplayedTextContent()
{
	QTreeWidgetItem *currentItem = ui->treeWidget->currentItem();
	if (currentItem == contributors)
	{
		ui->textEdit->setFont(contributorFont);
		ui->textEdit->setText(htmlHead + contributorsText);
	}
	else if (currentItem == dependencies)
	{
		ui->textEdit->setFont(contributorFont);
		ui->textEdit->setText(htmlHead + dependenciesText);
	}
	else
	{
		ui->textEdit->setFont(licenseFont);

		// Use ui->textEdit->document()->setHtml(...) and ui->textEdit->document()->setPlainText(...)
		// rather than ui->textEdit->setText(...) to avoid bug where viewing a license text after
		// clicking on any link within the contributor text caused the full license text to
		// be displayed as a link.
		QString licenseText = licenseTextForDependency[currentItem->text(0)];
		if (Qt::mightBeRichText(licenseText))
			ui->textEdit->document()->setHtml(htmlHead + licenseText);
		else
			ui->textEdit->document()->setPlainText(licenseText);
	}
}

VuoEditorAboutBox::~VuoEditorAboutBox()
{
	delete ui;
}
