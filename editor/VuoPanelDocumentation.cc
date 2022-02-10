/**
 * @file
 * VuoPanelDocumentation implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoPanelDocumentation.hh"

/**
 * Creates a new widget for displaying content within the documentation panel of the node library.
 * @todo https://b33p.net/kosada/node/9090 : Refactor VuoNodePopover and VuoCompositionMetaDataPanel here.
 */
VuoPanelDocumentation::VuoPanelDocumentation(QWidget *parent) :
	QWidget(parent)
{
	this->documentationEntered = false;
}

/**
 * Handles events for the documentation widget.
 */
bool VuoPanelDocumentation::event(QEvent * event)
{
	// Keep track of whether the cursor is currently positioned over this widget.
	if (event->type() == QEvent::Enter)
		documentationEntered = true;

	else if (event->type() == QEvent::Leave)
		documentationEntered = false;

	// Emit signals for repaint events received while the cursor is over this widget.
	// These events correspond to selection changes in the documentation text.
	else if (documentationEntered && (event->type() == QEvent::Paint))
		emit textSelectionChanged();

	// The selection may also be reset when the user clicks/navigates to a different widget.
	// Listening for layout request events isn't a perfect way to detect this, but covers many of the common cases.
	else if (event->type() == QEvent::LayoutRequest)
		emit textSelectionChanged();

	return QWidget::event(event);
}

/**
  * Returns the currently selected text within this widget's text field,
  * or the empty string of not applicable. Should be implemented by subclasses.
  */
QString VuoPanelDocumentation::getSelectedText()
{
	return QString("");
}
