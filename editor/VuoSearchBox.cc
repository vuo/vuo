/**
 * @file
 * VuoSearchBox implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoSearchBox.hh"
#include "ui_VuoSearchBox.h"

#include "VuoCompilerSpecializedNodeClass.hh"
#include "VuoComment.hh"
#include "VuoComposition.hh"
#include "VuoRendererComment.hh"
#include "VuoEditor.hh"
#include "VuoEditorComposition.hh"
#include "VuoEditorUtilities.hh"
#include "VuoDialogForInputEditor.hh"
#include "VuoNodeClass.hh"

extern "C" {
#include "VuoTextHtml.h"
}

/**
 * Creates an instance of a search dialogue.
 */
VuoSearchBox::VuoSearchBox(VuoEditorComposition *composition, QWidget *parent, Qt::WindowFlags flags) :
	QDockWidget(parent, flags),
	ui(new Ui::VuoSearchBox)
{
	this->composition = composition;

	ui->setupUi(this);
	setWindowTitle(tr("Find"));
	setAllowedAreas(Qt::TopDockWidgetArea);
	setFloating(false);

	ui->searchText->installEventFilter(this);

	ui->doneButton->setDefault(false);
	ui->doneButton->setAutoDefault(false);

	connect(ui->doneButton, &QPushButton::clicked, this, &QDockWidget::close);
	connect(ui->previousButton, &QPushButton::clicked, this, &VuoSearchBox::goToPreviousResult);
	connect(ui->nextButton, &QPushButton::clicked, this, &VuoSearchBox::goToNextResult);
	connect(ui->searchText, &QLineEdit::textChanged, this, &VuoSearchBox::searchForText);

	setTitleBarWidget(new QWidget()); // Disable the titlebar.

	currentResultIndex = 0;
	noResultsText = QApplication::translate("VuoSearchBox", "No results");
	resultCount = new QLabel(this);
	int resultCountTextWidth = qMax(70, QFontMetrics(resultCount->font()).size(0,noResultsText).width());

	resultCount->setFixedSize(resultCountTextWidth, resultCount->height());
	resultCount->setStyleSheet("QLabel { color : gray; }");
	resultCount->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

	searchIcon = QIcon(QApplication::applicationDirPath().append("/../Resources/search-loupe.png"));
	searchButton = new QToolButton(ui->searchText);
	searchButton->setIcon(searchIcon);
	searchButton->setCursor(Qt::ArrowCursor);
	searchButton->setStyleSheet("QToolButton { border: none; padding: 2px 3px 3px 3px; }");

	const int leftPadding = searchButton->iconSize().width()+4;
	setStyleSheet(VUO_QSTRINGIFY(
		QLineEdit {
			padding-left: %2px;	 // Space for search button
			padding-right: %1px; // Space for result count
		}
		QLineEdit:focus {
			padding-left: %2px;	 // Space for search button
			padding-right: %1px; // Space for result count
		}
	)
	.arg(resultCount->width()+10)
	.arg(leftPadding)
	);

	VuoEditor *editor = (VuoEditor *)qApp;
	connect(editor, &VuoEditor::darkInterfaceToggled, this, &VuoSearchBox::updateColor);
	updateColor(editor->isInterfaceDark());
}

/**
 * Displays the search dialog.
 */
void VuoSearchBox::show()
{
	ui->searchText->setFocus();
	ui->searchText->selectAll();
	searchForText(ui->searchText->text());
	QDockWidget::show();
	repositionChildWidgets();

	setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
	setMinimumHeight(geometry().height());
	setMaximumHeight(geometry().height());
}

/**
 * Updates the spotlighted items to reflect the current search results.
 */
void VuoSearchBox::updateSpotlightedItems()
{
	// Don't interfere with node execution spotlighting while in "Show Events" mode.
	if (composition->getShowEventsMode())
		return;

	if (ui->searchText->text().isEmpty())
	{
		if (composition->getRenderNodeActivity())
			composition->stopDisplayingActivity();
	}
	else
	{
		if (!composition->getRenderNodeActivity())
			composition->beginDisplayingActivity(false);

		foreach (VuoNode *node, composition->getBase()->getNodes())
		{
			if (node->getRenderer()->getTimeLastExecutionEnded() == VuoRendererItem::activityInProgress)
				node->getRenderer()->setExecutionEnded();
		}

		foreach (QGraphicsItem *matchingItem, searchResults)
		{
			VuoRendererNode *matchingNode = dynamic_cast<VuoRendererNode *>(matchingItem);
			if (matchingNode)
				matchingNode->setExecutionBegun();
		}

		// @todo https://b33p.net/kosada/node/9986 : Implement fade for comments?
	}
}

