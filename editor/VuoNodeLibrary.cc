/**
 * @file
 * VuoNodeLibrary implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoNodeLibrary.hh"
#include "ui_VuoNodeLibrary.h"

#include "VuoEditor.hh"
#include "VuoEditorComposition.hh"
#include "VuoEditorUtilities.hh"
#include "VuoEditorWindowToolbar.hh"
#include "VuoGenericType.hh"
#include "VuoCompiler.hh"
#include "VuoCompilerMakeListNodeClass.hh"
#include "VuoCompilerPortClass.hh"
#include "VuoCompilerType.hh"
#include "VuoNodeClass.hh"
#include "VuoNodeClassListItemDelegate.hh"
#include "VuoNodePopover.hh"
#include "VuoStringUtilities.hh"
#include "VuoType.hh"

#ifdef __APPLE__
#include <objc/objc-runtime.h>
#endif

map<pair<QString, QString>, QStringList> VuoNodeLibrary::tokensForNodeClass;
map<string, int> VuoNodeLibrary::nodeClassFrequency;
set<string> VuoNodeLibrary::stopWords;
map<VuoCompilerNodeClass *, int> VuoNodeLibrary::newlyInstalledNodeClasses;

/**
 * Creates a window for browsing node classes.
 */
VuoNodeLibrary::VuoNodeLibrary(VuoCompiler *compiler, QWidget *parent,
							   nodeLibraryDisplayMode displayMode) :
	QDockWidget(parent),
	ui(new Ui::VuoNodeLibrary)
{
	ui->setupUi(this);

	ui->nodeClassList->verticalScrollBar()->setAttribute(Qt::WA_MacSmallSize);
	ui->nodeClassList->horizontalScrollBar()->setAttribute(Qt::WA_MacSmallSize);
	ui->nodePopoverPane->verticalScrollBar()->setAttribute(Qt::WA_MacSmallSize);
	ui->nodePopoverPane->horizontalScrollBar()->setAttribute(Qt::WA_MacSmallSize);

	// When the node library is re-sized vertically, stretch the node class list,
	// not the documentation pane.
	ui->splitter->setStretchFactor(0, 1);
	ui->splitter->setStretchFactor(1, 0);

	this->setFocusProxy(ui->textFilter);
	setTabOrder(ui->textFilter, ui->nodeClassList);
	ui->VuoNodeLibraryContents->setFocusProxy(ui->nodeClassList);
	ui->nodeClassList->setItemDelegate(new VuoNodeClassListItemDelegate(ui->nodeClassList));
	setHumanReadable(displayMode == VuoNodeLibrary::displayByName);

	updateListViewTimer = new QTimer(this);
	updateListViewTimer->setObjectName("VuoNodeLibrary::updateListViewTimer");
	updateListViewTimer->setSingleShot(true);
	connect(updateListViewTimer, &QTimer::timeout, this, &VuoNodeLibrary::updateListViewForNewFilterTextNow);

	connect(ui->nodeClassList, &VuoNodeClassList::itemSelectionChanged, this, static_cast<void (QWidget::*)()>(&QWidget::update));
	connect(ui->nodeClassList, &VuoNodeClassList::componentsAdded, this, &VuoNodeLibrary::componentsAdded);
	connect(ui->nodeClassList, &VuoNodeClassList::nodePopoverRequestedForClass, this, static_cast<void (VuoNodeLibrary::*)(VuoNodeClass *)>(&VuoNodeLibrary::prepareAndDisplayNodePopoverForClass));
	connect(ui->splitter, &QSplitter::splitterMoved, this, &VuoNodeLibrary::emitNodeDocumentationPanelHeightChanged);
	connect(ui->textFilter, &VuoNodeLibraryTextFilter::textChanged, this, &VuoNodeLibrary::updateListViewForNewFilterTextOnTimer);
	connect(ui->textFilter, &VuoNodeLibraryTextFilter::nodeLibraryReceivedPasteCommand, this, &VuoNodeLibrary::nodeLibraryReceivedPasteCommand);

	ui->textFilter->installEventFilter(this);
	this->hasBeenShown = false;
	this->preferredNodeDocumentationPanelHeight = ((VuoEditor *)qApp)->getPreferredNodeDocumentationPanelHeight();
	this->preferredNodeLibraryWidth = ((VuoEditor *)qApp)->getPreferredNodeLibraryWidth();
	this->defaultMinimumWidth = minimumWidth();
	this->defaultMaximumWidth = maximumWidth();
	fixWidth(false);

	this->compiler = compiler;

	populateNodeClassFrequencyMap();
	populateStopWordList();
	updateListViewForNewFilterTextNow();
	updateUI();

	VuoEditor *editor = (VuoEditor *)qApp;
	connect(editor, &VuoEditor::darkInterfaceToggled, this, &VuoNodeLibrary::updateColor);
	updateColor(editor->isInterfaceDark());
#ifdef VUO_PRO
	VuoNodeLibrary_Pro();
#endif
}

/**
 * Removes from @a nodeClasses those that should not be displayed in the node library.
 */
void VuoNodeLibrary::cullHiddenNodeClasses(vector<VuoCompilerNodeClass *> &nodeClasses)
{
	for (int i = nodeClasses.size() - 1; i >= 0; --i)
	{
		VuoCompilerNodeClass *nodeClass = nodeClasses[i];

		if (
				// Exclude specializations of generic node classes from the node library.
				dynamic_cast<VuoCompilerSpecializedNodeClass *>(nodeClass) ||

				/// @todo Remove after https://b33p.net/kosada/node/5300
				(VuoStringUtilities::beginsWith(nodeClass->getBase()->getClassName(), "vuo.dictionary") &&
				 VuoStringUtilities::endsWith(nodeClass->getBase()->getClassName(), "VuoText.VuoReal")) ||

				// Exclude deprecated nodes from the node library.
				nodeClass->getBase()->getDeprecated()
			)
		{
			nodeClasses.erase(nodeClasses.begin() + i);
		}
	}
}

/**
 * Filters events on watched objects.
 */
bool VuoNodeLibrary::eventFilter(QObject *object, QEvent *event)
{
	// Customize handling of keypress events so that the node class list
	// always receives keypresses of the 'Return' and navigation keys.
	if (event->type() == QEvent::KeyPress)
	{
		QKeyEvent *keyEvent = (QKeyEvent *)(event);
		if ((keyEvent->key() == Qt::Key_Up) ||
			(keyEvent->key() == Qt::Key_Down) ||
			(keyEvent->key() == Qt::Key_PageUp) ||
			(keyEvent->key() == Qt::Key_PageDown) ||
			(keyEvent->key() == Qt::Key_Home) ||
			(keyEvent->key() == Qt::Key_End) ||
			(keyEvent->key() == Qt::Key_Return))
		{
			QApplication::sendEvent(ui->nodeClassList, event);
		}

		else
		{
			object->removeEventFilter(this);
			QApplication::sendEvent(object, event);
			object->installEventFilter(this);
		}

		return true;
	}

	return QDockWidget::eventFilter(object, event);
}


