/**
 * @file
 * VuoNodePopover implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoNodePopover.hh"

#include "VuoCompiler.hh"
#include "VuoComposition.hh"
#include "VuoCompositionMetadata.hh"
#include "VuoException.hh"
#include "VuoRendererComposition.hh"
#include "VuoRendererFonts.hh"
#include "VuoEditor.hh"
#include "VuoEditorComposition.hh"
#include "VuoEditorUtilities.hh"
#include "VuoEditorWindow.hh"
#include "VuoNodeClass.hh"
#include "VuoNodeClassList.hh"
#include "VuoNodeSet.hh"
#include "VuoStringUtilities.hh"
#include "VuoCompilerMakeListNodeClass.hh"

const int VuoNodePopover::defaultPopoverTextWidth = 192;
const int VuoNodePopover::margin = 8;

/**
 * Creates a new popover widget to display information about a node class.
 */
VuoNodePopover::VuoNodePopover(VuoNodeClass *nodeClass, VuoCompiler *compiler, QWidget *parent) :
	VuoPanelDocumentation(parent)
{
	this->node = NULL;
	this->nodeClass = nodeClass;
	this->displayModelNode = !VuoCompilerMakeListNodeClass::isMakeListNodeClassName(nodeClass->getClassName());
	this->modelNode = NULL;
	this->compiler = compiler;
	initialize();
}

/**
 * Helper function for VuoNodePopover constructors.
 */