/**
 * Searches the composition for the provided text.
 */
void VuoSearchBox::searchForText(const QString &searchText)
{
	searchResults.clear();
	currentResultIndex = 0;

	if (searchText.isEmpty())
	{
		composition->clearSelection();
		updateSpotlightedItems();
		updateResultCount();
		return;
	}

	searchResults = getCurrentSearchResults(searchText);

	composition->clearSelection();
	if (searchResults.size() >= 1)
		searchResults[currentResultIndex]->setSelected(true);

	updateSpotlightedItems();
	updateResultCount();
	updateViewportToFitResults();
	emit searchPerformed();
}

/**
 * Searches nodes and comments in the composition for the provided @c searchText.
 * Returns the matching items.
 */
vector<QGraphicsItem *> VuoSearchBox::getCurrentSearchResults(const QString &searchText)
{
	vector<QGraphicsItem *> searchResults;

	// Special handling for the "deprecated" search keyword: return all deprecated nodes in the composition.
	if (searchText.toLower().trimmed() == "deprecated")
		return findDeprecatedNodes();

	// Special handling for the "subcomposition" and ".vuo" search keywords: return all subcompositions.
	if (searchText.toLower().trimmed() == "subcomposition" || searchText.toLower().trimmed() == ".vuo"
			|| searchText.toLower().trimmed() == "source:.vuo")
		return findSubcompositionNodes();

	// Special handling for the ".fs" search keyword: return all shaders.
	if (searchText.toLower().trimmed() == ".fs" || searchText.toLower().trimmed() == "source:.fs")
		return findShaderNodes();

	// Special handling for the ".c" search keyword: return all C-language nodes.
	if (searchText.toLower().trimmed() == ".c" || searchText.toLower().trimmed() == "source:.c")
		return findCLanguageNodes();

	// Special handling for the ".vuonode" search keyword: return all pre-compiled 3rd-party nodes.
	if (searchText.toLower().trimmed() == ".vuonode" || searchText.toLower().trimmed() == "source:.vuonode")
		return find3rdPartyPrecompiledNodes();

	// Search nodes.
	foreach (VuoNode *node, composition->getBase()->getNodes())
	{
		// Retrieve the node class name, as rendered on the canvas.
		VuoNodeClass *nodeClass = node->getNodeClass();
		QString nodeClassName = "";
		if (nodeClass->hasCompiler() && dynamic_cast<VuoCompilerSpecializedNodeClass *>(nodeClass->getCompiler()))
			nodeClassName = QString::fromUtf8(dynamic_cast<VuoCompilerSpecializedNodeClass *>(nodeClass->getCompiler())->getOriginalGenericNodeClassName().c_str());
		else
			nodeClassName = QString::fromUtf8(nodeClass->getClassName().c_str());

		// Check whether we're dealing with a collapsed typecast.
		bool isCollapsedTypecast = false;
		vector<VuoPort *> inputPorts = node->getInputPorts();
		VuoPort *typecastInPort = (inputPorts.size() >= VuoNodeClass::unreservedInputPortStartIndex+1?
										node->getInputPorts()[VuoNodeClass::unreservedInputPortStartIndex] :
										NULL);

		if (typecastInPort && typecastInPort->hasRenderer())
		{
			VuoRendererPort *typecastParent = typecastInPort->getRenderer()->getTypecastParentPort();
			if (typecastParent)
				isCollapsedTypecast = true;
		}

		// Check whether we're dealing with another type of attachment.
		bool isAttachment = false;
		if (dynamic_cast<VuoRendererInputAttachment *>(node->getRenderer()))
			isAttachment = true;

		bool nodeMatched = false;

		// Match against node title.
		if (!isCollapsedTypecast && !isAttachment)
		{
			// Only match starting at the beginning of words within the title.
			int index = QString(node->getTitle().c_str()).toLower().indexOf(searchText.toLower());
			if (index == 0 || ((index > 0) && (QString(node->getTitle().c_str()).toLower().at(index-1) == ' ')))
			{
				searchResults.push_back(node->getRenderer());
				nodeMatched = true;
			}
		}

		// Match against node class name.
		if (!nodeMatched && !isCollapsedTypecast && !isAttachment)
		{
			// Only match starting at the beginning of '.'-delimited segments within the class name.
			int index = nodeClassName.toLower().indexOf(searchText.toLower());
			if (index == 0 || ((index > 0) && (nodeClassName.toLower().at(index-1) == '.')))
			{
				searchResults.push_back(node->getRenderer());
				nodeMatched = true;
			}
		}

		// Match against port display name.
		if (!nodeMatched)
		{
			foreach (VuoPort *port, node->getInputPorts())
			{
				if (nodeMatched)
					break;

				if (node->getRefreshPort() == port)
					continue;

				if (port && port->hasRenderer())
				{
					// Only match starting at the beginning of words within the display name.
					int index = QString(port->getRenderer()->getPortNameToRenderWhenDisplayed().c_str()).toLower().indexOf(searchText.toLower());
					if (index == 0 || ((index > 0) && (QString(port->getRenderer()->getPortNameToRenderWhenDisplayed().c_str()).toLower().at(index-1) == ' ')))
					{
						if (isCollapsedTypecast && typecastInPort->getRenderer()->getTypecastParentPort()->getRenderedParentNode())
							searchResults.push_back(typecastInPort->getRenderer()->getTypecastParentPort()->getRenderedParentNode());
						else
							searchResults.push_back(node->getRenderer());
						nodeMatched = true;
					}
				}
			}

			foreach (VuoPort *port, node->getOutputPorts())
			{
				if (nodeMatched)
					break;

				if (port && port->hasRenderer())
				{
					// Only match starting at the beginning of words within the display name.
					int index = QString(port->getRenderer()->getPortNameToRenderWhenDisplayed().c_str()).toLower().indexOf(searchText.toLower());
					if (index == 0 || ((index > 0) && (QString(port->getRenderer()->getPortNameToRenderWhenDisplayed().c_str()).toLower().at(index-1) == ' ')))
					{
						if (isCollapsedTypecast && typecastInPort->getRenderer()->getTypecastParentPort()->getRenderedParentNode())
							searchResults.push_back(typecastInPort->getRenderer()->getTypecastParentPort()->getRenderedParentNode());
						else
							searchResults.push_back(node->getRenderer());
						nodeMatched = true;
					}
				}
			}
		}

		// Match against port constants.
		if (!nodeMatched)
		{
			foreach (VuoPort *port, node->getInputPorts())
			{
				if (nodeMatched)
					break;

				if (port && port->hasRenderer())
				{
					if (port->getRenderer()->isConstant() &&
						 QString(VuoText_removeHtml(port->getRenderer()->getConstantAsStringToRender().c_str())).contains(searchText, Qt::CaseInsensitive))
					{
						if (isCollapsedTypecast && typecastInPort->getRenderer()->getTypecastParentPort()->getRenderedParentNode())
							searchResults.push_back(typecastInPort->getRenderer()->getTypecastParentPort()->getRenderedParentNode());
						else
							searchResults.push_back(node->getRenderer());
						nodeMatched = true;
					}
				}
			}
		}
	}

	// Now search comments.
	foreach (VuoComment *comment, composition->getBase()->getComments())
	{
		// Only match starting at the beginning of words within the comment text.
		int index = QString(comment->getContent().c_str()).toLower().indexOf(searchText.toLower());
		if (index == 0 || ((index > 0) && !(QString(comment->getContent().c_str()).toLower().at(index-1).isLetterOrNumber())))
			searchResults.push_back(comment->getRenderer());
	}

	sort(searchResults.begin(), searchResults.end(), itemLessThan);
	searchResults.erase(std::unique(searchResults.begin(), searchResults.end()), searchResults.end());

	return searchResults;
}

