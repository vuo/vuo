/**
 * @file
 * VuoPanelDocumentation implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoPanelDocumentation.hh"

#include "VuoEditor.hh"

/**
 * Creates a new widget for displaying content within the documentation panel of the node library.
 * @todo https://b33p.net/kosada/node/9090 : Refactor VuoNodePopover and VuoCompositionMetaDataPanel here.
 */
VuoPanelDocumentation::VuoPanelDocumentation(QWidget *parent) :
	QWidget(parent)
{
	this->documentationEntered = false;
	this->tooltipTimer = nullptr;
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

/**
 * Returns a translated text description of a URL,
 * clarifying the meaning of the `VuoEditor::vuo*Scheme` URL schemes.
 */
QString VuoPanelDocumentation::getDescriptionForURL(const QString &url)
{
	QUrl qurl{url};
	if (qurl.scheme() == VuoEditor::vuoNodeDocumentationScheme)
		return tr("Opens the \"%1\" documentation in this panel.").arg(qurl.host());

	if (qurl.scheme() == VuoEditor::vuoExampleCompositionScheme)
		return tr("Opens the \"%1\" example composition in a new window.").arg(qurl.path().mid(1, qurl.path().lastIndexOf('.') - 1));

	if (qurl.scheme() == VuoEditor::vuoNodeSetDocumentationScheme)
		return tr("Opens the \"%1\" documentation in a new web browser tab.").arg(qurl.host());

	return url;
}

/**
 * Invoked by QLabel when the user hovers the mouse over a hyperlink.
 */
void VuoPanelDocumentation::linkHovered(const QString &link)
{
	// Invalidate the previous pending tooltip (if any).
	delete tooltipTimer;
	tooltipTimer = nullptr;

	if (link.isEmpty())
		// When the mouse pointer stops hovering over the link,
		// immediately hide the currently-visible tooltip (if any).
		QToolTip::showText(QCursor::pos(), "");
	else
	{
		// After the mouse pointer has hovered for a while, show the tooltip.
		tooltipTimer = new QTimer(this);
		connect(tooltipTimer, &QTimer::timeout, [=]{
			QToolTip::showText(QCursor::pos(), getDescriptionForURL(link));
		});
		tooltipTimer->setSingleShot(true);
		tooltipTimer->start(500);
	}
}

/**
 * Invoked by Qt when this widget is no longer visible.
 */
void VuoPanelDocumentation::hideEvent(QHideEvent *event)
{
	// Invalidate the pending tooltip (if any).
	delete tooltipTimer;
	tooltipTimer = nullptr;
}
