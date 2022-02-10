/**
 * @file
 * VuoPublishedPortDropBox implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoPublishedPortDropBox.hh"
#include "VuoRendererColors.hh"

/**
 * Creates a "Publish Port" well.
 */
VuoPublishedPortDropBox::VuoPublishedPortDropBox(QWidget *parent) :
	QLabel(parent)
{
	setCurrentlyAcceptingDrops(false);
	updateStyleSheet();
}

/**
 * Draws the published port dropbox.
 */
void VuoPublishedPortDropBox::paintEvent(QPaintEvent *event)
{
	QLabel::paintEvent(event);
}

/**
 * Sets the boolean indicating whether the dropbox is highlighted to represent mouse hovering.
 */
void VuoPublishedPortDropBox::setHovered(bool hovered)
{
	this->isHovered = hovered;
	updateStyleSheet();
}

/**
 * Sets the boolean indicating whether the dropbox is visible, indicating that it is able to
 * receive a drop of the cable currently being dragged.
 */
void VuoPublishedPortDropBox::setCurrentlyAcceptingDrops(bool acceptingDrops)
{
	setVisible(acceptingDrops);
	if (! acceptingDrops)
		this->isHovered = false;

	updateStyleSheet();
}

/**
 * Updates the published port dropbox's style sheet based on its current state.
 */
void VuoPublishedPortDropBox::updateStyleSheet()
{
	VuoRendererColors colors(VuoNode::TintNone, VuoRendererColors::noSelection, isHovered, VuoRendererColors::noHighlight);
	QColor fillColor = colors.portFill();
	setStyleSheet(getBaseStyleSheet() + QString("background-color: rgb(%1, %2, %3);")
				  .arg(fillColor.red())
				  .arg(fillColor.green())
				  .arg(fillColor.blue()));
}

/**
 * Returns a string representing the basic style sheet for published port dropboxes.
 * The style sheet string may be appended to.
 */
QString VuoPublishedPortDropBox::getBaseStyleSheet()
{
	// Style
	const int horizontalPadding = 3;
	const int verticalPadding = 3;
	const int horizontalMargin = 2;
	const int verticalMargin = 5;

	return QString(
						"border-radius: 5px;" // Rounded corners
						"padding-right: %1px;"
						"padding-left: %1px;"
						"padding-top: %2px;"
						"padding-bottom: %2px;"
						"margin-left: %3px;"
						"margin-right: %3px;"
						"margin-top: %4px;"
						"margin-bottom: %4px;"

					)
					.arg(horizontalPadding)
					.arg(verticalPadding)
					.arg(horizontalMargin)
					.arg(verticalMargin);
}
