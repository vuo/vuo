/**
 * @file
 * VuoSliderWithLabels implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoSliderWithLabels.hh"

const int VuoSliderWithLabels::minimumHeight = 37; // @todo: Figure this out programatically
const int VuoSliderWithLabels::extraVSpaceForTicks = 11;

/**
 * Creates an instance of a slider widget with labeled values.
 */
VuoSliderWithLabels::VuoSliderWithLabels(QWidget *parent) :
	QSlider(parent)
{
	setTickInterval(1);
	setDisplayTicks(false);

	minLabel = NULL;
	maxLabel = NULL;
}

/**
 * Initializes the slider in preparation for display.
 * This should be called *after* the slider's minimum, maximum,
 * and all relevant text labels have been set (via setMinimum(),
 * setMaximum(), and setLabelTextForValue(), respectively).
 *
 * It is possible to change these values and re-initialize the slider
 * while it is still being displayed.
 */
void VuoSliderWithLabels::initialize()
{
	if (!minLabel)
		minLabel = new QLabel(this);

	if (!maxLabel)
		maxLabel = new QLabel(this);

	QStyleOptionSlider slider;
	slider.initFrom(this);

	QFont labelFont = minLabel->font();
	labelFont.setPointSize(9);

	minLabel->setFont(labelFont);
	minLabel->setText(getLabelTextForValue(minimum()));

	maxLabel->setFont(labelFont);
	// Append some extra space to account for a max label that might grow and shrink,
	// since once it shrinks it can't reliably be expanded.
	maxLabel->setText(getLabelTextForValue(maximum()).append("  "));

	if (displayTicks)
	{
		for (map<int, QLabel *>::iterator i = intermediateLabels.begin(); i != intermediateLabels.end(); ++i)
		{
			// Clear the text of previously initialized labels that are no longer relevant for the updated range.
			int labelIndex = i->first;
			QLabel *label = i->second;
			if (labelIndex <= minimum() || labelIndex >= maximum())
			{
				// Don't actually make the label empty, though, since once it shrinks it can't reliably be expanded.
				QString placeholderText = "          ";
				label->setText(placeholderText);
			}
		}

		for (int i = minimum()+1; i < maximum(); ++i)
		{
			if (intermediateLabels.find(i) == intermediateLabels.end())
				intermediateLabels[i] = new QLabel(this);

			intermediateLabels[i]->setFont(minLabel->font());
			intermediateLabels[i]->setText(getLabelTextForValue(i));
		}
	}

	setTickPosition(displayTicks? QSlider::TicksBelow : QSlider::NoTicks);
	setMinimumHeight(VuoSliderWithLabels::minimumHeight + (displayTicks? extraVSpaceForTicks : 0));
}

/**
 * Handle paint events.
 */
void VuoSliderWithLabels::paintEvent(QPaintEvent *e)
{
	QStyleOptionSlider slider;
	slider.initFrom(this);

	int sliderAvailableHorizontalSpace = style()->pixelMetric(QStyle::PM_SliderSpaceAvailable, &slider, this);
	int sliderLeft = QStyle::sliderPositionFromValue(minimum(), maximum(), minimum(), sliderAvailableHorizontalSpace);
	int sliderRight = width();

	int labelVerticalOffset = style()->pixelMetric(QStyle::PM_SliderThickness, &slider, this) + (displayTicks? extraVSpaceForTicks : 0) + 10;

	minLabel->move(QPoint(sliderLeft, labelVerticalOffset));
	maxLabel->move(QPoint(min(sliderRight - maxLabel->sizeHint().width() + 6,
							  QStyle::sliderPositionFromValue(minimum(), maximum(), maximum(), sliderAvailableHorizontalSpace)),
						  labelVerticalOffset));

	if (displayTicks)
	{
		for (int i = minimum()+tickInterval(); i < maximum(); ++i)
		{
			int labelHorizontalOffset = QStyle::sliderPositionFromValue(minimum(), maximum(), i, sliderAvailableHorizontalSpace);
			intermediateLabels[i]->move(labelHorizontalOffset, labelVerticalOffset);
		}
	}

	QSlider::paintEvent(e);
}

/**
 * Returns the text that should be used in the label to describe the provided `value`.
 */
QString VuoSliderWithLabels::getLabelTextForValue(int value)
{
	return labelTextForValue[value];
}

/**
 * Sets the text that should be used in the label to describe the provided `value`.
 */
void VuoSliderWithLabels::setLabelTextForValue(int value, QString text)
{
	this->labelTextForValue[value] = text;
}

/**
 * Sets the text that should be used in the label to describe the provided `value`.
 */
void VuoSliderWithLabels::setDisplayTicks(bool display)
{
	this->displayTicks = display;
}
