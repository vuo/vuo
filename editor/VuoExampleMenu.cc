/**
 * @file
 * VuoExampleMenu implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoExampleMenu.hh"

#include "VuoCompiler.hh"
#include "VuoCompilerNodeClass.hh"
#include "VuoComposition.hh"
#include "VuoCompositionMetadata.hh"
#include "VuoEventLoop.h"
#include "VuoNodeClass.hh"
#include "VuoNodeSet.hh"
#include "VuoEditor.hh"
#include "VuoEditorComposition.hh"
#include "VuoProtocol.hh"
#include "VuoStringUtilities.hh"

map<pair<string, string>, string> VuoExampleMenu::customizedTitleForNodeSetExample;

/**
 * Helper function for sorting node sets by name.
 */
static bool areNodeSetsInOrder(VuoNodeSet *first, VuoNodeSet *second)
{
	return first->getName() <= second->getName();
}

/**
 * Creates an empty menu, which will be populated asynchronously once @a compiler is ready.
 */
VuoExampleMenu::VuoExampleMenu(QWidget *parent, VuoCompiler *compiler) :
	QMenu(parent)
{
#if VUO_PRO
	VuoExampleMenu_Pro();
#endif

	this->compiler = compiler;
	this->exampleTitleLookupEnabled = true;
	this->enableMenusWhenNextPopulating = true;

	setTitle(QApplication::translate("VuoEditorWindow", "Open Example"));
	setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133

	this->exampleCompositionIcon = QIcon(":/Icons/vuo-composition.png");
	this->randomCompositionIdentifier = QObject::tr("Random Example");

	// Example compositions to be marked as containing pro nodes for non-pro users
	this->proExampleCompositionsAndNodeSets["CompareStereoTypes.vuo"] = "vuo.image";
	this->proExampleCompositionsAndNodeSets["HighlightEyesInVideo.vuo"] = "vuo.image";
	this->proExampleCompositionsAndNodeSets["MarkFacesInImage.vuo"] = "vuo.image";
	this->proExampleCompositionsAndNodeSets["MoveSeagullsCloserTogether.vuo"] = "vuo.image";
	this->proExampleCompositionsAndNodeSets["ObscureFacesInVideo.vuo"] = "vuo.image";
	this->proExampleCompositionsAndNodeSets["RemovePartsOfPhoto.vuo"] = "vuo.image";
	this->proExampleCompositionsAndNodeSets["WarpImageForFisheyeProjection.vuo"] = "vuo.image";
	this->proExampleCompositionsAndNodeSets["DisplayNDIVideo.vuo"] = "vuo.ndi";
	this->proExampleCompositionsAndNodeSets["SendLiveVideoWithNewsfeed.vuo"] = "vuo.ndi";
	this->proExampleCompositionsAndNodeSets["SendNDICheckerboard.vuo"] = "vuo.ndi";
	this->proExampleCompositionsAndNodeSets["SendNDICheckerboardAndMetadata.vuo"] = "vuo.ndi";
	this->proExampleCompositionsAndNodeSets["BounceStereoSphere.vuo"] = "vuo.scene";
	this->proExampleCompositionsAndNodeSets["MoveThroughTubeWithFisheye.vuo"] = "vuo.scene";
	this->proExampleCompositionsAndNodeSets["OperateOn3DObjects.vuo"] = "vuo.scene";
	this->proExampleCompositionsAndNodeSets["ShowTreesWithFisheye.vuo"] = "vuo.scene";
	this->proExampleCompositionsAndNodeSets["ReceiveArtnetMessages.vuo"] = "vuo.artnet";
	this->proExampleCompositionsAndNodeSets["SendArtnetMessages.vuo"] = "vuo.artnet";

	connect(this, &VuoExampleMenu::compilerReady, this, &VuoExampleMenu::populateMenus, Qt::QueuedConnection);

	dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
		// Wait for the compiler to be ready, without blocking the main thread.
		compiler->getNodeSetForName("");

		// Call populateMenus() unless the VuoExampleMenu has already been destroyed.
		emit compilerReady();
	});
}

/**
 * Adds the items to the menu.
 */