/**
 * Called when the Names button is clicked.
 */
void VuoNodeLibrary::clickedNamesButton()
{
	setHumanReadable(true);
	updateListViewForNewDisplayMode();
	updateUI();
}

/**
 * Called when the Flat Class button is clicked.
 */
void VuoNodeLibrary::clickedFlatClassButton()
{
	setHumanReadable(false);
	updateListViewForNewDisplayMode();
	updateUI();
}

/**
 * Sets the compiler instance to query for information about loaded modules.
 */
void VuoNodeLibrary::setCompiler(VuoCompiler *compiler)
{
	this->compiler = compiler;
}

/**
 * Populates the map of lowercased node class names to their official capitalizations.
 */
void VuoNodeLibrary::recordNodeClassCapitalizations()
{
	foreach(VuoCompilerNodeClass *nodeClass, loadedNodeClasses)
	{
		string className = nodeClass->getBase()->getClassName();
		capitalizationForNodeClass[QString(className.c_str()).toLower().toUtf8().constData()] = className;
	}
}

/**
 *  Initialize a node popover for the provided node class.
 *  Returns a pointer to the newly initailized popover, as well as adding it to the @c popoverForNodeClass map.
 */
VuoNodePopover * VuoNodeLibrary::initializeNodePopoverForClass(VuoNodeClass *nodeClass, VuoCompiler *compiler)
{
	VuoNodePopover *popover = new VuoNodePopover(nodeClass, compiler);
	connect(popover, &VuoNodePopover::popoverDisplayRequested, this, &VuoNodeLibrary::displayPopoverInPane, Qt::QueuedConnection);
	connect(popover, &VuoNodePopover::textSelectionChanged, this, &VuoNodeLibrary::documentationSelectionChanged);
	connect(this, &VuoNodeLibrary::nodeDocumentationPanelWidthChanged, popover, &VuoNodePopover::setTextWidth);
	popover->setTextWidth(ui->nodePopoverPane->viewport()->rect().width());
	popoverForNodeClass[nodeClass->getClassName()] = popover;
	return popover;
}

/**
 * Returns the popover associated with the provided node class, creating the popover
 * for the first time if necessary.
 */
VuoNodePopover * VuoNodeLibrary::getNodePopoverForClass(VuoNodeClass *nodeClass, VuoCompiler *compiler)
{
	string className = nodeClass->getClassName();
	map<string, VuoNodePopover *>::iterator popover = popoverForNodeClass.find(className);
	if (popover != popoverForNodeClass.end())
		return popover->second;
	else
	{
		VuoNodePopover *newPopover = initializeNodePopoverForClass(nodeClass, compiler);
		return newPopover;
	}
}

/**
 *  Populate the node library list with the provided ordered list of node classes.
 */
void VuoNodeLibrary::populateList(vector<VuoCompilerNodeClass *> nodeClasses, bool resetSelection)
{
	QList<QListWidgetItem *> preselectedItems = ui->nodeClassList->selectedItems();
	set<string> preselectedClasses;

	if (!resetSelection)
	{
		foreach (QListWidgetItem *item, preselectedItems)
			preselectedClasses.insert(item->data(VuoNodeClassListItemDelegate::classNameIndex).toString().toUtf8().constData());
	}

	ui->nodeClassList->clear();
	int firstSelectedMatchIndex = -1;
	int currentItemIndex = 0;

	foreach (VuoCompilerNodeClass *nodeClass, nodeClasses)
	{
		string className = nodeClass->getBase()->getClassName();
		string humanReadableName = nodeClass->getBase()->getDefaultTitle();

		QListWidgetItem *item = new QListWidgetItem();
		item->setData(Qt::DisplayRole, className.c_str());	// used for sort order
		item->setData(VuoNodeClassListItemDelegate::humanReadableNameIndex, humanReadableName.c_str());
		item->setData(VuoNodeClassListItemDelegate::classNameIndex, className.c_str());		// node class name, for drag-and-drop operations
		item->setData(VuoNodeClassListItemDelegate::nodeClassPointerIndex, qVariantFromValue((void *)nodeClass));

		ui->nodeClassList->addItem(item);

		if (preselectedClasses.find(className) != preselectedClasses.end())
		{
			item->setSelected(true);

			if (firstSelectedMatchIndex == -1)
				firstSelectedMatchIndex = currentItemIndex;
		}
		else
			item->setSelected(false);

		currentItemIndex++;
	}

	// Restore the previous selection or set a new one, as appropriate.
	if (ui->nodeClassList->count() >= 1)
	{
		if (firstSelectedMatchIndex >= 0)
			ui->nodeClassList->setCurrentItem(ui->nodeClassList->item(firstSelectedMatchIndex));
		else
		{
			ui->nodeClassList->item(0)->setSelected(true);
			ui->nodeClassList->setCurrentItem(ui->nodeClassList->item(0));
		}
	}

	ui->nodeClassList->update();
}

/**
 * Update the current view of the node class list to accommodate a change in display mode.
 */
void VuoNodeLibrary::updateListViewForNewDisplayMode()
{
	updateListView(false);
}

/**
 * Starts a timer that coalesces node class list updates.
 */
void VuoNodeLibrary::updateListViewForNewFilterTextOnTimer()
{
	ui->nodeClassList->selectionFinalized = false;
	updateListViewTimer->start(100);
}

/**
 * Immediately updates the current view of the node class list to accommodate a change in filter text.
 */
void VuoNodeLibrary::updateListViewForNewFilterTextNow()
{
	updateListView(true);
	ui->nodeClassList->selectionFinalized = true;
}

/**
 * Returns true if one of `cnc`'s input/output ports has a name or data type matching `needle`.
 */
bool VuoNodeLibrary::nodeHasPortMatchingString(VuoCompilerNodeClass *cnc, string needle, bool isInput)
{
	auto portsToSearch = isInput ? cnc->getBase()->getInputPortClasses() : cnc->getBase()->getOutputPortClasses();
	for (VuoPortClass *p : portsToSearch)
	{
		string name = p->getName();
		std::transform(name.begin(), name.end(), name.begin(), ::tolower);
		if (name.find(needle) != string::npos)
			return true;

		VuoCompilerPortClass *cpc = dynamic_cast<VuoCompilerPortClass *>(p->getCompiler());
		if (!cpc)
			continue;
		VuoType *dataType = cpc->getDataVuoType();
		if (!dataType)
			continue;
		string key = dataType->getModuleKey();
		std::transform(key.begin(), key.end(), key.begin(), ::tolower);
		if (key.find(needle) != string::npos)
			return true;

		VuoGenericType *genericType = dynamic_cast<VuoGenericType *>(dataType);
		if (!genericType)
			continue;
		VuoGenericType::Compatibility compatibility;
		for (string specializedType : genericType->getCompatibleSpecializedTypes(compatibility))
		{
			std::transform(specializedType.begin(), specializedType.end(), specializedType.begin(), ::tolower);
			if (specializedType.find(needle) != string::npos)
				return true;
		}

	}
	return false;
}

