/**
 * @file
 * VuoNodeLibraryTextFilter implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoEditor.hh"
#include "VuoNodeLibraryTextFilter.hh"

/**
 * Creates a new widget for filtering the nodes displayed in the Node Library.
 */
VuoNodeLibraryTextFilter::VuoNodeLibraryTextFilter(QWidget *parent) :
	QLineEdit(parent)
{
	// Icons
	searchIcon = QIcon(QApplication::applicationDirPath().append("/../Resources/search-loupe.png"));
	clearIcon = QIcon(QPixmap(QApplication::applicationDirPath().append("/../Resources/search-clear.png")));
	clearIconPressed = QIcon(QPixmap(QApplication::applicationDirPath().append("/../Resources/search-clear-pressed.png")));

	// 'Search' button
	searchButton = new QToolButton(this);
	searchButton->setIcon(searchIcon);
	searchButton->setIconSize(QSize(16,14));
	searchButton->setCursor(Qt::ArrowCursor);
	searchButton->setStyleSheet("QToolButton { border: none; padding: 7px 3px 3px 6px; }");

	// 'Clear' button
	clearButton = new QToolButton(this);
	clearButton->setIcon(clearIcon);
	clearButton->setIconSize(QSize(16,14));
	clearButton->setCursor(Qt::ArrowCursor);
	clearButton->setStyleSheet("QToolButton { border: none; padding: 7px 3px 3px 5px; }");
	clearButton->hide();
	connect(clearButton, &QToolButton::pressed, this, &VuoNodeLibraryTextFilter::updateUI);
	connect(clearButton, &QToolButton::released, this, &VuoNodeLibraryTextFilter::updateUI);
	connect(clearButton, &QToolButton::clicked, this, &VuoNodeLibraryTextFilter::clear);
	connect(this, &VuoNodeLibraryTextFilter::textChanged, this, &VuoNodeLibraryTextFilter::updateUI);

#ifdef __APPLE__
	// Disable standard OS X focus 'glow'
	setAttribute(Qt::WA_MacShowFocusRect, false);
#endif

	VuoEditor *editor = (VuoEditor *)qApp;
	connect(editor, &VuoEditor::darkInterfaceToggled, this, &VuoNodeLibraryTextFilter::updateColor);
	updateColor(editor->isInterfaceDark());
}

/**
 * Handle resize events.
 */
void VuoNodeLibraryTextFilter::resizeEvent(QResizeEvent *event)
{
	QLineEdit::resizeEvent(event);

	// Move the "Clear" button manually instead of using a layout, to avoid bug
	// https://b33p.net/kosada/node/13381.
	clearButton->move(width()-clearButton->iconSize().width()-10, -1);
}

/**
 * Handle keypress events.
 */
void VuoNodeLibraryTextFilter::keyPressEvent(QKeyEvent *event)
{
	Qt::KeyboardModifiers modifiers = event->modifiers();
	switch (event->key())
	{
		// The 'Escape' key is handled by VuoNodeLibrary::keyPressEvent.


		// The 'Paste' keyboard shortcut disambiguates the pasted text and pastes it either
		// to the text filter or the canvas, as appropriate.
		case Qt::Key_V:
		{
			if (modifiers & Qt::ControlModifier)
			{
				emit nodeLibraryReceivedPasteCommand();
				break;
			}
			[[clang::fallthrough]];
		}


		default:
		{
			QLineEdit::keyPressEvent(event);
			break;
		}
	}
}

/**
 * Updates the UI elements (e.g., enables/disables buttons) based on the application's state.
 */
void VuoNodeLibraryTextFilter::updateUI()
{
	if (text().isEmpty())
		clearButton->hide();
	else
		clearButton->show();

	VuoEditor *editor = (VuoEditor *)qApp;
	bool clearButtonState = clearButton->isDown();
	if (editor->isInterfaceDark())
		clearButtonState = !clearButtonState;
	clearButton->setIcon(clearButtonState ? clearIconPressed : clearIcon);
}

/**
 * Returns a boolean indicating whether we are currently operating in Graphite display mode.
 */
bool VuoNodeLibraryTextFilter::inGraphiteDisplayMode()
{
	const QColor graphiteAlternateBaseColor = QColor(240, 240, 240);
	QColor currentAlternateBaseColor = QApplication::palette(this).color(QPalette::Normal, QPalette::AlternateBase);
	return (currentAlternateBaseColor == graphiteAlternateBaseColor);
}

VuoNodeLibraryTextFilter::~VuoNodeLibraryTextFilter()
{
}

/**
 * Handle keyboard focus events (focus lost).
 */
void VuoNodeLibraryTextFilter::focusOutEvent(QFocusEvent *event)
{
	QLineEdit::focusOutEvent(event);
	emit nodeLibraryTextFilterFocusLost();
}

/**
 * Makes the widget dark.
 */
void VuoNodeLibraryTextFilter::updateColor(bool isDark)
{
	clearButton->setIcon(isDark ? clearIconPressed : clearIcon);

	const QString focusGlowActiveColor   = isDark ? "#1d6ae5" : "#74acec";
	const QString focusGlowInactiveColor = isDark ? "#505050" : "#efefef";
	const QString textColor              = isDark ? "#cacaca" : "#606060";
	const QString backgroundColor        = isDark ? "#262626" : "#e1e1e1";

	const int borderWidth = 1;
	const int addedFocusBorderWidth = 1;
	const int rightPadding = clearButton->iconSize().width()+1;
	const int leftPadding = searchButton->iconSize().width()+3;
	const int verticalPadding = 1;
	setStyleSheet(QString("VuoNodeLibraryTextFilter {	border: %1px solid %10;"
														"border-radius: 5px;"
														"padding-right: %2px;" // Space for 'Clear' button
														"padding-left: %3px;" // Space for 'Search' button
														"padding-top: %4px;"
														"padding-bottom: %4px;"
														"background-color: %12;"
														"margin: 3px 2px 0 3px;"
														"color: %11;"
													"}"
						  "VuoNodeLibraryTextFilter:focus {	border: %6px solid %5;"
															"border-radius: 5px;"
															"padding-right: %7px;" // Space for 'Clear' button
															"padding-left: %8px;" // Space for 'Search' button
															"padding-top: %9px;"
															"padding-bottom: %9px;"
													"}"
						)
						.arg(borderWidth)
						.arg(rightPadding)
						.arg(leftPadding)
						.arg(verticalPadding)
						.arg(focusGlowActiveColor)
						.arg(borderWidth + addedFocusBorderWidth)
						.arg(rightPadding - addedFocusBorderWidth)
						.arg(leftPadding - addedFocusBorderWidth)
						.arg(verticalPadding - addedFocusBorderWidth)
						.arg(focusGlowInactiveColor)
						.arg(textColor)
						.arg(backgroundColor)
				  );
}
