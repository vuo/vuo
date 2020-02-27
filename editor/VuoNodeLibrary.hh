/**
 * @file
 * VuoNodeLibrary interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoHeap.h"

class VuoCompiler;
class VuoCompilerNodeClass;
class VuoEditorComposition;
class VuoNodeClass;
class VuoNodePopover;

namespace Ui
{
	class VuoNodeLibrary;
}

/**
 * A dockable window displaying the list of node classes that can be added to compositions.
 */
class VuoNodeLibrary : public QDockWidget
{
	Q_OBJECT

public:
	/**
	 * Specifies the display format for node classes within the library.
	 */
	enum nodeLibraryDisplayMode
	{
		displayByClass,
		displayByName
	};

	/**
	 * Specifies the node library visibility setting.
	 */
	enum nodeLibraryState
	{
		nodeLibraryHidden,
		nodeLibraryDocked,
		nodeLibraryFloating
	};

	explicit VuoNodeLibrary(VuoCompiler *compiler, QWidget *parent = 0,
							VuoNodeLibrary::nodeLibraryDisplayMode displayMode = VuoNodeLibrary::displayByClass);
	void setCompiler(VuoCompiler *compiler);
	static void cullHiddenNodeClasses(vector<VuoCompilerNodeClass *> &nodeClasses);
	void prepareAndMakeVisible();
	void fixWidth(bool fix);
	void displayPopoverForCurrentNodeClass();
	bool eventFilter(QObject *object, QEvent *event) VuoWarnUnusedResult;
	~VuoNodeLibrary();
	static QStringList tokenizeNodeName(QString nodeName, QString className);
	static bool isStopWord(QString word);
	bool getHumanReadable();
	void focusTextFilter();
	void searchForText(QString text);
	void getState(QString &filterText, set<string> &selectedNodeClasses, string &documentedNodeClass);
	void setState(QString filterText, set<string> selectedNodeClasses, string documentedNodeClass);
	QString getSelectedDocumentationText();
	void emitNodeLibraryHiddenOrUnhidden(bool unhidden);
	void releaseDocumentationWidget();
	void clearNodeClassList();
	void updateNodeClassList(const vector<string> &nodeClassesToRemove, const vector<VuoCompilerNodeClass *> &nodeClassesToAdd);
	void highlightNodeClass(string targetNodeClassName, bool highlightAsNewlyInstalled=true, bool resetPreviousSelection=true);
	void prepareAndDisplayNodePopoverForClass(string nodeClassName);

signals:
	void componentsAdded(QList<QGraphicsItem *> addedComponents, VuoEditorComposition *target); ///< Emitted when the user has used the Node Library to add nodes to the composition.
	void changedIsHumanReadable(bool humanReadable); ///< Emitted when the Node Library's display mode is changed.
	void nodeLibraryHiddenOrUnhidden(bool unhidden); ///< Emitted when the Node Library's visibility is changed.
	void nodeLibraryReceivedPasteCommand(); ///< Emitted when Node Library's text filter receives the keyboard shortcut for the 'Paste' command.
	void nodeLibraryMoved(QPoint newPos); ///< Emitted when the node library's position changes relative to its parent.
	void nodeLibraryWidthChanged(int); ///< Emitted when the node library's width changes.
	void nodeLibraryHeightChanged(int); ///< Emitted when the node library's height changes.
	void nodeDocumentationPanelWidthChanged(int); ///< Emitted when the node documentation panel's width changes.
	void nodeDocumentationPanelHeightChanged(int); ///< Emitted when the node documentation panel's height changes.
	void documentationSelectionChanged(); ///< Emitted when the documentation text selection changes.
	void aboutToBeDestroyed(); ///< Emitted when the Node Library's destructor has been called, but has not yet had any effect.

public slots:
	void clickedNamesButton();
	void clickedFlatClassButton();
	void setHumanReadable(bool humanReadable);
	void prepareAndDisplayNodePopoverForClass(VuoNodeClass *nodeClass);
	void displayPopoverInPane(QWidget *panelContentWidget, QString resourceDir="");
	void updateUI();

protected:
	void moveEvent(QMoveEvent *event);
	void resizeEvent(QResizeEvent *event);
	void showEvent(QShowEvent *event);
	void closeEvent(QCloseEvent *event);
	void keyPressEvent(QKeyEvent *event);

private slots:
	void updateListViewForNewFilterTextOnTimer();
	void updateListViewForNewFilterTextNow();
	void emitNodeDocumentationPanelHeightChanged();
	void updateColor(bool isDark);

private:
	friend class TestVuoEditor;

	Ui::VuoNodeLibrary *ui;
	VuoCompiler *compiler;
	vector<VuoCompilerNodeClass *> loadedNodeClasses;
	static map<VuoCompilerNodeClass *, int> newlyInstalledNodeClasses;
	map<string, VuoNodePopover *> popoverForNodeClass;
	map<string, string> capitalizationForNodeClass;
	static map<pair<QString, QString>, QStringList> tokensForNodeClass;
	static map<string, int> nodeClassFrequency;
	static set<string> stopWords;
	int preferredNodeDocumentationPanelHeight;
	int preferredNodeLibraryWidth;
	bool hasBeenShown;
	int defaultMinimumWidth;
	int defaultMaximumWidth;
	QTimer *updateListViewTimer;

	void populateList(vector<VuoCompilerNodeClass *> nodeClasses, bool resetSelection);
	static bool nodeHasPortMatchingString(VuoCompilerNodeClass *cnc, string needle, bool isInput);
	static bool nodeHasSourceType(VuoCompilerNodeClass *cnc, string sourceType);
	void updateListView(bool resetSelection);
	vector<VuoCompilerNodeClass *> getMatchingNodeClassesForSearchTerms(QStringList rawTermList);
	void updateListViewForNewDisplayMode();
	VuoNodePopover * initializeNodePopoverForClass(VuoNodeClass *nodeClass, VuoCompiler *compiler);
	VuoNodePopover * getNodePopoverForClass(VuoNodeClass *nodeClass, VuoCompiler *compiler);
	void updateSplitterPosition();
	static bool nodeClassLessThan(VuoCompilerNodeClass *nodeClass1, VuoCompilerNodeClass *nodeClass2);
	QStringList applyFilterTransformations(QStringList filterTokenList);
	void populateNodeClassFrequencyMap();
	void populateStopWordList();
	void recordNodeClassCapitalizations();
	void releaseNodePopovers();

#ifdef VUO_PRO
#include "pro/VuoNodeLibrary_Pro.hh"
#endif
};
