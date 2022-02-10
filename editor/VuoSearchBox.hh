/**
 * @file
 * VuoSearchBox interface.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoHeap.h"

class VuoEditorComposition;
class VuoNode;

namespace Ui
{
	class VuoSearchBox;
}

/**
 * Provides a dialog to search the composition for text.
 */
class VuoSearchBox : public QDockWidget
{
	Q_OBJECT

public:
	explicit VuoSearchBox(VuoEditorComposition *composition, QWidget *parent, Qt::WindowFlags flags);
	~VuoSearchBox();
	void show();
	bool traversalButtonsEnabled();

public slots:
	void goToNextResult();
	void goToPreviousResult();
	void refreshResults();

signals:
	void searchPerformed(); ///< Emitted after a search is performed.

protected:
	bool eventFilter(QObject *object, QEvent *event) VuoWarnUnusedResult;
	void resizeEvent(QResizeEvent *event);
	void closeEvent(QCloseEvent *event);

private:
	Ui::VuoSearchBox *ui;
	VuoEditorComposition *composition;
	vector<QGraphicsItem *> searchResults;
	int currentResultIndex;
	QLabel *resultCount;
	QString noResultsText;
	vector<QGraphicsItem *> getCurrentSearchResults(const QString &searchText);
	vector<QGraphicsItem *> findDeprecatedNodes();
	vector<QGraphicsItem *> findSubcompositionNodes();
	vector<QGraphicsItem *> findShaderNodes();
	vector<QGraphicsItem *> findCLanguageNodes();
	vector<QGraphicsItem *> find3rdPartyPrecompiledNodes();
	void updateViewportToFitResults();
	void updateResultCount();
	void repositionChildWidgets();
	void updateSpotlightedItems();
	bool excludeNodeFromSearchResults(VuoNode *node);
	static bool itemLessThan(QGraphicsItem *item1, QGraphicsItem *item2);

	QIcon searchIcon;
	QToolButton *searchButton;

	friend class TestVuoEditor;

private slots:
	void searchForText(const QString &searchText);
	void updateColor(bool isDark);
};