/**
 * Returns true if the provided node has source of the specified format, where
 * recognized formats are ".vuo" for subcompositions, ".fs" for shaders, ".c" for
 * C-language nodes, and ".vuonode" for 3rd-party pre-compiled nodes.
 */
bool VuoNodeLibrary::nodeHasSourceType(VuoCompilerNodeClass *cnc, string sourceType)
{
	QString actionText, sourcePath;
	bool nodeClassEditable = VuoEditorUtilities::isNodeClassEditable(cnc->getBase(), actionText, sourcePath);

	if ((sourceType == ".vuo") && cnc->isSubcomposition())
		return true;
	else if ((sourceType == ".fs") && cnc->isIsf())
		return true;
	else if ((sourceType == ".c") && nodeClassEditable && !cnc->isSubcomposition() && !cnc->isIsf())
		return true;
	else if ((sourceType == ".vuonode") && !cnc->isBuiltIn() && !nodeClassEditable)
		return true;

	return false;
}

/**
 * Update the current view of the node class list.
 */
void VuoNodeLibrary::updateListView(bool resetSelection)
{
	// Split node class filter text on whitespace.
	QString textFilter = ui->textFilter->text();
	ui->nodeClassList->setFilterText(textFilter);
	QStringList tokenizedTextFilter = textFilter.split(QRegExp("\\s+"), QString::SkipEmptyParts);

	// Extract the filter predicates.
	QStringList predicates;
	for (QString token : tokenizedTextFilter)
	{
		if (token.startsWith("in:")
		 || token.startsWith("out:")
		 || token.startsWith("source:"))
		{
			predicates << token;
			tokenizedTextFilter.removeOne(token);
		}
	}

	vector<VuoCompilerNodeClass *> matchingNodeClasses = getMatchingNodeClassesForSearchTerms(tokenizedTextFilter);

	// If the first search yielded no matches, split the text on '.' as well as whitespace and try again.
	if ((matchingNodeClasses.size() == 0) && textFilter.contains('.'))
	{
		tokenizedTextFilter = textFilter.split(QRegExp("[\\s\\.]+"), QString::SkipEmptyParts);
		matchingNodeClasses = getMatchingNodeClassesForSearchTerms(tokenizedTextFilter);
	}

	// Apply the filter predicates.
	for (QString predicate : predicates)
	{
		string filterValue = predicate.split(':')[1].toLower().toStdString();
		std::function<bool(VuoCompilerNodeClass *)> predicateFunction;
		if (predicate.startsWith("in:"))
			predicateFunction = [=](VuoCompilerNodeClass *cnc) {
				return !nodeHasPortMatchingString(cnc, filterValue, true);
			};
		else if (predicate.startsWith("out:"))
			predicateFunction = [=](VuoCompilerNodeClass *cnc) {
				return !nodeHasPortMatchingString(cnc, filterValue, false);
			};
		else if (predicate.startsWith("source:"))
			predicateFunction = [=](VuoCompilerNodeClass *cnc) {
				return !nodeHasSourceType(cnc, filterValue);
			};

		matchingNodeClasses.erase(std::remove_if(matchingNodeClasses.begin(), matchingNodeClasses.end(), predicateFunction), matchingNodeClasses.end());
	}

	populateList(matchingNodeClasses, resetSelection);
}

/**
 * Given a list of terms to search for, returns a list of matching node classes,
 * ordered by relevance.
 */
vector<VuoCompilerNodeClass *> VuoNodeLibrary::getMatchingNodeClassesForSearchTerms(QStringList rawTermList)
{
	QStringList termList = applyFilterTransformations(rawTermList);
	int numTokens = termList.size();
	map<int, map<int, vector<VuoCompilerNodeClass *> > > nodeClassesWithTitleAndClassNameMatches;

	foreach (VuoCompilerNodeClass *nodeClass, loadedNodeClasses)
	{
		QString className = nodeClass->getBase()->getClassName().c_str();
		QString humanReadableName = nodeClass->getBase()->getDefaultTitle().c_str();

		// Check for the presence of each filter text token within the current
		// node class's searchable metadata.
		int numTokensFirstMatchedInTitle = 0;
		int numTokensFirstMatchedInClassName = 0;
		bool tokenSearchFailed = false;

		for (int tokenIndex = 0; (! tokenSearchFailed) && (tokenIndex < numTokens); ++tokenIndex)
		{
			QString currentToken = termList.at(tokenIndex);
			QStringMatcher textFilterPattern(currentToken, Qt::CaseInsensitive);
			bool foundCurrentTokenInTitle = false;
			bool foundCurrentTokenInClassName = false;
			bool foundCurrentTokenInKeywords = false;
			QStringList nodeTitleTokens = tokenizeNodeName(humanReadableName, "");
			QStringList nodeClassNameTokens = tokenizeNodeName("", className);

			for (QStringList::iterator nodeTitleToken = nodeTitleTokens.begin(); (! foundCurrentTokenInTitle) && (nodeTitleToken != nodeTitleTokens.end()); ++nodeTitleToken)
			{
				// At least one of the tokens within the human-readable node class title
				// must begin with the filter text token in order for the node class to be
				// considered a match based on its title.
				if (textFilterPattern.indexIn(*nodeTitleToken) == 0)
					foundCurrentTokenInTitle = true;
			}

			if (foundCurrentTokenInTitle)
				numTokensFirstMatchedInTitle++;

			if (! foundCurrentTokenInTitle)
			{
				for (QStringList::iterator nodeClassNameToken = nodeClassNameTokens.begin(); (! foundCurrentTokenInClassName) && (nodeClassNameToken != nodeClassNameTokens.end()); ++nodeClassNameToken)
				{
					// At least one of the tokens within the node class name
					// must begin with the filter text token in order for the node class to be
					// considered a match based on its node class name.
					if (textFilterPattern.indexIn(*nodeClassNameToken) == 0)
						foundCurrentTokenInClassName = true;
				}
			}

			if (foundCurrentTokenInClassName)
				numTokensFirstMatchedInClassName++;

			if (!(foundCurrentTokenInTitle || foundCurrentTokenInClassName))
			{
				vector<string> keywords = nodeClass->getBase()->getKeywords();
				vector<string> automaticKeywords = nodeClass->getAutomaticKeywords();
				foreach (string automaticKeyword, automaticKeywords)
					keywords.push_back(automaticKeyword);

				// Add port display names to searchable keywords.
				vector<VuoPortClass *> inputPorts = nodeClass->getBase()->getInputPortClasses();
				foreach (VuoPortClass *port, inputPorts)
					keywords.push_back(static_cast<VuoCompilerPortClass *>(port->getCompiler())->getDisplayName());

				vector<VuoPortClass *> outputPorts = nodeClass->getBase()->getOutputPortClasses();
				foreach (VuoPortClass *port, outputPorts)
					keywords.push_back(static_cast<VuoCompilerPortClass *>(port->getCompiler())->getDisplayName());

				// For "Share Value" and "Share List" nodes, add type display names to searchable keywords.
				if (	(nodeClass->getBase()->getClassName() == "vuo.data.share") ||
						(nodeClass->getBase()->getClassName() == "vuo.data.share.list"))
				{
					map<string, VuoCompilerType *> loadedTypes = compiler->getTypes();
					for (map<string, VuoCompilerType *>::iterator i = loadedTypes.begin(); i != loadedTypes.end(); ++i)
					{
						string typeName = i->first;
						if ((!VuoType::isListTypeName(typeName)) &&
								!VuoType::isDictionaryTypeName(typeName) &&  /// @todo Remove after https://b33p.net/kosada/node/5300
								(typeName != "VuoMathExpressionList"))  /// @todo Remove after https://b33p.net/kosada/node/8550

						{
							VuoCompilerType *type = i->second;
							keywords.push_back(type->getBase()->getDefaultTitle());
						}
					}
				}

				// Split keyphrases on whitespace so that each one is considered an individual searchable keyword.
				QStringList formattedKeywords;
				foreach (string keyword, keywords)
				{
					QStringList tokenizedKeywords = QString(keyword.c_str()).split(QRegExp("\\s+"), QString::SkipEmptyParts);
					foreach (QString tokenizedKeyword, tokenizedKeywords)
						formattedKeywords.push_back(tokenizedKeyword);
				}

				for (int i = 0; (!foundCurrentTokenInKeywords) && (i < formattedKeywords.size()); ++i)
				{
					QString keyword = formattedKeywords[i];

					// At least one node class keyword must begin with the filter text token in order for
					// the node class to be considered a match based on its keywords.
					if (textFilterPattern.indexIn(keyword) == 0)
						foundCurrentTokenInKeywords = true;
				}
			}

			if (!(foundCurrentTokenInTitle || foundCurrentTokenInClassName || foundCurrentTokenInKeywords || isStopWord(currentToken.toUtf8().constData())))
				tokenSearchFailed = true;
		}

		if (!tokenSearchFailed)
			nodeClassesWithTitleAndClassNameMatches[numTokensFirstMatchedInTitle][numTokensFirstMatchedInClassName].push_back(nodeClass);
	}

	// List matching node classes:
	// - Primarily in descending order of number of search token matches within the human-readable node title;
	// - Secondarily in descending order of number of search token matches within the node class name.
	vector<VuoCompilerNodeClass *> matchingNodeClasses;
	for (int i = numTokens; i >= 0; i--)
	{
		for (int j = numTokens-i; j >= 0; j--)
		{
			vector<VuoCompilerNodeClass *> currentMatches = nodeClassesWithTitleAndClassNameMatches[i][j];

			// List tertiarily in descending order of approximated frequency of use if the filter text is non-empty
			// or the library is in "Display by name" mode.
			if (termList.size() > 0 || getHumanReadable())
				sort(currentMatches.begin(), currentMatches.end(), nodeClassLessThan);

			matchingNodeClasses.insert(matchingNodeClasses.end(), currentMatches.begin(), currentMatches.end());
		}
	}

	return matchingNodeClasses;
}