void VuoNodePopover::initialize()
{
	VuoEditor *editor = (VuoEditor *)qApp;
	bool isDark = editor->isInterfaceDark();

#if VUO_PRO
	initialize_Pro();
#endif

	this->popoverHasBeenShown = false;

	if (nodeClass->hasCompiler())
	{
		VuoCompilerSpecializedNodeClass *specialized = dynamic_cast<VuoCompilerSpecializedNodeClass *>(nodeClass->getCompiler());
		this->nodeSet = (specialized ? specialized->getOriginalGenericNodeSet() : nodeClass->getNodeSet());
	}
	else
		this->nodeSet = NULL;

	// Header content
	this->headerLabel = new QLabel("", this);
	headerLabel->setText(generateNodePopoverTextHeader());
	headerLabel->setTextInteractionFlags(headerLabel->textInteractionFlags() | Qt::TextSelectableByMouse);

	// Text content
	this->textLabel = new QLabel("", this);
	textLabel->setText(generateNodePopoverText(isDark));
	textLabel->setTextInteractionFlags(textLabel->textInteractionFlags() | Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
	textLabel->setOpenExternalLinks(false);
	connect(textLabel, &QLabel::linkActivated, static_cast<VuoEditor *>(qApp), &VuoEditor::openUrl);

	QTextDocument *doc = textLabel->findChild<QTextDocument *>();
	doc->setIndentWidth(20);

	// Model node
	if (displayModelNode)
	{
		VuoRendererComposition *composition = new VuoRendererComposition(new VuoComposition());
		VuoNode *baseNode = (nodeClass->hasCompiler()? nodeClass->getCompiler()->newNode() :
													   nodeClass->newNode());
		this->modelNode = new VuoRendererNode(baseNode, NULL);
		modelNode->setAlwaysDisplayPortNames(true);
		composition->addNode(modelNode->getBase());

		this->modelNodeView = new QGraphicsView(this);
		modelNodeView->setBackgroundBrush(Qt::transparent);
		modelNodeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		modelNodeView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		modelNodeView->setScene(composition);
		modelNodeView->setRenderHints(QPainter::Antialiasing|QPainter::SmoothPixmapTransform);
		modelNodeView->setFrameShape(QFrame::NoFrame);
		modelNodeView->ensureVisible(modelNode->boundingRect());
		modelNodeView->setEnabled(false);
	}

	// Layout
	layout = new QVBoxLayout(this);
	layout->setContentsMargins(margin, margin, margin, margin);

	layout->addWidget(headerLabel, 0, Qt::AlignTop);
	layout->setStretch(0, 0);

	if (displayModelNode)
	{
		layout->addWidget(modelNodeView, 0, Qt::AlignTop|Qt::AlignLeft);
		layout->setStretch(1, 0);
	}

	layout->addWidget(textLabel, 0, Qt::AlignTop);
	layout->setStretch(2, 1);

	setLayout(layout);

	// Style
	setStyle();


	connect(editor, &VuoEditor::darkInterfaceToggled, this, &VuoNodePopover::updateColor);
	updateColor(isDark);
}

/**
 * Temporarily moved the destructor's disconnect call here.
 * https://b33p.net/kosada/node/13263
 * https://b33p.net/kosada/node/15784
 */
void VuoNodePopover::cleanup()
{
	delete modelNode;
	modelNode = nullptr;
	disconnect(static_cast<VuoEditor *>(qApp), &VuoEditor::darkInterfaceToggled, this, &VuoNodePopover::updateColor);
}

/**
 * Destroys a popover.
 */
VuoNodePopover::~VuoNodePopover()
{
	// See `cleanup`.
}

/**
 * Sets the popover's style attributes.
 */
void VuoNodePopover::setStyle()
{
	textLabel->setFont(VuoRendererFonts::getSharedFonts()->portPopoverFont());
	textLabel->setMargin(0);
	textLabel->setFixedWidth(defaultPopoverTextWidth);
	textLabel->setWordWrap(true);

	headerLabel->setFont(VuoRendererFonts::getSharedFonts()->portPopoverFont());
	headerLabel->setMargin(0);
	headerLabel->setFixedWidth(defaultPopoverTextWidth);
	headerLabel->setWordWrap(true);
	headerLabel->setOpenExternalLinks(true);

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
	headerLabel->setSizePolicy(sizePolicy);

	if (displayModelNode)
	{
		modelNodeView->setSizePolicy(sizePolicy);

		//if (modelNodeView->scene()->width() > getTextWidth())
		//	setTextWidth(modelNodeView->scene()->width());
	}

	setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
}

/**
 * Generates a text header for the popover's associated node.
 */
QString VuoNodePopover::generateNodePopoverTextHeader()
{
	QString nodeClassTitle = nodeClass->getDefaultTitle().c_str();

	QString nodeSetName = (nodeSet? nodeSet->getName().c_str() : "");
	QString nodeClassBreadcrumbsBar = QString("<h3>")
									  .append((!nodeSet)?
												  nodeClassTitle :
												  QString("<a href=\"").
												  append(VuoEditor::vuoNodeSetDocumentationScheme).
												  append("://").
												  append(nodeSetName).
												  append("\">").
												  append(VuoEditorComposition::formatNodeSetNameForDisplay(nodeSetName)).
												  append("</a>").
												  append(" › ").
												  append(nodeClassTitle))
									  .append("</h3>");

	return generateTextStyleString().append(nodeClassBreadcrumbsBar);
}

/**
 * Generates a descriptive text string containing information about the popover's associated node.
 */
QString VuoNodePopover::generateNodePopoverText(bool isDark)
{
	QString nodeClassDescription = generateNodeClassDescription(isDark ? "#606060" : "#b0b0b0");

	// QLabel expects strings to be canonically composed,
	// else it renders diacritics next to (instead of superimposed upon) their base glyphs.
	nodeClassDescription = nodeClassDescription.normalized(QString::NormalizationForm_C);

	// Example compositions
	vector<string> exampleCompositions = nodeClass->getExampleCompositionFileNames();
	QString exampleCompositionLinks = "";

	foreach (string compositionFilePath, exampleCompositions)
	{
		string formattedCompositionName = "";
		string compositionAsString = "";

		string genericNodeClassName;
		if (nodeClass->hasCompiler() && dynamic_cast<VuoCompilerSpecializedNodeClass *>(nodeClass->getCompiler()))
			genericNodeClassName = dynamic_cast<VuoCompilerSpecializedNodeClass *>(nodeClass->getCompiler())->getOriginalGenericNodeClassName();
		else
			genericNodeClassName = nodeClass->getClassName();

		exampleCompositionLinks.append("<li>")
				.append("<a href=\"");

		// Case: compositionFilePath is of form "ExampleCompositionName.vuo"
		// Example composition belongs to this node's own node set.
		if (!QString(compositionFilePath.c_str()).startsWith(VuoEditor::vuoExampleCompositionScheme))
		{
			formattedCompositionName = VuoEditorComposition::formatCompositionFileNameForDisplay(compositionFilePath.c_str()).toUtf8().constData();

			if (nodeSet)
			{
				QString nodeSetName = nodeSet->getName().c_str();
				compositionAsString = nodeSet->getExampleCompositionContents(compositionFilePath);

				exampleCompositionLinks
						.append(VuoEditor::vuoExampleCompositionScheme)
						.append("://")
						.append(nodeSetName)
						.append("/");
			}
		}

		// Case: compositionFilePath is of form "vuo-example://nodeSet/ExampleCompositionName.vuo"
		// Example composition belongs to some other node set.
		else
		{
			QUrl exampleUrl(compositionFilePath.c_str());
			string exampleNodeSetName = exampleUrl.host().toUtf8().constData();
			string exampleFileName = VuoStringUtilities::substrAfter(exampleUrl.path().toUtf8().constData(), "/");
			formattedCompositionName = VuoEditorComposition::formatCompositionFileNameForDisplay(exampleFileName.c_str()).toUtf8().constData();

			VuoNodeSet *exampleNodeSet = (compiler? compiler->getNodeSetForName(exampleNodeSetName) : NULL);
			if (exampleNodeSet)
				compositionAsString = exampleNodeSet->getExampleCompositionContents(exampleFileName);
		}

		// Extract description and formatted name (if present) from composition header.
		VuoCompositionMetadata metadata(compositionAsString);

		string customizedName = metadata.getCustomizedName();
		if (! customizedName.empty())
			formattedCompositionName = customizedName;

		string description = metadata.getDescription();
		string filteredDescription = VuoEditor::removeVuoLinks(description);
		QString compositionDescription = VuoStringUtilities::generateHtmlFromMarkdownLine(filteredDescription).c_str();

		exampleCompositionLinks
				.append(compositionFilePath.c_str())
				.append("?")
				.append(VuoEditor::vuoExampleHighlightedNodeClassQueryItem)
				.append("=")
				.append(genericNodeClassName.c_str())
				.append("\">")
				.append(formattedCompositionName.c_str())
				.append("</a>");

		if (!compositionDescription.isEmpty())
			exampleCompositionLinks.append(": ").append(compositionDescription);

		exampleCompositionLinks.append("</li>");
	}

	if (exampleCompositions.size() > 0)
		//: Appears in the node documentation panel at the bottom of the node library.
		exampleCompositionLinks = "<BR><font size=+1><b>" + tr("Example composition(s)", "", exampleCompositions.size()) + ":</b></font><ul>" + exampleCompositionLinks + "</ul>";

	QString proNodeIndicator;
#if VUO_PRO
	if (nodeClass->isPro())
		proNodeIndicator = (nodeClass->hasCompiler() ? installedProNodeText : missingProNodeText);
#endif

	// Deprecated node indicator
	//: Appears in the node documentation panel at the bottom of the node library.
	QString deprecatedNodeIndicator = nodeClass->getDeprecated()
		? "<p><b>" + tr("This node is deprecated.") + "</b></p>"
		: "";

	return generateTextStyleString()
			.append(nodeClassDescription)
			.append(exampleCompositionLinks)
			.append(proNodeIndicator)
			.append(deprecatedNodeIndicator);
}

/**
 * Generates the section of the popover text with the node class metadata and documentation.
 */
QString VuoNodePopover::generateNodeClassDescription(string smallTextColor)
{
	string className = "";
	string description = "";
	string version = "";

	if (nodeClass->hasCompiler())
	{
		VuoCompilerSpecializedNodeClass *specialized = (nodeClass->hasCompiler() ?
															dynamic_cast<VuoCompilerSpecializedNodeClass *>(nodeClass->getCompiler()) :
															NULL);
		if (specialized)
		{
			className = specialized->getOriginalGenericNodeClassName();
			description = specialized->getOriginalGenericNodeClassDescription();
		}
		else
		{
			className = nodeClass->getClassName();
			description = nodeClass->getDescription();
		}

		version = nodeClass->getVersion();
	}
	else
	{
		className = nodeClass->getClassName();
		description = nodeClass->getDescription();
		if (description.empty())
			//: Appears in the node documentation panel at the bottom of the node library.
			description = tr("This node is not installed. "
							 "You can either remove it from the composition or install it on your computer.").toUtf8().data();
	}

	QString editLink = "";
	QString enclosingFolderLink = "";
	QString editLabel, sourcePath;
	bool nodeClassIsEditable = VuoEditorUtilities::isNodeClassEditable(nodeClass, editLabel, sourcePath);
	bool nodeClassIs3rdParty = nodeClass->hasCompiler() && !nodeClass->getCompiler()->isBuiltIn();

	if (nodeClassIsEditable)
		editLink = QString("<a href=\"file://%1\">%2</a>").arg(sourcePath).arg(editLabel);

	if (nodeClassIsEditable || nodeClassIs3rdParty)
	{
		QString modulePath = (nodeClassIsEditable? sourcePath : nodeClass->getCompiler()->getModulePath().c_str());
		if (!modulePath.isEmpty())
			enclosingFolderLink = QString("<a href=\"file://%1\">%2</a>")
								  .arg(QFileInfo(modulePath).dir().absolutePath())
								  //: Appears in the node documentation panel at the bottom of the node library.
								  .arg(tr("Show in Finder"));
	}

	QString formattedDescription = ((description.empty() || (description == VuoRendererComposition::deprecatedDefaultDescription))?
										"" : VuoStringUtilities::generateHtmlFromMarkdown(description).c_str());

	QString formattedVersion = version.empty()
		? ""
		//: Appears in the node documentation panel at the bottom of the node library.
		: tr("Version %1").arg(QString::fromStdString(version));

	QString tooltipBody;
	tooltipBody.append(QString("<font color=\"%2\">%1</font>")
					   .arg(formattedDescription)
					   .arg(smallTextColor.c_str()));

	tooltipBody.append("<p>");

	tooltipBody.append(QString("<font color=\"%2\">%1</font>")
					   .arg(className.c_str())
					   .arg(smallTextColor.c_str()));

	if (!formattedVersion.isEmpty())
		tooltipBody.append(QString("<br><font color=\"%2\">%1</font>")
						   .arg(formattedVersion)
						   .arg(smallTextColor.c_str()));

	if (!editLink.isEmpty() || !enclosingFolderLink.isEmpty())
		tooltipBody.append("<br>");

	if (!editLink.isEmpty())
		tooltipBody.append(QString("<br><font color=\"%2\">%1</font>")
						   .arg(editLink)
						   .arg(smallTextColor.c_str()));

	if (!enclosingFolderLink.isEmpty())
		tooltipBody.append(QString("<br><font color=\"%2\">%1</font>")
						   .arg(enclosingFolderLink)
						   .arg(smallTextColor.c_str()));

	tooltipBody.append("</p>");

	return tooltipBody;
}

/**
  * Returns the popover's fixed text width.
  */
int VuoNodePopover::getTextWidth()
{
	return textLabel->maximumWidth(); // maximumWidth() = minimumWidth() = fixedWidth
}

/**
  * Returns the currently selected text within this popover's text field,
  * or the empty string of not applicable.
  */
QString VuoNodePopover::getSelectedText()
{
	if (textLabel->hasSelectedText())
		return textLabel->selectedText();
	else if (headerLabel->hasSelectedText())
		return headerLabel->selectedText();
	else
		return QString("");
}

/**
  * Sets the fixed text width of the popover to the provided
  * @c width minus `2 * VuoNodePopover::margin`.
  */
void VuoNodePopover::setTextWidth(int width)
{
	int adjustedWidth = width-2*margin;
	if (adjustedWidth != textLabel->width())
	{
		textLabel->setFixedWidth(adjustedWidth);
		headerLabel->setFixedWidth(adjustedWidth);

		// The following setWordWrap() call is for some reason necessary
		// to ensure that the popover does not end up with empty
		// vertical space when resized. (Word wrap remains on regardless.)
		textLabel->setWordWrap(true);
		headerLabel->setWordWrap(true);
	}
}

/**
 * Displays the popover, extracting any necessary resources (e.g., images) first.
 */
void VuoNodePopover::prepareResourcesAndShow()
{
	dispatch_async(((VuoEditor *)qApp)->getDocumentationQueue(), ^{
		string tmpSaveDir = "";

		// Extract documentation resources (e.g., images)
		if (nodeSet)
		{
			try
			{
				string preexistingNodeSetResourceDir = ((VuoEditor *)qApp)->getResourceDirectoryForNodeSet(nodeSet->getName());
				tmpSaveDir = (!preexistingNodeSetResourceDir.empty()? preexistingNodeSetResourceDir : VuoFileUtilities::makeTmpDir(nodeSet->getName()));

				if (tmpSaveDir != preexistingNodeSetResourceDir)
					nodeSet->extractDocumentationResources(tmpSaveDir);
			}
			catch (VuoException &e)
			{
				VUserLog("%s", e.what());
			}
		}

		emit popoverDisplayRequested(static_cast<QWidget *>(this), tmpSaveDir.c_str());
	});
}

/**
 * Returns a boolean indicating whether this popover description contains an embedded image.
 */
bool VuoNodePopover::containsImage()
{
	QRegExp embeddedImageHtml("<img\\s+", Qt::CaseInsensitive);
	return textLabel->text().contains(embeddedImageHtml);
}

/**
  * Returns the node class associated with this popover.
  */
VuoNodeClass * VuoNodePopover::getNodeClass()
{
	return nodeClass;
}

/**
 * Returns the model renderer node associated with this popover.
 */
VuoRendererNode * VuoNodePopover::getModelNode()
{
	return modelNode;
}

/**
 * Returns the stylesheet for text areas.
 */
QString VuoNodePopover::generateTextStyleString(void)
{
	VuoEditor *editor = (VuoEditor *)qApp;
	bool isDark = editor->isInterfaceDark();
	return VuoEditor::generateHtmlDocumentationStyles(false, isDark) + VUO_QSTRINGIFY(
				<style>
				* {
					font-size: 13px;
					color: %1;
				}
				span {
					font-size: 11px;
					color: %1;
				}
				a {
					color: %2;
				}
				p {
					color: %3;
				}

				// Reduce space between nested lists.
				// https://b33p.net/kosada/node/11442#comment-54820
				ul {
					margin-bottom: 0px;
				}
				</style>)
			.arg(isDark ? "#a0a0a0" : "#606060")
			.arg(isDark ? "#6882be" : "#74acec")
			.arg(isDark ? "#909090" : "#606060")
			;
}

/**
 * Regenerates the text to use the selected light/dark style.
 */
void VuoNodePopover::updateColor(bool isDark)
{
	headerLabel->setText(generateNodePopoverTextHeader());
	textLabel->setText(generateNodePopoverText(isDark));

	QPalette p;
	QColor bulletColor = isDark ? "#a0a0a0" : "#606060";
	p.setColor(QPalette::Normal,   QPalette::Text, bulletColor);
	p.setColor(QPalette::Inactive, QPalette::Text, bulletColor);
	textLabel->setPalette(p);
}

/**
 * Handle mouse move events.
 */
void VuoNodePopover::mouseMoveEvent(QMouseEvent *event)
{
	// Detect whether we should initiate a drag.
	if (event->buttons() & Qt::LeftButton)
	{
			// Display the model node during the drag.
			QRectF r = modelNode->boundingRect().toRect();
			qreal dpRatio = devicePixelRatio();
			QPixmap pixmap(dpRatio*r.width(), dpRatio*r.height());
			pixmap.setDevicePixelRatio(dpRatio);
			pixmap.fill(Qt::transparent);

			QPainter painter(&pixmap);
			painter.setRenderHint(QPainter::Antialiasing, true);
			painter.setRenderHint(QPainter::HighQualityAntialiasing, true);
			painter.setRenderHint(QPainter::TextAntialiasing, true);
			painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

			painter.translate(-r.topLeft());
			modelNode->paint(&painter, NULL, NULL);
			painter.end();

			// Execute the drag.
			QDrag *drag = new QDrag(this);
			drag->setPixmap(pixmap);
			drag->setHotSpot(event->pos()-(modelNodeView->pos()
										   +QPoint(38,0) // magic number that simulates dragging the node
										   ));			 // from the cursor position for most nodes

			// Include the node class name and the hotspot in the dragged data.
			QMimeData *mimeData = new QMimeData();
			const QString dropText = QString(nodeClass->getClassName().c_str())
						   .append(QLatin1Char('\n'))
						   .append(QString::number(drag->hotSpot().x()))
						   .append(QLatin1Char('\n'))
						   .append(QString::number(drag->hotSpot().y()));
			mimeData->setData("text/plain", dropText.toUtf8().constData());
			drag->setMimeData(mimeData);
			drag->exec(Qt::CopyAction);
	}
}

/**
 * Handle mouse double-click events.
 */
void VuoNodePopover::mouseDoubleClickEvent(QMouseEvent *event)
{
	VuoEditorWindow *targetWindow = VuoEditorWindow::getMostRecentActiveEditorWindow();
	if (!targetWindow || !targetWindow->getComposition())
		return;

	QPointF startPos = targetWindow->getFittedScenePos(targetWindow->getCursorScenePos()-
													   QPointF(0,VuoRendererNode::nodeHeaderYOffset));

	QList<QGraphicsItem *> newNodes;
	VuoRendererNode *newNode = targetWindow->getComposition()->createNode(nodeClass->getClassName().c_str(), "", startPos.x(), startPos.y());
	newNodes.append((QGraphicsItem *)newNode);

	targetWindow->componentsAdded(newNodes, targetWindow->getComposition());
}

/**
 * Handle context menu events.
 */
void VuoNodePopover::contextMenuEvent(QContextMenuEvent *event)
{
	QMenu *contextMenu = new QMenu(this);
	contextMenu->setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133
	VuoNodeClassList::populateContextMenuForNodeClass(contextMenu, nodeClass->getCompiler());
	contextMenu->exec(event->globalPos());
}