/**
 * Returns deprecated nodes within the composition.
 */
vector<QGraphicsItem *> VuoSearchBox::findDeprecatedNodes()
{
	// Search nodes.
	foreach (VuoNode *node, composition->getBase()->getNodes())
	{
		bool nodeIsDeprecated = node->getNodeClass()->getDeprecated();
		if (nodeIsDeprecated && !excludeNodeFromSearchResults(node))
			searchResults.push_back(node->getRenderer());
	}

	sort(searchResults.begin(), searchResults.end(), itemLessThan);
	searchResults.erase(std::unique(searchResults.begin(), searchResults.end()), searchResults.end());

	return searchResults;
}

/**
 * Returns subcomposition nodes within the composition.
 */
vector<QGraphicsItem *> VuoSearchBox::findSubcompositionNodes()
{
	// Search nodes.
	foreach (VuoNode *node, composition->getBase()->getNodes())
	{

		bool nodeIsSubcomposition = node->getNodeClass()->hasCompiler()?
										node->getNodeClass()->getCompiler()->isSubcomposition() :
										false;
		if (nodeIsSubcomposition && !excludeNodeFromSearchResults(node))
			searchResults.push_back(node->getRenderer());
	}

	sort(searchResults.begin(), searchResults.end(), itemLessThan);
	searchResults.erase(std::unique(searchResults.begin(), searchResults.end()), searchResults.end());

	return searchResults;
}