/**
  * Applies certain custom transformations to the input search filter text in order to ensure that the
  * most relevant nodes appear first in the search results.
  */
QStringList VuoNodeLibrary::applyFilterTransformations(QStringList filterTokenList)
{
	QStringList transformedTokenList;
	foreach (QString token, filterTokenList)
	{
		// Apply special handling for arithmetic and comparison operators:
		// First correct for anticipated variants in user-entered input.
		if (token == "=<")
			token = "<=";
		else if (token == "=>")
			token = ">=";

		// Now tweak token lists to ensure that the most relevant nodes are listed first in the search results.
		if (token == "+")
			transformedTokenList.append("add");
		else if (token == "-")
			transformedTokenList.append("subtract");
		else if (token == "/")
			transformedTokenList.append("divide");
		else if ((token == "*") || (token == "•") || (token == "×"))
			transformedTokenList.append("multiply");
		else if (token == "<")
			transformedTokenList.append("less");
		else if (token == ">")
			transformedTokenList.append("greater");
		else if ((token == "==") || (token == "=") || (token == "≠") || (token == "!=") || (token == "<>"))
			transformedTokenList.append("equal");
		else if ((token == "<=") || (token == ">="))
			transformedTokenList.append("compare");
		else if (token == "|")
			transformedTokenList.append("value");
		else if (token == "%")
			transformedTokenList.append("limit");
		else if (token == "^")
			transformedTokenList.append("exponentiate");

		transformedTokenList.append(token);
	}

	return transformedTokenList;
}

/**
 * Updates the UI elements (e.g., enables/disables buttons) based on the application's state.
 */
void VuoNodeLibrary::updateUI()
{
	// Force a re-paint of each list item, in case the display mode has changed.
	for (int nodeClassIndex = 0; nodeClassIndex < ui->nodeClassList->count(); ++nodeClassIndex)
		ui->nodeClassList->setRowHidden(nodeClassIndex, false);

	ui->nodeClassList->update();
	update();
}

/**
 * Retrieves certain components of the node library's current state.
 *
 * @param filterText The current text content of the search filter.
 * @param selectedNodeClasses The names of the currently selected node classes within the class list.
 * @param documentedNodeClass The name of the node class whose documentation is currently displayed, or the empty string if none.
 */
void VuoNodeLibrary::getState(QString &filterText, set<string> &selectedNodeClasses, string &documentedNodeClass)
{
	filterText = ui->textFilter->text();

	selectedNodeClasses.clear();
	QList<QListWidgetItem *> selectedItems = ui->nodeClassList->selectedItems();
	foreach (QListWidgetItem *item, selectedItems)
		selectedNodeClasses.insert(ui->nodeClassList->getNodeClassForItem(item)->getBase()->getClassName());

	VuoNodePopover *nodeDocumentation = dynamic_cast<VuoNodePopover *>(ui->nodePopoverPane->widget());
	documentedNodeClass = (nodeDocumentation? nodeDocumentation->getNodeClass()->getClassName() : "");
}

/**
 * Sets the filter text, node class selection, and node class documentation as indicated.
 */
void VuoNodeLibrary::setState(QString filterText, set<string> selectedNodeClasses, string documentedNodeClass)
{
	disconnect(ui->textFilter, &QLineEdit::textChanged, this, &VuoNodeLibrary::updateListViewForNewFilterTextOnTimer);
	ui->textFilter->setText(filterText);
	connect(ui->textFilter, &QLineEdit::textChanged, this, &VuoNodeLibrary::updateListViewForNewFilterTextOnTimer);

	ui->nodeClassList->disablePopovers();
	highlightNodeClass("", false, true);
	foreach (string nodeClassName, selectedNodeClasses)
		highlightNodeClass(nodeClassName, false, false);
	ui->nodeClassList->enablePopovers();

	prepareAndDisplayNodePopoverForClass(documentedNodeClass);
}

