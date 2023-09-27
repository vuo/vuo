/**
 * @file
 * VuoSliderWithLabels interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/**
 * Provides a slider widget with labeled values.
 */
class VuoSliderWithLabels : public QSlider
{
	Q_OBJECT

public:
	explicit VuoSliderWithLabels(QWidget *parent = 0);
	QString getLabelTextForValue(int value);
	void setLabelTextForValue(int value, QString text);
	void setDisplayTicks(bool display);
	void initialize();

protected:
	void paintEvent(QPaintEvent *e);

private:
	QLabel *minLabel;
	QLabel *maxLabel;
	map<int, QLabel *> intermediateLabels;
	map<int, QString> labelTextForValue;
	bool displayTicks;

	static const int extraVSpaceForTicks;
	static const int minimumHeight;
};