void VuoExampleMenu::populateMenus()
{
	// Quick Start composition to be displayed first in the menu.
	QString quickStartFileName = "QuickStart.vuo";
	QString quickStartNodeSetName = "vuo.image";
	addMenuItemForExampleComposition(quickStartFileName, quickStartNodeSetName, this, true);

	this->addSeparator();

	// Uberexamplecompositions to be displayed at the top level of the menu.
	this->modelExampleCompositionsAndNodeSets = static_cast<VuoEditor *>(qApp)->getUberExampleCompositions();
	for (map<QString, QString>::iterator i = modelExampleCompositionsAndNodeSets.begin(); i != modelExampleCompositionsAndNodeSets.end(); ++i)
	{
		QString exampleFileName = i->first;
		QString exampleNodeSetName = i->second;

		addMenuItemForExampleComposition(exampleFileName, exampleNodeSetName, this, true);
	}
	this->modelExampleCompositionsAndNodeSets[quickStartFileName] = quickStartNodeSetName;

	// Example compositions to be displayed in association with particular protocols in their own submenus.
	map<VuoProtocol *, QMenu *> submenuForProtocol;
	foreach (VuoProtocol *protocol, VuoProtocol::getProtocols())
	{
		QMenu *protocolMenu = new QMenu(this);
		protocolMenu->setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133
		protocolMenu->setTitle(QString::fromStdString(protocol->getName()));
		submenuForProtocol[protocol] = protocolMenu;

		map<QString, QString> exampleCompositionsAndNodeSetsForCurrentProtocol = static_cast<VuoEditor *>(qApp)->getExampleCompositionsForProtocol(protocol);
		for (map<QString, QString>::iterator i = exampleCompositionsAndNodeSetsForCurrentProtocol.begin(); i != exampleCompositionsAndNodeSetsForCurrentProtocol.end(); ++i)
		{
			QString exampleFileName = i->first;
			QString exampleNodeSetName = i->second;

			addMenuItemForExampleComposition(exampleFileName, exampleNodeSetName, protocolMenu, true);
			this->protocolExampleCompositionsAndNodeSets[exampleFileName] = exampleNodeSetName;
		}
	}

	this->addSeparator();

	// Randomly selected example composition.
	qsrand(QDateTime::currentDateTime().toSecsSinceEpoch());
	QAction *randomExampleAction = new QAction(this);
	randomExampleAction->setText(randomCompositionIdentifier);
	randomExampleAction->setShortcut(QKeySequence("Alt+Ctrl+O"));
	randomExampleAction->setEnabled(enableMenusWhenNextPopulating);
	connect(randomExampleAction, &QAction::triggered, this, &VuoExampleMenu::exampleActionTriggered);
	this->addAction(randomExampleAction);
	this->addSeparator();

	// Protocol-specific example compositions.
	foreach (VuoProtocol *protocol, VuoProtocol::getProtocols())
	{
		QMenu *protocolMenu = submenuForProtocol[protocol];
		if (!protocolMenu->isEmpty())
			this->addMenu(protocolMenu);
	}

	this->addSeparator();

	// Node-set-specific example compositions.
	map<string, VuoNodeSet *> nodeSetsMap = compiler->getNodeSets();
	for (auto n : nodeSetsMap)
		this->sortedNodeSets.push_back(n.second);
	std::sort(sortedNodeSets.begin(), sortedNodeSets.end(), areNodeSetsInOrder);

	for (vector<VuoNodeSet *>::iterator i = sortedNodeSets.begin(); i != sortedNodeSets.end(); ++i)
	{
		VuoNodeSet *nodeSet = *i;
		QMenu *nodeSetMenu = new QMenu(this);
		nodeSetMenu->setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133

		QString nodeSetDisplayName = VuoEditorComposition::formatNodeSetNameForDisplay(QString::fromStdString(nodeSet->getName()));
		nodeSetMenu->setTitle(nodeSetDisplayName);
		nodeSetWithDisplayName[nodeSetDisplayName.toUtf8().constData()] = nodeSet->getName();
		connect(nodeSetMenu, &QMenu::aboutToShow, this, &VuoExampleMenu::aboutToShowNodeSetSubmenu);

		populateNodeSetSubmenu(nodeSetMenu, nodeSet, false);

		if (!nodeSetMenu->isEmpty())
			this->addMenu(nodeSetMenu);
	}
}