/**
 * Returns the currently selected text in the documentation pane, or the empty string if none.
 */
QString VuoNodeLibrary::getSelectedDocumentationText()
{
	VuoPanelDocumentation *documentationWidget= dynamic_cast<VuoPanelDocumentation *>(ui->nodePopoverPane->widget());
	return (documentationWidget? documentationWidget->getSelectedText() : "");
}

/**
 * Tokenizes the human-readable @c nodeName on whitespace; tokenizes the @c className on
 * '.' and, additionally, on both '.' and transitions from lowercase to uppercase;
 * also includes all '.'-segmented suffixes of the class name;
 * returns the combined token list.
 */
QStringList VuoNodeLibrary::tokenizeNodeName(QString nodeName, QString className)
{
	// Check whether the cache already contains the tokenized node name of interest.
	pair<QString, QString> namePair = make_pair(nodeName, className);
	map<pair<QString, QString>, QStringList>::iterator i = VuoNodeLibrary::tokensForNodeClass.find(namePair);
	if (i != VuoNodeLibrary::tokensForNodeClass.end())
		return i->second;

	QStringList tokenizedNodeName = nodeName.split(QRegExp("\\s+"), QString::SkipEmptyParts);
	QString classNameDelimitedByCaseTransitions = className;
	classNameDelimitedByCaseTransitions.replace(QRegExp("([a-z0-9])([A-Z])"), "\\1.\\2");
	QStringList tokenizedClassName = className.split(QRegExp("\\."), QString::SkipEmptyParts);
	QStringList classNameTokenizedAlsoByCaseTransitions = classNameDelimitedByCaseTransitions.split(QRegExp("\\."), QString::SkipEmptyParts);

	QStringList classNameSuffixes;
	if (!className.isEmpty())
	{
		QRegExp dotTerminatedPrefix("^.*\\.");
		dotTerminatedPrefix.setMinimal(true);

		QString currentClassNameSuffix = className;
		classNameSuffixes.append(currentClassNameSuffix);
		while (currentClassNameSuffix.contains('.'))
			classNameSuffixes.append(currentClassNameSuffix.remove(dotTerminatedPrefix));
	}

	tokenizedNodeName.append(tokenizedClassName);
	tokenizedNodeName.append(classNameTokenizedAlsoByCaseTransitions);
	tokenizedNodeName.append(classNameSuffixes);

	// Cache the result for later.
	VuoNodeLibrary::tokensForNodeClass[namePair] = tokenizedNodeName;

	return tokenizedNodeName;
}

/**
 * Returns a boolean indicating whether node class names are currently displayed in
 * "human-readable" mode (i.e., using default node titles rather than class names).
 */
bool VuoNodeLibrary::getHumanReadable()
{
	return ((VuoNodeClassListItemDelegate *)(ui->nodeClassList->itemDelegate()))->getHumanReadable();
}

/**
 * Focuses the node library's text filter widget and highlights any text
 * already displayed within the filter.
 */
void VuoNodeLibrary::focusTextFilter()
{
	ui->textFilter->setFocus();
	ui->textFilter->selectAll();
}

/**
 * Sets the node library's text filter content to the input @c text.
 */
void VuoNodeLibrary::searchForText(QString text)
{
	ui->textFilter->setText(text);
}

/**
 * Sets the boolean indicating whether node class names are currently displayed in
 * "human-readable" mode (i.e., using default node titles rather than class names).
 */
void VuoNodeLibrary::setHumanReadable(bool humanReadable)
{
	((VuoNodeClassListItemDelegate *)(ui->nodeClassList->itemDelegate()))->setHumanReadable(humanReadable);
	emit changedIsHumanReadable(getHumanReadable());
}

/**
 * Receive move events.  When the widget receives this event, it is already at the new position.
 */
void VuoNodeLibrary::moveEvent(QMoveEvent *event)
{
	bool disablePropagation = isHidden();

	if (!disablePropagation)
		emit nodeLibraryMoved(event->pos());
}

/**
 * Receive resize events.  When the widget receives this event, it already has its new geometry.
 */
void VuoNodeLibrary::resizeEvent(QResizeEvent *event)
{
	bool disablePropagation = isHidden();

	int oldWidth = event->oldSize().width();
	int newWidth = event->size().width();

	if (oldWidth != newWidth)
	{
		int documentationPaneContentWidth = ui->nodePopoverPane->viewport()->rect().width();

		if (!disablePropagation)
			emit nodeDocumentationPanelWidthChanged(documentationPaneContentWidth);

		// Respond to manual re-sizings by the user after the library has been shown,
		// but not to automatic re-sizings that occur beforehand.
		if (hasBeenShown)
		{
			if (!disablePropagation)
				emit nodeLibraryWidthChanged(newWidth);

			this->preferredNodeLibraryWidth = newWidth;
		}
	}

	int oldHeight = event->oldSize().height();
	int newHeight = event->size().height();

	if (oldHeight != newHeight)
	{
		if (!disablePropagation)
			emit nodeLibraryHeightChanged(newHeight);

		// Respond to manual re-sizings by the user after the library has been shown,
		// but not to automatic re-sizings that occur beforehand.
		if (hasBeenShown)
		{
			// The documentation panel height may or may not have actually changed --
			// we emit this signal liberally.
			if (!disablePropagation)
				emitNodeDocumentationPanelHeightChanged();
		}
	}
}

/**
 * Emits a signal indicating that the height of the node documentation panel has changed.
 */
void VuoNodeLibrary::emitNodeDocumentationPanelHeightChanged()
{
	QList<int> widgetSizes = ui->splitter->sizes();
	int newSize = widgetSizes[1];

	this->preferredNodeDocumentationPanelHeight = newSize;
	emit nodeDocumentationPanelHeightChanged(newSize);
}

/**
 * Emits a signal indicating that this node class library should be hidden (and that
 * all other node class libraries application-wide should follow suit).
 */
void VuoNodeLibrary::closeEvent(QCloseEvent *event)
{
	bool disablePropagation = isHidden();

	if (!disablePropagation)
		emitNodeLibraryHiddenOrUnhidden(false);
}

/**
 * Handles keypress events.
 */
void VuoNodeLibrary::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Escape)
	{
		// The 'Escape' key clears the text filter,
		// or closes the node library if the text filter is already empty.
		if (ui->textFilter->text().isEmpty())
			close();
		else
			ui->textFilter->clear();
	}
}

/**
 * Disassociates the currently displayed documentation widget from the
 * documentation panel, to ensure that the widget is not destroyed
 * along with the panel and may be used elsewhere.
 */
void VuoNodeLibrary::releaseDocumentationWidget()
{
	ui->nodePopoverPane->takeWidget();
}

/**
 * Receive show events.
 */
