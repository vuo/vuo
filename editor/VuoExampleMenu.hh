/**
 * @file
 * VuoExampleMenu interface.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

class VuoCompiler;
class VuoNodeSet;

/**
 * Provides a menu for opening the example compositions, organized by node set.
 */
class VuoExampleMenu : public QMenu
{
	Q_OBJECT

public:
	explicit VuoExampleMenu(QWidget *parent, VuoCompiler *compiler);
	void openRandomExample();
	void enableExampleTitleLookup();
	void disableExampleTitleLookup();
	void enableMenuItems(bool enable);

signals:
	void compilerReady(); ///< For internal use. Emitted when `compiler` is finished initializing and ready to provide information about node sets.
	void exampleSelected(QString exampleCompositionUrl); ///< Emitted when an example composition has been selected from the menu.

private slots:
	void exampleActionTriggered(void);
	void aboutToShowNodeSetSubmenu();

private:
	void populateMenus();
	void populateNodeSetSubmenu(QMenu *nodeSetMenu, VuoNodeSet *nodeSet, bool parseCustomizedTitles);
	QAction * addMenuItemForExampleComposition(QString exampleFileName, QString exampleNodeSetName, QMenu *parentMenu, bool parseCustomizedTitle);
	QString selectRandomExample();
	string lookUpCustomizedTitle(string exampleNodeSetName, string exampleFileName);
	void enableMenuItems(QMenu *menu, bool enable);

	VuoCompiler *compiler;
	vector<VuoNodeSet *> sortedNodeSets;
	map<string, string> nodeSetWithDisplayName;
	map<QString, QString> proExampleCompositionsAndNodeSets;
	map<QString, QString> modelExampleCompositionsAndNodeSets;
	map<QString, QString> protocolExampleCompositionsAndNodeSets;
	map<QString, bool> exampleTitlesAlreadyParsedForSubmenu;
	QIcon exampleCompositionIcon;
	QString randomCompositionIdentifier;
	bool exampleTitleLookupEnabled;
	bool enableMenusWhenNextPopulating;

	static map<pair<string, string>, string> customizedTitleForNodeSetExample;

#if VUO_PRO
#include "pro/VuoExampleMenu_Pro.hh"
#endif
};