/**
 * Populates the example composition submenu specific to the provided @c nodeSet.
 */
void VuoExampleMenu::populateNodeSetSubmenu(QMenu *nodeSetMenu, VuoNodeSet *nodeSet, bool parseCustomizedTitles)
{
	nodeSetMenu->clear();

	vector<string> exampleFileNames = nodeSet->getExampleCompositionFileNames();
	foreach (string exampleFileName, exampleFileNames)
		addMenuItemForExampleComposition(QString::fromStdString(exampleFileName), QString::fromStdString(nodeSet->getName()), nodeSetMenu, parseCustomizedTitles);

	exampleTitlesAlreadyParsedForSubmenu[nodeSetMenu->title()] = parseCustomizedTitles;
}

/**
 * Creates a menu item to open the provided example composition; adds it to the provided menu.
 */
QAction * VuoExampleMenu::addMenuItemForExampleComposition(QString exampleFileName, QString exampleNodeSetName, QMenu *parentMenu, bool parseCustomizedTitle)
{
	QAction *exampleAction = new QAction(parentMenu);

	QString compositionTitle = (parseCustomizedTitle? lookUpCustomizedTitle(exampleNodeSetName.toUtf8().constData(), exampleFileName.toUtf8().constData()) : "").c_str();
	if (compositionTitle.isEmpty())
		compositionTitle = VuoEditorComposition::formatCompositionFileNameForDisplay(exampleFileName);

	exampleAction->setText(compositionTitle);
	exampleAction->setData(VuoEditor::getURLForExampleComposition(exampleFileName, exampleNodeSetName));
	exampleAction->setIcon(exampleCompositionIcon);
	exampleAction->setEnabled(enableMenusWhenNextPopulating);

#if VUO_PRO
	addMenuItemForExampleComposition_Pro(exampleFileName, exampleNodeSetName, exampleAction);
#else
	map<QString, QString>::iterator i = proExampleCompositionsAndNodeSets.find(exampleFileName);
	if ((i != proExampleCompositionsAndNodeSets.end()) && (i->second == exampleNodeSetName))
		exampleAction->setEnabled(false);
#endif

	connect(exampleAction, &QAction::triggered, this, &VuoExampleMenu::exampleActionTriggered);
	parentMenu->addAction(exampleAction);

	return exampleAction;
}

/**
 * Responds when an example is selected from the menu.
 */
void VuoExampleMenu::exampleActionTriggered(void)
{
	QAction *exampleAction = (QAction *)QObject::sender();
	QString fileName = exampleAction->text();

	if (fileName == randomCompositionIdentifier)
		exampleAction->setData(selectRandomExample());

	QString filePath = (QString)exampleAction->data().value<QString>();
	emit exampleSelected(filePath);
}

/**
 * Responds when an example node set submenu is about to be displayed.
 */
void VuoExampleMenu::aboutToShowNodeSetSubmenu()
{
	QMenu *menu = static_cast<QMenu *>(sender());
	QString menuName = (menu? menu->title() : "");

	// Don't do the work for a given submenu more than once.
	if (!exampleTitleLookupEnabled || exampleTitlesAlreadyParsedForSubmenu[menuName])
		return;

	string nodeSetName = nodeSetWithDisplayName[menuName.toUtf8().constData()];
	VuoNodeSet *exampleNodeSet = ((compiler && !nodeSetName.empty())?
									  compiler->getNodeSetForName(nodeSetName) :
									  NULL);
	if (!exampleNodeSet)
		return;

	// Re-populate the menu with customized composition display names parsed from the composition headers.
	populateNodeSetSubmenu(menu, exampleNodeSet, true);
}

/**
 * Randomly selects an example composition and results its URL.
 */