void VuoNodeLibrary::showEvent(QShowEvent *event)
{
	bool disablePropagation = isHidden();

	// The call to re-size the node library to the preferred width has to be made here rather than in
	// VuoNodeLibrary::prepareAndMakeVisible(). prepareAndMakeVisible() calls setVisible(true), which
	// indirectly calls this, but putting the re-sizing call within prepareAndMakeVisible() has no effect.
	if (preferredNodeLibraryWidth >= 0)
	{
		setMinimumWidth(preferredNodeLibraryWidth+1); // Prevents the docked library from losing its draggable border.
		setMinimumWidth(preferredNodeLibraryWidth);

	}
	else
	{
		setMinimumWidth(minimumWidth()+1); // Prevents the docked library from losing its draggable border.
		setMinimumWidth(minimumWidth()-1);
	}

	updateGeometry();
	updateSplitterPosition();

	// The node documentation panel content width must be estimated as if the
	// vertical scrollbar will be displayed, so that the documentation
	// text will be fully in view.
	int documentationPaneContentWidth = ui->nodePopoverPane->viewport()->rect().width() -
			ui->nodePopoverPane->verticalScrollBar()->sizeHint().width();

	if (!disablePropagation)
		emit nodeDocumentationPanelWidthChanged(documentationPaneContentWidth);

	// The call to display the first popover must come after the width-change
	// signal is emitted or the popover will be displayed with extra vertical
	// padding for a few moments.
	if (!this->hasBeenShown)
		ui->nodeClassList->enablePopovers();

	this->hasBeenShown = true;
}

/**
 * Makes the node library visible and sets its minimum and maximum widths to
 * their original values.
 */
void VuoNodeLibrary::prepareAndMakeVisible()
{
	setVisible(true);

	// The call to restore the original minimum and maximum widths has to be made here rather than in
	// VuoNodeLibrary::showEvent(); otherwise the call that sets the width to the preferred value has no effect.
	setMinimumWidth(defaultMinimumWidth);
	setMaximumWidth(defaultMaximumWidth);
}

/**
  * Enables and disables horizontal resizing of the node library.
  */
void VuoNodeLibrary::fixWidth(bool fix)
{
	if (fix)
	{
		widget()->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Ignored);
		widget()->setMinimumWidth(this->preferredNodeLibraryWidth >= 0?
									  this->preferredNodeLibraryWidth :
									  this->defaultMinimumWidth);
		widget()->setMaximumWidth(this->preferredNodeLibraryWidth >= 0?
									  this->preferredNodeLibraryWidth :
									  this->defaultMinimumWidth);
	}
	else
	{
		widget()->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
		widget()->setMinimumWidth(this->defaultMinimumWidth);
		widget()->setMaximumWidth(this->defaultMaximumWidth);
	}
}

/**
 * Updates the relative sizes of the widgets separated by the splitter
 * in accordance with the stored @c preferredNodeDocumentationPanelHeight.
 */
void VuoNodeLibrary::updateSplitterPosition()
{
	if (this->preferredNodeDocumentationPanelHeight != -1)
	{
		QList<int> oldWidgetSizes = ui->splitter->sizes();
		QList<int> newWidgetSizes;

		int totalAvailableSize = 0;
		foreach(int size, oldWidgetSizes)
			totalAvailableSize += size;

		// Splitter widget[0]: Node class list
		newWidgetSizes.append(totalAvailableSize - this->preferredNodeDocumentationPanelHeight);

		// Spliter widget[1]: Node documentation pane
		newWidgetSizes.append(this->preferredNodeDocumentationPanelHeight);
		ui->splitter->setSizes(newWidgetSizes);
	}
}

/**
 * Prepares (extracts any necessary resources for) and displays the popover
 * for the node class with the provided name, which in this case is not case-sensitive.
 */
void VuoNodeLibrary::prepareAndDisplayNodePopoverForClass(string nodeClassName)
{
	string lowercaseNodeClassName = QString(nodeClassName.c_str()).toLower().toUtf8().constData();
	VuoCompilerNodeClass *nodeClass = compiler->getNodeClass(capitalizationForNodeClass[lowercaseNodeClassName]);
	if (nodeClass)
		prepareAndDisplayNodePopoverForClass(nodeClass->getBase());
}

/**
 * Prepares (extracts any necessary resources for) and displays the popover
 * relevant to the provided node class.
 */
void VuoNodeLibrary::prepareAndDisplayNodePopoverForClass(VuoNodeClass *nodeClass)
{
	getNodePopoverForClass(nodeClass, compiler)->prepareResourcesAndShow();
}

/**
 * Enables display of popovers in response to user interaction and prepares
 * and displays the popover for the currently selected node class.
 */
void VuoNodeLibrary::displayPopoverForCurrentNodeClass()
{
	ui->nodeClassList->enablePopoversAndDisplayCurrent();
}

/**
 * Displays the provided widget in the popover pane. Assumes the popover
 * has been prepared (had any relevant resources extracted) already.
 */
void VuoNodeLibrary::displayPopoverInPane(QWidget *panelContentWidget, QString resourceDir)
{
	if (ui->nodePopoverPane->widget() == panelContentWidget)
		return;

	// Temporarily set the current working directory to the documentation resource directory
	// so that relative paths within the popover HTML are resolved correctly.
	QString savedWorkingDir = QDir::currentPath();
	if (!resourceDir.isEmpty())
		QDir::setCurrent(resourceDir);

	// Prevent the previous popover from being deleted when a new one is displayed.
	releaseDocumentationWidget();

	ui->nodePopoverPane->setWidget(panelContentWidget);

	if (!resourceDir.isEmpty())
		QDir::setCurrent(savedWorkingDir);
}

/**
 * Emits a @c nodeLibraryHiddenOrUnhidden(bool unhidden) signal to indicate that the
 * visibility of this node library has changed.
 */
void VuoNodeLibrary::emitNodeLibraryHiddenOrUnhidden(bool unhidden)
{
	emit nodeLibraryHiddenOrUnhidden(unhidden);
}

VuoNodeLibrary::~VuoNodeLibrary()
{
	emit aboutToBeDestroyed();

	// Prevent the documentation from being destroyed along with the documentation pane,
	// since (in the case of composition metadata), the composition window might try to
	// use it again later.
	releaseDocumentationWidget();

	releaseNodePopovers();

	delete ui;
}

/**
 * Comparison function for node classes, for use in sorting.
 * Returns a boolean indicating whether @c nodeClass1 should be considered less than
 * (listed before) @c nodeClass2.
 * When comparing a typecast class to a non-typecast class, always determines the
 * non-typecast class to be the lesser of the two (so that it is listed first).
 * Otherwise, returns @c true if @c nodeClass1 occurs *more* frequently than @c nodeClass2.
 * This means that nodes sorted using this function as less-than will be listed
 * in descending order of frequency.  In the case of a frequency tie, returns the node class
 * with the name that occurs first alphabetically.
 */