/**
 * Returns shader nodes within the composition.
 */
vector<QGraphicsItem *> VuoSearchBox::findShaderNodes()
{
	// Search nodes.
	foreach (VuoNode *node, composition->getBase()->getNodes())
	{

		bool nodeIsShader = node->getNodeClass()->hasCompiler()?
										node->getNodeClass()->getCompiler()->isIsf() :
										false;
		if (nodeIsShader && !excludeNodeFromSearchResults(node))
			searchResults.push_back(node->getRenderer());
	}

	sort(searchResults.begin(), searchResults.end(), itemLessThan);
	searchResults.erase(std::unique(searchResults.begin(), searchResults.end()), searchResults.end());

	return searchResults;
}

/**
 * Returns C-language nodes within the composition.
 */
vector<QGraphicsItem *> VuoSearchBox::findCLanguageNodes()
{
	// Search nodes.
	foreach (VuoNode *node, composition->getBase()->getNodes())
	{
		VuoNodeClass *nodeClass = node->getNodeClass();
		QString actionText, sourcePath;
		bool nodeIsEditable = VuoEditorUtilities::isNodeClassEditable(nodeClass, actionText, sourcePath);
		bool nodeHasExternalSource = nodeIsEditable && !nodeClass->getCompiler()->isSubcomposition() && !nodeClass->getCompiler()->isIsf();

		if (nodeHasExternalSource && !excludeNodeFromSearchResults(node))
			searchResults.push_back(node->getRenderer());
	}

	sort(searchResults.begin(), searchResults.end(), itemLessThan);
	searchResults.erase(std::unique(searchResults.begin(), searchResults.end()), searchResults.end());

	return searchResults;
}

/**
 * Returns 3rd party pre-compiled nodes within the composition.
 */
vector<QGraphicsItem *> VuoSearchBox::find3rdPartyPrecompiledNodes()
{
	// Search nodes.
	foreach (VuoNode *node, composition->getBase()->getNodes())
	{
		VuoNodeClass *nodeClass = node->getNodeClass();
		QString actionText, sourcePath;
		bool nodeIs3rdParty = node->getNodeClass()->hasCompiler()?
								 !nodeClass->getCompiler()->isBuiltIn():
								 false;
		bool nodeIsEditable = VuoEditorUtilities::isNodeClassEditable(nodeClass, actionText, sourcePath);

		if (nodeIs3rdParty && !nodeIsEditable && !excludeNodeFromSearchResults(node))
			searchResults.push_back(node->getRenderer());
	}

	sort(searchResults.begin(), searchResults.end(), itemLessThan);
	searchResults.erase(std::unique(searchResults.begin(), searchResults.end()), searchResults.end());

	return searchResults;
}

/**
 * Returns a boolean indicating whether the provided @c node should be omitted from search
 * results whether or not it matches the user-entered search terms.
 */
bool VuoSearchBox::excludeNodeFromSearchResults(VuoNode *node)
{
	// Check whether we're dealing with a collapsed typecast.
	bool isCollapsedTypecast = false;
	vector<VuoPort *> inputPorts = node->getInputPorts();
	VuoPort *typecastInPort = (inputPorts.size() >= VuoNodeClass::unreservedInputPortStartIndex+1?
								   node->getInputPorts()[VuoNodeClass::unreservedInputPortStartIndex] :
							   NULL);
	if (typecastInPort && typecastInPort->hasRenderer() && typecastInPort->getRenderer()->getTypecastParentPort())
		isCollapsedTypecast = true;

	// Check whether we're dealing with another type of attachment.
	bool isAttachment = (dynamic_cast<VuoRendererInputAttachment *>(node->getRenderer()));

	return (isCollapsedTypecast || isAttachment);
}

/**
 * Highlights the next search result.
 */
