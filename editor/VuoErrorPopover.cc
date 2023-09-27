/**
 * @file
 * VuoErrorPopover implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoErrorPopover.hh"

#include "VuoCompilerIssue.hh"
#include "VuoRendererFonts.hh"
#include "VuoEditor.hh"

/**
 * Creates (but does not yet show) the popover.
 */
VuoErrorPopover::VuoErrorPopover(VuoCompilerIssue &issue, QWidget *parent)
	: VuoPopover(parent)
{
	QString summary = QString::fromStdString(issue.getSummary());
	QString details = QString::fromStdString(issue.getDetails(true));
	helpPath = QString::fromStdString(issue.getHelpPath());

	QString text = QString("<h3>%1</h3><p>%2</p>").arg(summary).arg(details);
	textLabel = new QLabel(text, this);

	textLabel->installEventFilter(this);
	textLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard);  // not Qt::TextBrowserInteraction since it makes text selectable
	textLabel->setOpenExternalLinks(true);

	layout = new QVBoxLayout(this);
	layout->addWidget(textLabel, 0, Qt::AlignTop);

	if (!issue.getHelpPath().empty())
	{
		QToolButton *helpButton = new QToolButton(textLabel);
		helpButton->setIcon(QIcon(":/Icons/question-circle.svg"));
		helpButton->setStyleSheet("QToolButton { border: none; }");
		helpButton->setCursor(Qt::PointingHandCursor);
		connect(helpButton, &QToolButton::clicked, this, &VuoErrorPopover::helpButtonClicked);

		// Place the button in the top-right corner of the popover.
		QVBoxLayout *innerLayout = new QVBoxLayout(textLabel);
		innerLayout->setContentsMargins(0, 4, 4, 0);
		innerLayout->addWidget(helpButton, 0, Qt::AlignTop | Qt::AlignRight);
		textLabel->setLayout(innerLayout);
	}

	setLayout(layout);

	setStyle();

	dragInProgress = false;

	VuoEditor *editor = (VuoEditor *)qApp;
	connect(editor, &VuoEditor::darkInterfaceToggled, this, &VuoErrorPopover::updateColor);
}

/**
 * Sets the popover's style attributes.
 */
void VuoErrorPopover::setStyle()
{
	textLabel->setFont(VuoRendererFonts::getSharedFonts()->portPopoverFont());
	textLabel->setMargin(5);
	textLabel->setWordWrap(true);

	Qt::WindowFlags flags = windowFlags();
	flags |= Qt::FramelessWindowHint;
	flags |= Qt::Tool;
	setWindowFlags(flags);

	// Transparent background behind rounded corners
	setAttribute(Qt::WA_TranslucentBackground, true);

	setArrowSide(Qt::AnchorTop);
	arrowTipY = -1; // Disable arrow.

	VuoEditor *editor = (VuoEditor *)qApp;
	bool isDark = editor->isInterfaceDark();
	updateColor(isDark);
}

/**
 * Draws the popover.
 */
void VuoErrorPopover::paintEvent(QPaintEvent *event)
{
	VuoEditor *editor = (VuoEditor *)qApp;
	bool isDark = editor->isInterfaceDark();
	QColor backgroundColor = isDark ? QColor("#282828") : QColor("#f9f9f9");

	QPainter painter(this);

	// Workaround to ensure that transparent background behind rounded corners works consistently
	painter.setCompositionMode(QPainter::CompositionMode_Clear);
	painter.setBrush(backgroundColor);
	painter.drawRect(QRect(0, 0, width(), height()));
	painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

	painter.setRenderHint(QPainter::Antialiasing, true);

	painter.setPen(Qt::NoPen);
	painter.fillPath(getPopoverPath(arrowSide, arrowTipY), backgroundColor);
}

/**
 * Reacts to mouse events (so that the user can drag the dialog)
 * before passing them on (so that the user can click on links in the label).
 */
bool VuoErrorPopover::eventFilter(QObject *obj, QEvent *event)
{
	if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseMove || event->type() == QEvent::MouseButtonRelease)
	{
		QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);

		if (mouseEvent->type() == QEvent::MouseButtonPress && mouseEvent->button() == Qt::LeftButton)
		{
			dragInProgress = true;
			positionBeforeDrag = mouseEvent->globalPos();
		}
		else if (mouseEvent->type() == QEvent::MouseMove && (mouseEvent->buttons() & Qt::LeftButton) && dragInProgress)
		{
			QPoint delta = mouseEvent->globalPos() - positionBeforeDrag;
			move(x() + delta.x(), y() + delta.y());
			positionBeforeDrag = mouseEvent->globalPos();
		}
		else if (mouseEvent->type() == QEvent::MouseButtonRelease && mouseEvent->button() == Qt::LeftButton)
		{
			dragInProgress = false;
		}
	}

	return false;
}

void VuoErrorPopover::helpButtonClicked()
{
	QDesktopServices::openUrl("vuo-help:" + helpPath);
}

/**
  * Specify that the arrow should be attached to the @c arrowSide of the popover.
  */
void VuoErrorPopover::setArrowSide(Qt::AnchorPoint arrowSide)
{
	this->arrowSide = arrowSide;
	layout->setContentsMargins(getPopoverContentsMargins(arrowSide));
}

/**
 * Changes the popover to float on top of all windows (if @c top is true),
 * or to behave as a normal window (if @c top is false).  If @c top is false,
 * also lowers the popover, effectively hiding it.
 */
void VuoErrorPopover::setWindowLevelAndVisibility(bool top)
{
	VuoPopover::setWindowLevelAndVisibility(top, this);
}

/**
 * Changes the popover to float on top of all windows (if @c top is true),
 * or to behave as a normal window (if @c top is false).
 */
void VuoErrorPopover::setWindowLevel(bool top)
{
	VuoPopover::setWindowLevel(top, this);
}

/**
 * Makes the widget dark.
 */
void VuoErrorPopover::updateColor(bool isDark)
{
	QString textColor = isDark ? "#cacaca" : "#000000";
	textLabel->setStyleSheet(QString("color: %1;").arg(textColor));

	emit repaint();
}