bool VuoNodeLibrary::nodeClassLessThan(VuoCompilerNodeClass *nodeClass1, VuoCompilerNodeClass *nodeClass2)
{
	string nodeClassName1 = nodeClass1->getBase()->getClassName();
	string nodeClassName2 = nodeClass2->getBase()->getClassName();

	// List subcompositions highlighted during the current editor session first (in reverse chronological highlighting order),
	// and subcompositions installed in a previous editor session next (alphabetically).
	bool nodeClass1IsSubcomposition = nodeClass1->isSubcomposition();
	bool nodeClass2IsSubcomposition = nodeClass2->isSubcomposition();

	if (nodeClass1IsSubcomposition && !nodeClass2IsSubcomposition)
		return true;

	else if (nodeClass2IsSubcomposition && !nodeClass1IsSubcomposition)
		return false;

	else if (nodeClass1IsSubcomposition && nodeClass2IsSubcomposition)
	{
		bool nodeClass1InstalledThisSession = newlyInstalledNodeClasses.find(nodeClass1) != newlyInstalledNodeClasses.end();
		bool nodeClass2InstalledThisSession = newlyInstalledNodeClasses.find(nodeClass2) != newlyInstalledNodeClasses.end();

		if (nodeClass1InstalledThisSession && !nodeClass2InstalledThisSession)
			return true;
		else if (nodeClass2InstalledThisSession && !nodeClass1InstalledThisSession)
			return false;
		else if (nodeClass1InstalledThisSession && nodeClass2InstalledThisSession)
			return (newlyInstalledNodeClasses[nodeClass1] > newlyInstalledNodeClasses[nodeClass2]);
		else // if (!nodeClass1InstalledThisSession && !nodeClass2InstalledThisSession)
			return (nodeClassName1 < nodeClassName2);
	}

	// List typecasts last.
	bool nodeClass1IsTypecast = nodeClass1->getBase()->isTypecastNodeClass();
	bool nodeClass2IsTypecast = nodeClass2->getBase()->isTypecastNodeClass();
	if (nodeClass1IsTypecast && !nodeClass2IsTypecast)
		return false;
	else if (nodeClass2IsTypecast && !nodeClass1IsTypecast)
		return true;

	// Otherwise, list by decreasing frequency.
	int nodeClass1Frequency = nodeClassFrequency[nodeClassName1];
	int nodeClass2Frequency = nodeClassFrequency[nodeClassName2];

	return ((nodeClass1Frequency > nodeClass2Frequency) ||
			((nodeClass1Frequency == nodeClass2Frequency) && (nodeClassName1 < nodeClassName2)));
}

/**
 * Clears the stored list of node classes.
 *
 * Also resets the filter text, clears the popover pane, and resets the list of cached popovers.
 */
void VuoNodeLibrary::clearNodeClassList()
{
	loadedNodeClasses.clear();

	// Clear the popover pane and reset the list of cached popovers.
	displayPopoverInPane(NULL, "");
	releaseNodePopovers();

	updateNodeClassList(vector<string>(), vector<VuoCompilerNodeClass *>());
}

/**
 * Updates the display and the stored list of node classes, removing those with class names
 * in @a nodeClassesToRemove and adding those in @a nodeClassesToAdd.
 *
 * Also resets the filter text and, if any node classes are being removed,
 * clears the popover pane and resets the list of cached popovers.
 */
void VuoNodeLibrary::updateNodeClassList(const vector<string> &nodeClassesToRemove,
										 const vector<VuoCompilerNodeClass *> &nodeClassesToAdd)
{
	if (nodeClassesToRemove.size() >= 1)
	{
		// Clear the popover pane and reset the list of cached popovers.
		displayPopoverInPane(NULL, "");
		releaseNodePopovers();
	}

	// Remove the old node classes from the list.
	for (int i = loadedNodeClasses.size() - 1; i >= 0; --i)
	{
		string nodeClassName = loadedNodeClasses[i]->getBase()->getClassName();
		if (std::find(nodeClassesToRemove.begin(), nodeClassesToRemove.end(), nodeClassName) != nodeClassesToRemove.end())
			loadedNodeClasses.erase(loadedNodeClasses.begin() + i);
	}

	// Add the filtered list of new node classes to the list.
	vector<VuoCompilerNodeClass *> nodeClassesToAddCopy = nodeClassesToAdd;
	cullHiddenNodeClasses(nodeClassesToAddCopy);
	foreach (VuoCompilerNodeClass *nodeClassToAdd, nodeClassesToAddCopy)
		if (std::find(loadedNodeClasses.begin(), loadedNodeClasses.end(), nodeClassToAdd) == loadedNodeClasses.end())
			loadedNodeClasses.push_back(nodeClassToAdd);

	recordNodeClassCapitalizations();

	// Clear the filter text to make sure any newly added nodes are immediately visible in the list.
	// @todo https://b33p.net/kosada/node/15217 and https://b33p.net/kosada/node/14657:
	// Be more discerning -- this isn't desirable behavior in all circumstances.
	bool clearFilterText = false;
	if (clearFilterText)
	{
		// Disable the timer-based refresh prior to changes to the filter text.
		disconnect(ui->textFilter, &QLineEdit::textChanged, this, &VuoNodeLibrary::updateListViewForNewFilterTextOnTimer);

		ui->textFilter->setText("");

		// Re-enable the timer-based refresh following changes to the filter text.
		connect(ui->textFilter, &QLineEdit::textChanged, this, &VuoNodeLibrary::updateListViewForNewFilterTextOnTimer);
	}

	{
		ui->nodeClassList->disablePopovers();
		updateListView(false);
		ui->nodeClassList->enablePopovers();
	}

	if (nodeClassesToRemove.size() >= 1)
	{
		// Re-display something relevant in the documentation panel if we cleared it previously.
		displayPopoverForCurrentNodeClass();
	}

	updateUI();
}

/**
 * Selects the provided node class within the library.
 * If @c highlightAsNewlyInstalled is true, also marks the node class as being newly installed, for possible custom display.
 * If @c resetSelection is true, clears any previous node class selection.
 */
void VuoNodeLibrary::highlightNodeClass(string targetNodeClassName, bool highlightAsNewlyInstalled, bool resetPreviousSelection)
{
	// Avoid bug where the final set of selected items in the QListWidget is apparently
	// sensitive to the order in which their selection status is set:
	// First iterate through the entire list and select only the target node class.
	bool searchDone = !targetNodeClassName.empty();
	for (int i = 0; i < ui->nodeClassList->count() && !searchDone; ++i)
	{
		QListWidgetItem *currentItem = ui->nodeClassList->item(i);
		string currentNodeClassName = currentItem->data(VuoNodeClassListItemDelegate::classNameIndex).toString().toUtf8().constData();
		if (currentNodeClassName == targetNodeClassName)
		{
			ui->nodeClassList->setCurrentItem(currentItem);
			currentItem->setSelected(true);
			searchDone = true;
		}
	}

	if (resetPreviousSelection)
	{
		// Now do a second pass and deselect everything other than the target node class.
		for (int i = 0; i < ui->nodeClassList->count(); ++i)
		{
			QListWidgetItem *currentItem = ui->nodeClassList->item(i);
			string currentNodeClassName = currentItem->data(VuoNodeClassListItemDelegate::classNameIndex).toString().toUtf8().constData();
			if (currentNodeClassName != targetNodeClassName)
				currentItem->setSelected(false);
		}
	}

	ui->nodeClassList->update();

	if (highlightAsNewlyInstalled)
		newlyInstalledNodeClasses[compiler->getNodeClass(targetNodeClassName)] = newlyInstalledNodeClasses.size();

	updateListView(false);
}

