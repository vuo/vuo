/**
 * @file
 * VuoConsoleWindow implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoConsole.hh"
#include "VuoConsoleToolbar.hh"
#include "VuoConsoleWindow.hh"
#include "VuoEditor.hh"
#include "VuoEditorUtilities.hh"
#include "VuoRecentFileMenu.hh"

/**
 * Initializes a window that is ready to have its model populated and be shown.
 */
VuoConsoleWindow::VuoConsoleWindow(VuoConsole *console, QWidget *screenMate) :
	QMainWindow()
{
	setAttribute(Qt::WA_DeleteOnClose);

	quickContainer = new QQuickWidget;
	quickContainer->setResizeMode(QQuickWidget::SizeRootObjectToView);
	quickContainer->rootContext()->setContextProperty(QStringLiteral("modelProvider"), console);
	quickContainer->setSource(QUrl("qrc:/qml/console/VuoConsole.qml"));

	setCentralWidget(quickContainer);
	setWindowTitle(tr("Console"));

	toolbar = VuoConsoleToolbar::create(this, console);

	populateMenus(console);

	connect(static_cast<VuoEditor *>(qApp), &VuoEditor::darkInterfaceToggled, this, &VuoConsoleWindow::updateColor);
	updateColor();

	connect(static_cast<VuoEditor *>(qApp), &VuoEditor::canvasOpacityChanged, this, &VuoConsoleWindow::updateOpacity);
	updateOpacity();

	setMinimumSize(540, 100);

	// QRect screenGeometry = screenMate->screen()->availableGeometry();  // Qt 5.14+
	QRect screenGeometry = qApp->desktop()->availableGeometry(screenMate);
	const int height = 300;
	move(screenGeometry.left(), screenGeometry.bottom() - height);
	resize(screenGeometry.width(), height);
}

/**
 * Sets the data model for the list of logs.
 */
void VuoConsoleWindow::setModel(const QStringList &logs)
{
	quickContainer->rootObject()->setProperty("logsModel", logs);
}

/**
 * Retrieves the data model for the list of logs.
 */
QStringList VuoConsoleWindow::getModel(void)
{
	return quickContainer->rootObject()->property("logsModel").toStringList();
}

/**
 * Retrieves the row indices that are currently selected in the list of logs.
 */
QList<QVariant> VuoConsoleWindow::getSelectedIndices(void)
{
	return quickContainer->rootObject()->property("selectedIndices").toList();
}

/**
 * Creates a menu bar for the window (if it doesn't already exist) and adds menus.
 */
void VuoConsoleWindow::populateMenus(VuoConsole *console)
{
	{
		// "File" menu
		QMenu *m = new QMenu(menuBar());
		m->setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133
		m->setTitle(tr("&File"));

		m->addAction(tr("&New Composition"), static_cast<VuoEditor *>(qApp), &VuoEditor::newComposition, QKeySequence("Ctrl+N"));

		{
			// "New Composition from Template" menu
			QMenu *mm = new QMenu(tr("New Composition from Template"));
			mm->setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133

			static_cast<VuoEditor *>(qApp)->populateNewCompositionWithTemplateMenu(mm);

			m->addMenu(mm);
		}

		{
			// "New Shader" menu
			QMenu *mm = new QMenu(tr("New Shader"));
			mm->setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133

			static_cast<VuoEditor *>(qApp)->populateNewShaderMenu(mm);

			m->addMenu(mm);
		}

		m->addSeparator();
		m->addAction(tr("&Open…"), static_cast<VuoEditor *>(qApp), &VuoEditor::openFile, QKeySequence("Ctrl+O"));

		{
			// "Open Recent" menu
			recentFileMenu = new VuoRecentFileMenu();

			connect(recentFileMenu, &VuoRecentFileMenu::recentFileSelected, static_cast<VuoEditor *>(qApp), &VuoEditor::openUrl);

			// Unlike editing windows, the console window doesn't add itself to the recent file list when it's opened,
			// triggering the editor to populate the window's recent file menu, so we have to do it explicitly.
			static_cast<VuoEditor *>(qApp)->synchronizeOpenRecentFileMenus();

			m->addMenu(recentFileMenu);
		}

		m->addSeparator();
		m->addAction(tr("&Save"), console, &VuoConsole::save, QKeySequence("Ctrl+S"));

		m->addSeparator();
		m->addAction(tr("Close"), this, &VuoConsoleWindow::close, QKeySequence("Ctrl+W"));

		// "About" menu item.
		// On macOS, this menu item will automatically be moved from the "File" menu to the Application menu by way of `QAction::AboutRole`.
		QAction *aboutAction = new QAction(nullptr);
		aboutAction->setText(tr("About Vuo…"));
		connect(aboutAction, &QAction::triggered, static_cast<VuoEditor *>(qApp), &VuoEditor::about);
		aboutAction->setMenuRole(QAction::AboutRole);
		m->addAction(aboutAction);

		// Connect the "Quit" menu item action to our customized quit method.
		// On macOS, this menu item will automatically be moved from the "File" menu to the Application menu by way of `QAction::QuitRole`.
		QAction *quitAction = new QAction(nullptr);
		quitAction->setText(tr("&Quit"));
		quitAction->setShortcut(QKeySequence("Ctrl+Q"));
		connect(quitAction, &QAction::triggered, static_cast<VuoEditor *>(qApp), &VuoEditor::quitCleanly);
		quitAction->setMenuRole(QAction::QuitRole);
		m->addAction(quitAction);

		menuBar()->addAction(m->menuAction());
	}

	{
		// "Edit" menu
		QMenu *m = new QMenu(menuBar());
		m->setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133
		m->setTitle(tr("Edit"));

		m->addAction(tr("Copy"), console, &VuoConsole::copy, QKeySequence("Ctrl+C"));

		m->addAction(tr("Select All"), [console](){ emit console->selectedAllLogs(); }, QKeySequence("Ctrl+A"));

		m->addAction(tr("Clear"), console, &VuoConsole::clear, QKeySequence("Ctrl+K"));

		menuBar()->addAction(m->menuAction());
	}

	{
		// "Help" menu
		QMenu *m = new QMenu(menuBar());
		m->setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133
		m->setTitle(tr("&Help"));

		static_cast<VuoEditor *>(qApp)->populateHelpMenu(m);

		menuBar()->addAction(m->menuAction());
	}
}

/**
 * Applies the editor's light/dark interface styling to the list of logs.
 */
void VuoConsoleWindow::updateColor(void)
{
	bool isDark = static_cast<VuoEditor *>(qApp)->isInterfaceDark();
	quickContainer->rootObject()->setProperty("isInterfaceDark", isDark);
}

/**
 * Applies the editor's canvas transparency setting to the list of logs.
 */
void VuoConsoleWindow::updateOpacity(void)
{
	int opacity = static_cast<VuoEditor *>(qApp)->getCanvasOpacity();
	VuoEditorUtilities::setWindowOpacity(this, opacity);
	quickContainer->rootObject()->setProperty("opacity", opacity/255.0);
}

/**
 * Returns the "File > Open Recent" menu associated with this console window.
 */
VuoRecentFileMenu * VuoConsoleWindow::getRecentFileMenu(void)
{
	return recentFileMenu;
}