void VuoSearchBox::goToNextResult()
{
	bool startingOver = false;
	vector<QGraphicsItem *> newSearchResults = getCurrentSearchResults(ui->searchText->text());
	if (newSearchResults != searchResults)
	{
		searchResults.clear();
		startingOver = true;
		searchResults = newSearchResults;
	}

	composition->clearSelection();
	updateSpotlightedItems();

	if (searchResults.size() > 0)
	{
		currentResultIndex = (startingOver? 0 : (currentResultIndex + 1) % searchResults.size());
		searchResults[currentResultIndex]->setSelected(true);
	}

	updateResultCount();
	updateViewportToFitResults();
}

/**
 * Highlights the previous search result.
 */
void VuoSearchBox::goToPreviousResult()
{
	bool startingOver = false;
	vector<QGraphicsItem *> newSearchResults = getCurrentSearchResults(ui->searchText->text());
	if (newSearchResults != searchResults)
	{
		searchResults.clear();
		startingOver = true;
		searchResults = newSearchResults;
	}

	composition->clearSelection();
	updateSpotlightedItems();

	if (searchResults.size() > 0)
	{
		currentResultIndex = (startingOver? 0 : currentResultIndex - 1);
		if (currentResultIndex < 0)
			currentResultIndex += searchResults.size();

		searchResults[currentResultIndex]->setSelected(true);
	}

	updateResultCount();
	updateViewportToFitResults();
}

/**
 * If the search box is currently displayed, refreshes the results of the current search.
 */
void VuoSearchBox::refreshResults()
{
	if (isHidden())
		return;

	bool startingOver = false;
	vector<QGraphicsItem *> newSearchResults = getCurrentSearchResults(ui->searchText->text());
	if (newSearchResults != searchResults)
	{
		searchResults.clear();
		startingOver = true;
		searchResults = newSearchResults;
	}

	composition->clearSelection();
	updateSpotlightedItems();

	if (searchResults.size() > 0)
	{
		currentResultIndex = (startingOver? 0 : currentResultIndex);
		searchResults[currentResultIndex]->setSelected(true);
	}

	updateResultCount();
	updateViewportToFitResults();
}


/**
 * If the view doesn't already contain all of the search results, refit it so that it does.
 */
void VuoSearchBox::updateViewportToFitResults()
{
	QRectF itemsTightBoundingRect = (!composition->selectedItems().isEmpty()? composition->internalSelectedItemsChildrenBoundingRect() :
																   composition->internalItemsBoundingRect());

	if (!composition->selectedItems().isEmpty() &&
			!composition->views()[0]->mapToScene(composition->views()[0]->rect()).boundingRect().
			contains(QRect(itemsTightBoundingRect.topLeft().toPoint(),
						   itemsTightBoundingRect.bottomRight().toPoint())))
	{
		composition->views()[0]->ensureVisible(itemsTightBoundingRect);
	}
}

/**
 * Updates the text of the result count string to reflect the current result index and total.
 * Enables or disables the "Previous" and "Next" buttons as appropriate.
 */
void VuoSearchBox::updateResultCount()
{
	if (ui->searchText->text().isEmpty())
		resultCount->setText("");
	else if (searchResults.size() == 0)
		resultCount->setText(noResultsText);
	else
		resultCount->setText(QString("%1 of %2").arg(currentResultIndex+1).arg(searchResults.size()));

	ui->nextButton->setEnabled(searchResults.size() >= 1);
	ui->nextButton->setAutoRaise(!ui->nextButton->isEnabled());
	ui->previousButton->setEnabled(searchResults.size() >= 1);
	ui->previousButton->setAutoRaise(!ui->previousButton->isEnabled());
}

/**
 * Handle keypress events.
 */
bool VuoSearchBox::eventFilter(QObject *object, QEvent *event)
{
	if ((event->type() == QEvent::KeyPress) && (static_cast<QKeyEvent *>(event)->key() == Qt::Key_Return))
		goToNextResult();
	else if ((event->type() == QEvent::KeyPress) && (static_cast<QKeyEvent *>(event)->key() == Qt::Key_G)
			 && (static_cast<QKeyEvent *>(event)->modifiers() & Qt::ControlModifier)
			 && (!(static_cast<QKeyEvent *>(event)->modifiers() & Qt::ShiftModifier)))
		goToNextResult();
	else if ((event->type() == QEvent::KeyPress) && (static_cast<QKeyEvent *>(event)->key() == Qt::Key_G)
			 && (static_cast<QKeyEvent *>(event)->modifiers() & Qt::ControlModifier)
			 && (static_cast<QKeyEvent *>(event)->modifiers() & Qt::ShiftModifier))
		goToPreviousResult();
	else if ((event->type() == QEvent::KeyPress) && (static_cast<QKeyEvent *>(event)->key() == Qt::Key_Escape))
		close();
	else
	{
		object->removeEventFilter(this);
		QApplication::sendEvent(object, event);
		object->installEventFilter(this);
	}

	return true;
}