/**
 * Populates the node class frequency map, for use in sorting nodes that all
 * match a given search query.  Frequencies were generated using countNodes.pl,
 * but may be tweaked as desired.
 */
void VuoNodeLibrary::populateNodeClassFrequencyMap()
{
#include "VuoNodeClassFrequencyMap.hh"
}

/**
 * Populates the stop-word list, for use in node class filtering.
 */
void VuoNodeLibrary::populateStopWordList()
{
	stopWords.insert("a");
	stopWords.insert("an");
	stopWords.insert("for");
	stopWords.insert("from");
	stopWords.insert("in");
	stopWords.insert("on");
	stopWords.insert("of");
	stopWords.insert("the");
	stopWords.insert("to");
	stopWords.insert("with");
}

/**
 * Returns a boolean indicating whether the input @c word is a stop word.
 */
bool VuoNodeLibrary::isStopWord(QString word)
{
	return (stopWords.find(word.toLower().toUtf8().constData()) != stopWords.end());
}

void VuoNodeLibrary::releaseNodePopovers()
{
	// Temporarily disabled deletion to work around crash when docking the node library.
	// https://b33p.net/kosada/node/15784
	for (auto i : popoverForNodeClass)
		i.second->cleanup();
//		i.second->deleteLater();

	popoverForNodeClass.clear();
}

/**
 * Makes the node library dark.
 */
void VuoNodeLibrary::updateColor(bool isDark)
{
	QString titleTextColor            = isDark ? "#303030" : "#808080";
	QString titleBackgroundColor      = isDark ? "#919191" : "#efefef";
	QString dockwidgetBackgroundColor = isDark ? "#505050" : "#efefef";
	QString listBackgroundColor       = isDark ? "#262626" : "#ffffff";
	QString scrollBarColor            = isDark ? "#505050" : "#dfdfdf";
	QString focusRingColor            = isDark ? "#1d6ae5" : "#74acec";

	setStyleSheet(VUO_QSTRINGIFY(
					  QDockWidget {
						  titlebar-close-icon: url(:/Icons/dockwidget-close-%3.png);
						  font-size: 11px;
						  border: none;
						  color: %1;
					  }
					  QDockWidget::title {
						  text-align: left;
						  margin-left: -14px;
						  background-color: %2;
					  }

					  // Hide it (Qt doesn't seem to support `display: none`).
					  QDockWidget::float-button {
						  top: -999px;
						  left: -999px;
					  }
				  )
				  .arg(titleTextColor)
				  .arg(titleBackgroundColor)
				  .arg(isDark ? "dark" : "light")
				  );

	ui->splitter->setStyleSheet(VUO_QSTRINGIFY(
									QSplitter::handle {
										background: transparent;
										height: 6px;
									}
									QSplitter {
										background-color: %1;
									}
								)
								.arg(dockwidgetBackgroundColor)
								);


	QString nodeClassListStyle = VUO_QSTRINGIFY(
				VuoNodeClassList {
					background: %2;
					border-radius: 5px;
					color: #cacaca;
					border: 1px solid %1;
					padding: 1px;
					margin: 3px 2px 0 3px;
				}
				VuoNodeClassList:focus {
					background: %2;
					color: #cacaca;
					border: 2px solid %3;
					padding: 0;
				}
				QAbstractScrollArea::corner {
					border: none;
				}
				)
			.arg(dockwidgetBackgroundColor)
			.arg(listBackgroundColor)
			.arg(focusRingColor);

	if (!VuoEditorWindowToolbar::usingOverlayScrollers())
		nodeClassListStyle += VUO_QSTRINGIFY(
					QScrollBar {
						background: transparent;
						height: 10px;
						width: 10px;
					}
					QScrollBar::handle {
						background: %1;
						border-radius: 3px;
						min-width: 20px;
						min-height: 20px;
						margin: 2px;
					}
					QAbstractScrollArea::corner,
					QScrollBar::add-line,
					QScrollBar::sub-line,
					QScrollBar::add-page,
					QScrollBar::sub-page {
						background: transparent;
						border: none;
					}
					)
				.arg(scrollBarColor);

	ui->nodeClassList->setStyleSheet(nodeClassListStyle);


	QString nodePopoverPaneStyle = VUO_QSTRINGIFY(
				QScrollArea,
				QGraphicsView {
					background: %1;
					border: none;
					border-radius: 5px;
					margin: 0 2px 3px 3px;
				}
				)
			.arg(listBackgroundColor);

	if (!VuoEditorWindowToolbar::usingOverlayScrollers())
		nodePopoverPaneStyle += VUO_QSTRINGIFY(
					QScrollBar {
						background: transparent;
						height: 10px;
						width: 10px;
					}
					QScrollBar::handle {
						background: %1;
						border-radius: 3px;
						min-width: 20px;
						min-height: 20px;
						margin: 2px;
					}
					QAbstractScrollArea::corner,
					QScrollBar::add-line,
					QScrollBar::sub-line,
					QScrollBar::add-page,
					QScrollBar::sub-page {
						background: transparent;
						border: none;
					}
					)
				.arg(scrollBarColor);

	ui->nodePopoverPane->setStyleSheet(nodePopoverPaneStyle);

	if (isFloating())
	{
#ifdef __APPLE__
		// This causes Qt to create an NSPanel instead of an NSWindow.
		setWindowFlags(windowFlags() | Qt::Dialog);

		id nsView = (id)winId();
		id nsWindow = ((id (*)(id, SEL))objc_msgSend)(nsView, sel_getUid("window"));
		unsigned long styleMask = 0;
		styleMask |= 1 << 0; // NSWindowStyleMaskTitled
		styleMask |= 1 << 1; // NSWindowStyleMaskClosable
		styleMask |= 1 << 3; // NSWindowStyleMaskResizable
		styleMask |= 1 << 4; // NSWindowStyleMaskUtilityWindow
		if (isDark)
			styleMask |= 1 << 13; // NSWindowStyleMaskHUDWindow
		((void (*)(id, SEL, unsigned long))objc_msgSend)(nsWindow, sel_getUid("setStyleMask:"), styleMask);

		// https://b33p.net/kosada/node/15082
		((void (*)(id, SEL, BOOL))objc_msgSend)(nsWindow, sel_getUid("setMovableByWindowBackground:"), NO);
#endif
	}
}