QString VuoExampleMenu::selectRandomExample()
{
	if (sortedNodeSets.empty())
		populateMenus();

	bool pickedRandomExample = false;
	while (! pickedRandomExample)
	{
		// Workaround for bug https://b33p.net/kosada/node/17611
		// @todo: Use QRandomGenerator, available in Qt 5.10+
		qrand();

		int randomNodeSetIndex = qrand() % sortedNodeSets.size();
		VuoNodeSet *randomNodeSet = sortedNodeSets.at(randomNodeSetIndex);
		vector<string> randomNodeSetExampleNames = randomNodeSet->getExampleCompositionFileNames();
		if (randomNodeSetExampleNames.size() > 0)
		{
			int randomExampleIndex = qrand() % randomNodeSetExampleNames.size();
			QString randomExampleName = QString::fromStdString(randomNodeSetExampleNames.at(randomExampleIndex));

#if VUO_PRO
			if (selectRandomExample_Pro(randomNodeSet, randomExampleName))
				continue;
#endif

			return VuoEditor::getURLForExampleComposition(randomExampleName, QString::fromStdString(randomNodeSet->getName()));
		}
	}

	return "";
}

/**
 * Opens a random example composition.
 */
void VuoExampleMenu::openRandomExample()
{
	emit exampleSelected(selectRandomExample());
}

/**
 * Enables on-demand lookup of customized example titles.
 */
void VuoExampleMenu::enableExampleTitleLookup()
{
	this->exampleTitleLookupEnabled = true;
}

/**
 * Disables on-demand lookup of customized example titles.
 */
void VuoExampleMenu::disableExampleTitleLookup()
{
	this->exampleTitleLookupEnabled = false;
}

/**
 * Parses the provided composition source to determine the composition's customized title, if any.
 * If the composition has no customized title, returns the empty string.
 */
string VuoExampleMenu::lookUpCustomizedTitle(string exampleNodeSetName, string exampleFileName)
{
	// Check whether the cache already contains the example name of interest.
	pair<string, string> exampleNamePair = make_pair(exampleNodeSetName, exampleFileName);
	map<pair<string, string>, string>::iterator i = VuoExampleMenu::customizedTitleForNodeSetExample.find(exampleNamePair);
	if (i != VuoExampleMenu::customizedTitleForNodeSetExample.end())
		return i->second;

	// If not, parse the example composition now to find the display name.
	VuoNodeSet *exampleNodeSet = (compiler? compiler->getNodeSetForName(exampleNodeSetName) : NULL);
	string displayName = "";
	if (exampleNodeSet)
	{
		string compositionAsString = exampleNodeSet->getExampleCompositionContents(exampleFileName);
		VuoCompositionMetadata metadata(compositionAsString);
		displayName = metadata.getCustomizedName();
	}

	// Cache the result for later.
	VuoExampleMenu::customizedTitleForNodeSetExample[exampleNamePair] = displayName;
	return displayName;
}

/**
* Sets the enabled/disabled state of all items within this menu and its descendent menus,
* with the exception of items that should not be enabled for the current user session.
*/
void VuoExampleMenu::enableMenuItems(bool enable)
{
	enableMenuItems(this, enable);
	enableMenusWhenNextPopulating = enable;
}

/**
* Sets the enabled/disabled state of all items within the provided menu and its descendent menus,
* with the exception of items that should not be enabled for the current user session.
*
* Helper function for `VuoExampleMenu::enableMenuItems(bool enable)`.
*/
void VuoExampleMenu::enableMenuItems(QMenu *menu, bool enable)
{
	foreach (QAction *action, menu->actions())
	{
		// Apply recursively to submenus.
		if (action->menu())
			enableMenuItems(action->menu(), enable);
		else
		{
			action->setEnabled(enable);

			QUrl exampleURL(action->data().value<QString>());
			QString exampleNodeSetName = exampleURL.host();
			QString exampleFileName = VuoStringUtilities::substrAfter(exampleURL.path().toUtf8().constData(), "/").c_str();
#if VUO_PRO
			addMenuItemForExampleComposition_Pro(exampleFileName, exampleNodeSetName, action);
#else
			map<QString, QString>::iterator i = proExampleCompositionsAndNodeSets.find(exampleFileName);
			if ((i != proExampleCompositionsAndNodeSets.end()) && (i->second == exampleNodeSetName))
				action->setEnabled(false);
#endif
		}
	}
}
