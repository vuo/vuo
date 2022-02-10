/**
 * @file
 * VuoCompositionMetadataPanel implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCompositionMetadataPanel.hh"

#include "VuoComposition.hh"
#include "VuoCompositionMetadata.hh"
#include "VuoRendererFonts.hh"
#include "VuoStringUtilities.hh"
#include "VuoEditor.hh"
#include "VuoEditorComposition.hh"
#include "VuoNodePopover.hh"

const int VuoCompositionMetadataPanel::defaultPopoverTextWidth = 192;
const int VuoCompositionMetadataPanel::margin = 8;
const QString VuoCompositionMetadataPanel::editLink = "vuo-edit";

/**
 * Creates a new panel widget to display information about a composition.
 */
VuoCompositionMetadataPanel::VuoCompositionMetadataPanel(VuoComposition *composition, QWidget *parent) :
	VuoPanelDocumentation(parent)
{
	this->composition = composition;
	this->isUserComposition = true;

	// Text content
	this->textLabel = new QLabel("", this);
	textLabel->setText(generateCompositionMetadataText());
	textLabel->setTextInteractionFlags(textLabel->textInteractionFlags() | Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
	textLabel->setOpenExternalLinks(false);
	connect(textLabel, &QLabel::linkActivated, this, &VuoCompositionMetadataPanel::handleMetadataLinkClick);

	// Layout
	layout = new QVBoxLayout(this);
	layout->setContentsMargins(margin, margin, margin, margin);
	layout->addWidget(textLabel, 0, Qt::AlignTop);
	layout->setStretch(0, 0);

	// Style
	setStyle();

	VuoEditor *editor = (VuoEditor *)qApp;
	connect(editor, &VuoEditor::darkInterfaceToggled, this, &VuoCompositionMetadataPanel::update);
}

/**
 * Sets the panel's style attributes.
 * @todo https://b33p.net/kosada/node/9090: Copied from VuoNodePopover::setStyle(); re-factor.
 */
void VuoCompositionMetadataPanel::setStyle()
{
	textLabel->setFont(VuoRendererFonts::getSharedFonts()->portPopoverFont());
	textLabel->setMargin(0);
	textLabel->setFixedWidth(defaultPopoverTextWidth);
	textLabel->setWordWrap(true);

	Qt::WindowFlags flags = windowFlags();
	flags |= Qt::FramelessWindowHint;
	flags |= Qt::WindowStaysOnTopHint;
	flags |= Qt::ToolTip;
	setWindowFlags(flags);

	// No border around embedded QGraphicsView
	setBackgroundRole(QPalette::Base);
	QPalette pal;
	pal.setColor(QPalette::Base, QColor(Qt::transparent));
	setPalette(pal);

	QSizePolicy sizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
	textLabel->setSizePolicy(sizePolicy);

	setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
}

/**
 * Draws the composition metadata panel.
 */
void VuoCompositionMetadataPanel::paintEvent(QPaintEvent *event)
{
	VuoEditor *editor = (VuoEditor *)qApp;
	bool isDark = editor->isInterfaceDark();

	QPainter painter(this);
	QRect popoverRect(0, 0, width(), height());
	QColor popoverColor = isDark ? QColor("#282828") : QColor("#f9f9f9");

	painter.setBrush(popoverColor);
	painter.setRenderHint(QPainter::Antialiasing, true);
	painter.fillRect(popoverRect, popoverColor);
}

/**
 * Generates a descriptive text string containing information about the panel's associated composition.
 */
QString VuoCompositionMetadataPanel::generateCompositionMetadataText()
{
	if (!composition || !composition->hasRenderer())
		return "";

	QString name = static_cast<VuoEditorComposition *>(composition->getRenderer())->getFormattedName();

	VuoCompositionMetadata *metadata = composition->getMetadata();
	QString compositionVersion = QString::fromStdString(metadata->getVersion());
	QString createdInVuoVersion = QString::fromStdString(metadata->getCreatedInVuoVersion());
	QString lastSavedInVuoVersion = QString::fromStdString(metadata->getLastSavedInVuoVersion());
	string description = metadata->getDescription();
	string copyright = metadata->getCopyright();
	string homepageURL = metadata->getHomepageURL();
	string documentationURL = metadata->getDocumentationURL();

	QString formattedName = QString("<h2>%1</h2>").arg(name);

	//:	Appears in the documentation panel at the bottom of the node library.
	//: Refers to a composition version string specified by the composition author in the Edit > Composition Information dialog.
	QString formattedCompositionVersion = isUserComposition && !compositionVersion.isEmpty()
		? "<span>" + tr("Version %1").arg(compositionVersion) + "</span><br>\n"
		: "";

	//:	Appears in the documentation panel at the bottom of the node library.
	//: Refers to the Vuo version in which the composition was created.
	QString formattedCreatedInVuoVersion = isUserComposition && !createdInVuoVersion.isEmpty()
		? "<span>" + tr("Created in Vuo %1").arg(createdInVuoVersion) + "</span><br>\n"
		: "";

	//:	Appears in the documentation panel at the bottom of the node library.
	//: Refers to the Vuo version in which the composition was most recently saved.
	QString formattedLastSavedInVuoVersion = isUserComposition && !lastSavedInVuoVersion.isEmpty()
		? "<span>" + tr("Last saved in Vuo %1").arg(lastSavedInVuoVersion) + "</span><br>\n"
		: "";

	QString formattedVersionInfo = formattedCompositionVersion
								   .append(formattedCreatedInVuoVersion)
								   .append(formattedLastSavedInVuoVersion);
	if (!formattedVersionInfo.isEmpty())
		formattedVersionInfo.append("<br>\n");

	QString formattedDescription = "";
	if (!description.empty() && (description != VuoRendererComposition::deprecatedDefaultDescription))
		formattedDescription = VuoStringUtilities::generateHtmlFromMarkdown(description).c_str();

	QString formattedCopyright= "";
	// Show the copyright for community-authored compositions
	// (can't use isUserComposition, since we want to show the original author for community-authored built-in example compositions).
	if (!copyright.empty() && copyright.find("Kosada Incorporated") == string::npos)
		formattedCopyright = VuoStringUtilities::generateHtmlFromMarkdown(copyright).c_str();

	QString formattedLinks = "";
	if (!homepageURL.empty() || !documentationURL.empty())
	{
		formattedLinks = "<p>";
		if (!homepageURL.empty())
			formattedLinks.append(QString("<a href=\""))
					.append(homepageURL.c_str())
					.append("\">Homepage</a>\n");

		if (!homepageURL.empty() && !documentationURL.empty())
			formattedLinks.append(QString("<br>\n"));

		if (!documentationURL.empty())
			formattedLinks.append(QString("<a href=\""))
							   .append(documentationURL.c_str())
							   .append("\">Documentation</a>\n");

		formattedLinks.append("</p>");
	}

	QString editMetadataLink = (isUserComposition?
									  QString("<p><a href=\"")
									  .append(VuoCompositionMetadataPanel::editLink)
									  .append("\">Edit Composition Information…</a></p>") : "");

	return VuoNodePopover::generateTextStyleString()
			+ formattedName
			.append(formattedDescription)
			.append(formattedCopyright)
			.append(formattedLinks)
			.append(formattedVersionInfo)
			.append(editMetadataLink);
}

/**
  * Sets the fixed text width of the panel to the provided
  * @c width minus `2 * VuoCompositionMetadataPanel::margin`.
  * @todo https://b33p.net/kosada/node/9090: Copied from VuoNodePopover::setTextWidth(int width); re-factor.
  */
void VuoCompositionMetadataPanel::setTextWidth(int width)
{
	int adjustedWidth = width-2*margin;
	if (adjustedWidth != textLabel->width())
	{
		textLabel->setFixedWidth(adjustedWidth);

		// The following setWordWrap() call is for some reason necessary
		// to ensure that the popover does not end up with empty
		// vertical space when resized. (Word wrap remains on regardless.)
		textLabel->setWordWrap(true);
	}
}

/**
  * Returns the currently selected text within this widget's text field,
  * or the empty string of not applicable.
  */
QString VuoCompositionMetadataPanel::getSelectedText()
{
	if (textLabel->hasSelectedText())
		return textLabel->selectedText();
	else
		return QString("");
}

/**
 * Updates the widget with the current background color and text content.
 */
void VuoCompositionMetadataPanel::update()
{
	textLabel->setText(generateCompositionMetadataText());
}

/**
  * Handles clicks to the composition's metadata panel.
  */
void VuoCompositionMetadataPanel::handleMetadataLinkClick(const QString &url)
{
	if (url == VuoCompositionMetadataPanel::editLink)
		emit metadataEditRequested();
	else
		QDesktopServices::openUrl(url);
}

/**
  * Sets the boolean indicating whether or not this is a user-created
  * composition (rather than a built-in example composition).
  */
void VuoCompositionMetadataPanel::setIsUserComposition(bool userComposition)
{
	if (userComposition != this->isUserComposition)
	{
		this->isUserComposition = userComposition;
		update();
	}
}

/**
 * Destroys a panel.
 */
VuoCompositionMetadataPanel::~VuoCompositionMetadataPanel()
{
}