/**
 * Handle resize events.  When the widget receives this event, it already has its new geometry.
 */
void VuoSearchBox::resizeEvent(QResizeEvent *event)
{
	QDockWidget::resizeEvent(event);
	repositionChildWidgets();
}

/**
 * Handle close events.
 */
void VuoSearchBox::closeEvent(QCloseEvent *event)
{
	if (!composition->getShowEventsMode() && composition->getRenderNodeActivity())
		composition->stopDisplayingActivity();

	QDockWidget::closeEvent(event);
}

/**
 * Moves and resizes child widgets to adapt to the current search box width.
 */
void VuoSearchBox::repositionChildWidgets()
{
	ui->searchText->resize(widget()->geometry().width()-ui->previousButton->geometry().width()
						   -ui->nextButton->geometry().width()
						   -ui->doneButton->geometry().width()-5,
						   ui->searchText->height());
	resultCount->move(ui->searchText->geometry().right()-resultCount->width()-5, ui->searchText->pos().y()-5);
	ui->previousButton->move(ui->searchText->geometry().right()+5, ui->previousButton->y());
	ui->nextButton->move(ui->previousButton->geometry().right()+2, ui->nextButton->y());
	ui->doneButton->move(ui->nextButton->geometry().right(), ui->doneButton->y());
	update();
}

/**
 * Makes the search box dark or light.
 */
void VuoSearchBox::updateColor(bool isDark)
{
	QString barBackgroundColor =			isDark ? "#919191" : "#efefef";
	QString pressedToolButtonBackgroundColor =	isDark ? "#aaaaaa" : "#d4d4d4";
	QString disabledToolButtonArrowColor     =	isDark ? "#707070" : "#c0c0c0";

	QString styleSheet = VuoDialogForInputEditor::getStyleSheet(isDark)
		+ VUO_QSTRINGIFY(
			QWidget#searchBoxContents {
				background-color: %1;
			}
			QToolButton#previousButton,
			QToolButton#nextButton {
				border: 1px;
				background-color: %1;
				border-radius: 4px;
			}
			QToolButton#previousButton:pressed,
			QToolButton#nextButton:pressed {
				background-color: %2;
			}
			QToolButton#previousButton:disabled,
			QToolButton#nextButton:disabled {
				color: %3;
			}
		)
		.arg(barBackgroundColor)
		.arg(pressedToolButtonBackgroundColor)
		.arg(disabledToolButtonArrowColor);

	if (isDark)
	{
		QString focusRingColor = "#1d6ae5";
		styleSheet += VUO_QSTRINGIFY(
			QLineEdit {
				border: 1px solid #919191;
				background: #262626;
				padding-top: 2px;
			}
			QLineEdit:focus {
				border: 2px solid %1;
				padding-top: 1px;
			}

			QPushButton {
				border: none;
				background: #aaaaaa;
				border-radius: 4px;
				margin: 4px 7px 7px 5px;
			}
			QPushButton:pressed {
				background: #cccccc;
			}
			QPushButton:focus {
				border: 2px solid %1;
			}
		)
		.arg(focusRingColor);
	}

	ui->searchBoxContents->setStyleSheet(styleSheet);

	ui->searchText->setAttribute(Qt::WA_MacShowFocusRect, !isDark);
}

/**
 * Comparison function for graphics items, for use in sorting.
 * Returns a boolean indicating whether @c item1 should be considered less than
 * (listed before) @c item2.
 */
bool VuoSearchBox::itemLessThan(QGraphicsItem *item1, QGraphicsItem *item2)
{
	int item1RowNum = item1->scenePos().y()/VuoRendererComposition::majorGridLineSpacing;
	int item2RowNum = item2->scenePos().y()/VuoRendererComposition::majorGridLineSpacing;

	if (item1RowNum < item2RowNum)
		return true;
	else if (item2RowNum < item1RowNum)
		return false;
	return (item1->scenePos().x() < item2->scenePos().x());
}

/**
 * Returns a boolean indicating whether the "Next" and "Previous" buttons are currently enabled.
 */
bool VuoSearchBox::traversalButtonsEnabled()
{
	return (ui->nextButton->isEnabled() && ui->previousButton->isEnabled());
}

VuoSearchBox::~VuoSearchBox()
{
	delete ui;
}
